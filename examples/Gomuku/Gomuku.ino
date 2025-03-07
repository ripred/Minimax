/**
 * Gomoku.ino - Gomoku (Five in a Row) game implementation using Minimax library
 * 
 * This sketch implements a Gomoku game that can be played:
 * - Human vs. AI
 * - AI vs. AI (self-play)
 * 
 * The game interface uses Serial communication for display and input.
 * Board visualization uses emoji symbols for better visual experience.
 * Memory optimization using bitfields to allow for the full 15x15 board.
 * 
 * March 6, 2025 ++tmw
 */

#include "Minimax.h"
#include <avr/pgmspace.h>

// Constants for board representation
#define   EMPTY     0
#define   BLACK     1  // Human player
#define   WHITE     2  // AI player

// Board dimensions
#define   BOARD_SIZE     8        // Define the Gomoku board grid size
#define   WIN_LENGTH     5        // 5 in a row to win

// Game configuration
#define   MINIMAX_DEPTH     3      // Search depth for AI (reduce for larger boards)
#define   MAX_MOVES       225      // Maximum possible moves for one position (15x15 board)

// Game modes
#define   MODE_HUMAN_VS_AI    0
#define   MODE_AI_VS_AI       1

// Game states
#define   STATE_INIT        0    // Initial state
#define   STATE_PLAYING     1    // Game in progress
#define   STATE_GAME_OVER   2    // Game over

// Direction vectors for win checking stored in PROGMEM
const PROGMEM int8_t DX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
const PROGMEM int8_t DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};

// Emoji number strings stored in PROGMEM
const char emoji0[] PROGMEM = "0️⃣ ";
const char emoji1[] PROGMEM = "1️⃣ ";
const char emoji2[] PROGMEM = "2️⃣ ";
const char emoji3[] PROGMEM = "3️⃣ ";
const char emoji4[] PROGMEM = "4️⃣ ";
const char emoji5[] PROGMEM = "5️⃣ ";
const char emoji6[] PROGMEM = "6️⃣ ";
const char emoji7[] PROGMEM = "7️⃣ ";
const char emoji8[] PROGMEM = "8️⃣ ";
const char emoji9[] PROGMEM = "9️⃣ ";

// Array of pointers to emoji strings in PROGMEM
const char* const emojiNumbers[] PROGMEM = {
  emoji0, emoji1, emoji2, emoji3, emoji4,
  emoji5, emoji6, emoji7, emoji8, emoji9
};

// Helper function to print strings from PROGMEM
void printProgmemString(const char* str) {
  char c;
  while ((c = pgm_read_byte(str++))) {
    Serial.print(c);
  }
}

// Helper function to print emoji numbers from PROGMEM
void printEmojiNumber(uint8_t number) {
  if (number < 10) {
    char buffer[6]; // Large enough for emoji characters
    const char* emojiStr = (const char*)pgm_read_word(&(emojiNumbers[number]));
    strcpy_P(buffer, emojiStr);
    Serial.print(buffer);
  } else {
    Serial.print(' ');
    Serial.print(number);
    Serial.print(' ');
  }
}

// Game state - represents the board with optimized bitfields
struct GomokuState {
  // Board representation using bitfields to optimize memory
  // Each cell needs 2 bits: 00 (empty), 01 (black), 10 (white)
  // 15x15 board = 225 cells = 450 bits = ~57 bytes
  // We'll use a 1D array of bytes with manual bit packing
  uint8_t board[(BOARD_SIZE * BOARD_SIZE * 2 + 7) / 8];
  
  // Current player turn (1 bit)
  uint8_t whiteTurn : 1;
  
  // Number of empty cells left (for more efficient terminal state checking)
  // 15x15 board needs 9 bits to represent 0-225 empty cells
  uint16_t emptyCells : 9;
  
  // Last move coordinates (for more efficient evaluation)
  uint8_t lastRow : 4;    // 0-15 needs 4 bits
  uint8_t lastCol : 4;    // 0-15 needs 4 bits
  
  // Initialize the board with empty cells
  void init() {
    whiteTurn = false;  // Black goes first
    emptyCells = BOARD_SIZE * BOARD_SIZE;
    
    // Initialize empty board (all bits to 0)
    memset(board, 0, sizeof(board));
    
    // Initialize last move to invalid position
    lastRow = 0;
    lastCol = 0;
  }
  
  // Get cell value (0=empty, 1=black, 2=white)
  uint8_t getCell(uint8_t row, uint8_t col) const {
    int pos = row * BOARD_SIZE + col;
    int bytePos = (pos * 2) / 8;
    int bitPos = (pos * 2) % 8;
    return (board[bytePos] >> bitPos) & 0x03;
  }
  
  // Set cell value (0=empty, 1=black, 2=white)
  void setCell(uint8_t row, uint8_t col, uint8_t value) {
    int pos = row * BOARD_SIZE + col;
    int bytePos = (pos * 2) / 8;
    int bitPos = (pos * 2) % 8;
    
    // Clear the two bits first
    board[bytePos] &= ~(0x03 << bitPos);
    
    // Set the new value
    board[bytePos] |= (value & 0x03) << bitPos;
  }
};

// Move structure - for Gomoku, a move is a row-column pair
struct GomokuMove {
  uint8_t row : 4;  // 0-15 needs 4 bits
  uint8_t col : 4;  // 0-15 needs 4 bits
  
  GomokuMove() : row(0), col(0) {}
  GomokuMove(uint8_t r, uint8_t c) : row(r), col(c) {}
};

// Game logic implementation
class GomokuLogic : public Minimax<GomokuState, GomokuMove, MAX_MOVES, MINIMAX_DEPTH>::GameLogic {
public:
  // Check if there's a win starting from a specific position and direction
  int checkLine(const GomokuState& state, int startRow, int startCol, int dirIdx, int piece) {
    // Read direction from PROGMEM
    int8_t dirX = pgm_read_byte(&DX[dirIdx]);
    int8_t dirY = pgm_read_byte(&DY[dirIdx]);
    
    int count = 0;
    int maxCount = 0;
    
    // Check for 5 in a row
    for (int i = -4; i <= 4; i++) {
      int r = startRow + i * dirY;
      int c = startCol + i * dirX;
      
      if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
        if (state.getCell(r, c) == piece) {
          count++;
          maxCount = max(maxCount, count);
        } else {
          count = 0;
        }
      }
    }
    
    return maxCount;
  }
  
  // Check for a win in all directions
  bool hasWin(const GomokuState& state, int piece) {
    // Check all possible directions for 5 in a row
    for (int row = 0; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        if (state.getCell(row, col) != piece) continue;
        
        // Check all 4 primary directions (other 4 are reverse of first 4)
        for (int dir = 0; dir < 4; dir++) {
          int count = 1; // Count the current piece
          
          // Check forward direction
          int8_t dirX = pgm_read_byte(&DX[dir]);
          int8_t dirY = pgm_read_byte(&DY[dir]);
          
          // Check forward
          for (int i = 1; i < WIN_LENGTH; i++) {
            int r = row + dirY * i;
            int c = col + dirX * i;
            
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || 
                state.getCell(r, c) != piece) {
              break;
            }
            count++;
          }
          
          // Check backward
          dirX = pgm_read_byte(&DX[dir+4]); // Reverse direction
          dirY = pgm_read_byte(&DY[dir+4]); // Reverse direction
          
          for (int i = 1; i < WIN_LENGTH; i++) {
            int r = row + dirY * i;
            int c = col + dirX * i;
            
            if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE || 
                state.getCell(r, c) != piece) {
              break;
            }
            count++;
          }
          
          if (count >= WIN_LENGTH) {
            return true;
          }
        }
      }
    }
    
    return false;
  }
  
  // Evaluate patterns for a specific direction
  int evaluateDirection(const GomokuState& state, int row, int col, int dirIdx, int piece) {
    // Read direction from PROGMEM
    int8_t dirX = pgm_read_byte(&DX[dirIdx]);
    int8_t dirY = pgm_read_byte(&DY[dirIdx]);
    
    uint8_t opponent = (piece == BLACK) ? WHITE : BLACK;
    int score = 0;
    
    // Pattern detection window size
    const int windowSize = 6;
    
    // Create pattern window - use static to save stack space
    static uint8_t pattern[6];
    
    // Fill pattern window
    int centerIdx = windowSize / 2 - 1;
    for (int i = 0; i < windowSize; i++) {
      pattern[i] = 0; // Default to out of bounds
      
      int r = row + (i - centerIdx) * dirY;
      int c = col + (i - centerIdx) * dirX;
      
      if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
        pattern[i] = state.getCell(r, c);
      } else {
        pattern[i] = opponent; // Treat out of bounds as blocked
      }
    }
    
    // Evaluate various patterns
    // Five in a row
    if ((pattern[0] == piece && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == piece)) {
      score += 100000;
    }
    
    // Open four (can win next move)
    if ((pattern[0] == EMPTY && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == piece && pattern[5] == EMPTY)) {
      score += 10000;
    }
    
    // Four with one end blocked
    if ((pattern[0] == opponent && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == piece && pattern[5] == EMPTY) ||
        (pattern[0] == EMPTY && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == piece && pattern[5] == opponent)) {
      score += 1000;
    }
    
    // Open three
    if ((pattern[0] == EMPTY && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == EMPTY && pattern[5] == EMPTY) ||
        (pattern[0] == EMPTY && pattern[1] == EMPTY && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == piece && pattern[5] == EMPTY)) {
      score += 500;
    }
    
    // Three with one end blocked
    if ((pattern[0] == opponent && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == EMPTY && pattern[5] == EMPTY) ||
        (pattern[0] == EMPTY && pattern[1] == EMPTY && pattern[2] == piece && 
         pattern[3] == piece && pattern[4] == piece && pattern[5] == opponent)) {
      score += 100;
    }
    
    // Open two
    if ((pattern[0] == EMPTY && pattern[1] == piece && pattern[2] == piece && 
         pattern[3] == EMPTY && pattern[4] == EMPTY && pattern[5] == EMPTY) ||
        (pattern[0] == EMPTY && pattern[1] == EMPTY && pattern[2] == EMPTY && 
         pattern[3] == piece && pattern[4] == piece && pattern[5] == EMPTY)) {
      score += 50;
    }
    
    return score;
  }
  
  // Evaluate board position from current player's perspective
  int evaluate(const GomokuState& state) override {
    // Check for terminal states first (wins)
    if (hasWin(state, BLACK)) {
      return state.whiteTurn ? 100000 : -100000; // Perspective of current player
    }
    if (hasWin(state, WHITE)) {
      return state.whiteTurn ? -100000 : 100000; // Perspective of current player
    }
    
    int score = 0;
    
    // Current player piece
    uint8_t currentPiece = state.whiteTurn ? WHITE : BLACK;
    uint8_t opponentPiece = state.whiteTurn ? BLACK : WHITE;
    
    // Instead of checking all cells, focus evaluation around the last move
    // and a selection of strategic positions
    
    // Evaluate the last move area
    int lastRow = state.lastRow;
    int lastCol = state.lastCol;
    
    // Evaluate patterns in all directions from the last move
    if (lastRow > 0 || lastCol > 0) { // If there's a last move
      for (int dir = 0; dir < 4; dir++) {
        score += evaluateDirection(state, lastRow, lastCol, dir, currentPiece);
        score -= evaluateDirection(state, lastRow, lastCol, dir, opponentPiece);
      }
    }
    
    // Evaluate strategic positions (center and key points)
    int center = BOARD_SIZE / 2;
    
    // Evaluate center region - just check a smaller area for efficiency
    for (int row = center-2; row <= center+2; row++) {
      for (int col = center-2; col <= center+2; col++) {
        if (state.getCell(row, col) == currentPiece) {
          int distFromCenter = abs(row - center) + abs(col - center);
          score += max(0, 10 - distFromCenter);
        }
      }
    }
    
    return score;
  }
  
  // Generate all valid moves from the current state
  int generateMoves(const GomokuState& state, GomokuMove moves[], int maxMoves) override {
    int moveCount = 0;
    
    // First move optimization: place in the center
    if (state.emptyCells == BOARD_SIZE * BOARD_SIZE) {
      moves[0] = GomokuMove(BOARD_SIZE / 2, BOARD_SIZE / 2);
      return 1;
    }
    
    // Smart move generation - only consider empty spaces that are
    // within 2 cells of an existing piece to reduce the search space
    const int vicinity = 2;
    
    // Use a single static array to track considered moves across function calls
    // This saves stack space compared to a fresh array each call
    static bool considered[BOARD_SIZE][BOARD_SIZE];
    memset(considered, 0, sizeof(considered)); // Clear the array efficiently
    
    // First pass - look around existing pieces
    for (int row = 0; row < BOARD_SIZE && moveCount < maxMoves; row++) {
      for (int col = 0; col < BOARD_SIZE && moveCount < maxMoves; col++) {
        if (state.getCell(row, col) != EMPTY) {
          // Check vicinity around this piece
          for (int dr = -vicinity; dr <= vicinity && moveCount < maxMoves; dr++) {
            for (int dc = -vicinity; dc <= vicinity && moveCount < maxMoves; dc++) {
              int r = row + dr;
              int c = col + dc;
              
              if (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE &&
                  state.getCell(r, c) == EMPTY && !considered[r][c]) {
                moves[moveCount] = GomokuMove(r, c);
                moveCount++;
                considered[r][c] = true;
              }
            }
          }
        }
      }
    }
    
    // If no moves were found in vicinity (unlikely), add all empty spaces
    if (moveCount == 0) {
      for (int row = 0; row < BOARD_SIZE && moveCount < maxMoves; row++) {
        for (int col = 0; col < BOARD_SIZE && moveCount < maxMoves; col++) {
          if (state.getCell(row, col) == EMPTY) {
            moves[moveCount] = GomokuMove(row, col);
            moveCount++;
          }
        }
      }
    }
    
    return moveCount;
  }
  
  // Apply a move to a state, modifying the state
  void applyMove(GomokuState& state, const GomokuMove& move) override {
    // Place the piece
    state.setCell(move.row, move.col, state.whiteTurn ? WHITE : BLACK);
    
    // Update state
    state.emptyCells--;
    state.lastRow = move.row;
    state.lastCol = move.col;
    
    // Switch turns
    state.whiteTurn = !state.whiteTurn;
  }
  
  // Check if the game has reached a terminal state (win/loss/draw)
  bool isTerminal(const GomokuState& state) override {
    // Check if either player has won
    if (hasWin(state, BLACK) || hasWin(state, WHITE)) {
      return true;
    }
    
    // Check for a draw (board is full)
    if (state.emptyCells == 0) {
      return true;
    }
    
    return false;
  }
  
  // Check if the current player is the maximizing player
  bool isMaximizingPlayer(const GomokuState& state) override {
    // WHITE is the maximizing player (AI)
    return state.whiteTurn;
  }
};

// Forward declarations to fix compiler errors
class GomokuLogic;
void displayBoard(const GomokuState& state);
GomokuMove getHumanMove();
GomokuMove getAIMove();
bool checkGameOver();
void setupGame();

// Global variables
GomokuState gameState;
GomokuLogic gameLogic;
Minimax<GomokuState, GomokuMove, MAX_MOVES, MINIMAX_DEPTH> minimaxAI(gameLogic);

int gameMode = MODE_HUMAN_VS_AI;    // Default to Human vs AI
int gameCurrentState = STATE_INIT;  // Current game state
bool waitingForRestart = false;     // Flag to control game flow
bool firstRun = true;               // Flag for first run to prompt for game mode
int moveNum = 0;                    // the current move number

// Function to display the board with emoji symbols - Othello style
void displayBoard(const GomokuState& state) {
  // Column numbers with regular ASCII numbers
  Serial.print(F("  "));
  for (int col = 0; col < BOARD_SIZE; col++) {
    if (col < 10) {
      printEmojiNumber(col);
    } else {
      Serial.print("・");
      Serial.print(col);
      // Serial.print(' ');
    }
  }
  Serial.println();
  
  for (int row = 0; row < BOARD_SIZE; row++) {
    // Use plain ASCII numbers for row indicators
    if (row < 10) {
      Serial.print(row);
      Serial.print(F(" "));
    } else {
      Serial.print(row);
      Serial.print(F(" "));
    }
    
    for (int col = 0; col < BOARD_SIZE; col++) {
      switch (state.getCell(row, col)) {
        case EMPTY:
          Serial.print(F("・ ")); // Empty square
          break;
        case BLACK:
          Serial.print(F("⚫ ")); // Black stone
          break;
        case WHITE:
          Serial.print(F("⚪ ")); // White stone
          break;
      }
    }
    
    Serial.println();
  }
  
  // Display current player
  int blackCount = 0;
  int whiteCount = 0;
  
  // Count pieces
  for (int row = 0; row < BOARD_SIZE; row++) {
    for (int col = 0; col < BOARD_SIZE; col++) {
      if (state.getCell(row, col) == BLACK) blackCount++;
      else if (state.getCell(row, col) == WHITE) whiteCount++;
    }
  }
  
  Serial.print(F("⚫ BLACK: "));
  Serial.print(blackCount);
  Serial.print(F("  ⚪ WHITE: "));
  Serial.println(whiteCount);
  
  Serial.print(state.whiteTurn ? F("⚪ WHITE's turn") : F("⚫ BLACK's turn"));
  Serial.println();
}

// Function to get a move from human player
GomokuMove getHumanMove() {
  GomokuMove move;
  bool validMove = false;
  
  while (!validMove) {
    // Prompt for input
    Serial.println(F("Enter row and column (e.g., '7 7'):"));
    
    // Wait for input
    while (!Serial.available()) {
      delay(100);
    }
    
    // Read row and column
    move.row = Serial.parseInt();
    move.col = Serial.parseInt();
    
    // Clear the input buffer
    while (Serial.available()) {
      Serial.read();
    }
    
    // Check if the move is valid
    if (move.row < BOARD_SIZE && move.col < BOARD_SIZE) {
      if (gameState.getCell(move.row, move.col) == EMPTY) {
        validMove = true;
      } else {
        Serial.println(F("Position already occupied. Try another one."));
      }
    } else {
      Serial.println(F("Invalid position. Please enter values between 0 and 14."));
    }
  }
  
  return move;
}

// Function to get AI move
GomokuMove getAIMove() {
  Serial.println(F("AI is thinking..."));
  
  unsigned long startTime = millis();
  GomokuMove move = minimaxAI.findBestMove(gameState);
  unsigned long endTime = millis();
  
  Serial.print(F("AI chose position: "));
  Serial.print(move.row);
  Serial.print(F(", "));
  Serial.println(move.col);
  
  Serial.print(F("Nodes searched: "));
  Serial.println(minimaxAI.getNodesSearched());
  
  Serial.print(F("Time: "));
  Serial.print((endTime - startTime) / 1000.0);
  Serial.println(F(" seconds"));
  
  return move;
}

// Function to check for game over
bool checkGameOver() {
  if (gameLogic.isTerminal(gameState)) {
    displayBoard(gameState);
    
    // Determine the winner
    if (gameLogic.hasWin(gameState, BLACK)) {
      Serial.println(F("BLACK wins! ⚫"));
    } else if (gameLogic.hasWin(gameState, WHITE)) {
      Serial.println(F("WHITE wins! ⚪"));
    } else {
      Serial.println(F("Game ended in a draw!"));
    }
    
    Serial.println(F("Enter 'r' to restart or 'm' to change mode."));
    waitingForRestart = true;
    gameCurrentState = STATE_GAME_OVER;
    return true;
  }
  
  return false;
}

// function to attempt to generate consistently different game PRN seeds
uint32_t generateSeed() {
  uint32_t  seed = 0;
  uint16_t total = analogRead(A0);
  randomSeed(total);
  for (uint16_t i=0; i < total; i++) {
    seed += analogRead(A0);
  }

  randomSeed(seed);
  return seed;
}

// Function to handle game setup and restart
void setupGame() {
  randomSeed(generateSeed());

  // Initialize game state
  gameState.init();
  
  // Only show the game mode selection on first run
  if (firstRun) {
    Serial.println(F("\n=== GOMOKU (FIVE IN A ROW) ==="));
    Serial.print(F("Ply Depth: "));
    Serial.println(MINIMAX_DEPTH, DEC);
    Serial.println(F("Game Modes:"));
    Serial.println(F("1. Human (Black) vs. AI (White)"));
    Serial.println(F("2. AI vs. AI"));
    Serial.println(F("Select mode (1-2):"));
    
    char choice = 0;
    while (choice != '1' && choice != '2') {
      while (!Serial.available()) {
        delay(100);
      }
      
      choice = Serial.read();
      
      // Clear the input buffer
      while (Serial.available()) {
        Serial.read();
      }
      
      if (choice != '1' && choice != '2') {
        Serial.println(F("Invalid choice. Please enter 1 or 2:"));
      }
    }
    
    gameMode = (choice == '2') ? MODE_AI_VS_AI : MODE_HUMAN_VS_AI;
    firstRun = false;
  } else {
    // Just display the title and selected mode
    Serial.println(F("\n=== GOMOKU (FIVE IN A ROW) ==="));
    if (gameMode == MODE_AI_VS_AI) {
      Serial.println(F("AI vs. AI mode selected."));
    } else {
      Serial.println(F("Human vs. AI mode selected."));
      Serial.println(F("You play as Black (⚫), AI plays as White (⚪)."));
    }
  }
  
  // Reset the restart flag and set game state to playing
  waitingForRestart = false;
  gameCurrentState = STATE_PLAYING;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  randomSeed(generateSeed());
  
  // Initialize the game
  setupGame();
  
  // Display the board initially
  displayBoard(gameState);
}

void loop() {
  // State machine approach to handle game flow
  switch (gameCurrentState) {
    case STATE_INIT:
      // Should never get here after setup
      setupGame();
      displayBoard(gameState);
      break;
      
    case STATE_GAME_OVER:
      // Check for restart input
      if (Serial.available()) {
        char choice = Serial.read();
        
        // Clear input buffer
        while (Serial.available()) {
          Serial.read();
        }
        
        if (choice == 'r') {
          setupGame();
          displayBoard(gameState);
        } else if (choice == 'm') {
          gameMode = (gameMode == MODE_HUMAN_VS_AI) ? MODE_AI_VS_AI : MODE_HUMAN_VS_AI;
          setupGame();
          displayBoard(gameState);
        }
      }
      delay(100);
      break;
      
    case STATE_PLAYING: {
      // Check for game over first
      if (checkGameOver()) {
        break; // Game is over, wait for restart input
      }
      
      // Handle current player's move
      GomokuMove move;
      moveNum++;
      Serial.print(F("Move #"));
      Serial.print(moveNum, DEC);  
      Serial.print(F(", "));

      if (gameMode == MODE_HUMAN_VS_AI) {
        if (!gameState.whiteTurn) {
          // Human's turn (Black)
          move = getHumanMove();
        } else {
          // AI's turn (White)
          move = getAIMove();
          delay(1000); // Small delay to make AI moves visible
        }
      } else {
        // AI vs. AI mode
        move = getAIMove();
        delay(2000); // Longer delay to observe the game
      }
      
      // Apply the move
      gameLogic.applyMove(gameState, move);
      
      // Display the updated board
      displayBoard(gameState);
      break;
    }
  }
}