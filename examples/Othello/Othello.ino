/**
 * Othello.ino - Othello/Reversi game implementation using Minimax library
 * 
 * This sketch implements an Othello/Reversi game that can be played:
 * - Human vs. AI
 * - AI vs. AI (self-play)
 * 
 * The game interface uses Serial communication for display and input.
 * Board visualization uses emoji symbols for better visual experience.
 * 
 * March 4, 2025 ++tmw
 */

#include "Minimax.h"

// Constants for board representation
#define EMPTY   0
#define BLACK   1    // First player (human in Human vs. AI mode)
#define WHITE   2    // Second player (AI in Human vs. AI mode)

// Game configuration
#define MINIMAX_DEPTH  2      // Search depth for AI (reduced for memory constraints)
#define MAX_MOVES      60     // Maximum possible moves (worst case)

// Board dimensions
#define ROWS    8
#define COLS    8

// Game modes
#define MODE_HUMAN_VS_AI    0
#define MODE_AI_VS_AI       1

// Direction vectors for searching in 8 directions
const int8_t DIRECTIONS[8][2] = {
  {-1, -1}, {-1, 0}, {-1, 1},   // NW, N, NE
  {0, -1},           {0, 1},    // W, E
  {1, -1},  {1, 0},  {1, 1}     // SW, S, SE
};

// Game state - represents the board
struct OthelloState {
  byte board[ROWS][COLS];
  bool whiteTurn;  // true if it's white's turn, false for black's turn
  
  // Initialize the board with starting position
  void init() {
    whiteTurn = false;  // Black goes first
    
    // Initialize empty board
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        board[row][col] = EMPTY;
      }
    }
    
    // Set the four center pieces in starting position
    board[3][3] = WHITE;
    board[3][4] = BLACK;
    board[4][3] = BLACK;
    board[4][4] = WHITE;
  }
};

// Move structure - an Othello move is a row and column position
struct OthelloMove {
  byte row;
  byte col;
  
  OthelloMove() : row(0), col(0) {}
  OthelloMove(byte r, byte c) : row(r), col(c) {}
};

// Game logic implementation
class OthelloLogic : public Minimax<OthelloState, OthelloMove, MAX_MOVES, MINIMAX_DEPTH>::GameLogic {
public:
  // Check if a position is on the board
  bool isOnBoard(int row, int col) {
    return row >= 0 && row < ROWS && col >= 0 && col < COLS;
  }
  
  // Get the opponent's piece color
  byte getOpponent(byte piece) {
    return (piece == BLACK) ? WHITE : BLACK;
  }
  
  // Check if a move is valid by checking if it captures any opponent pieces
  bool isValidMove(const OthelloState& state, int row, int col, byte piece) {
    // The position must be empty
    if (state.board[row][col] != EMPTY) {
      return false;
    }
    
    byte opponent = getOpponent(piece);
    bool validInAnyDirection = false;
    
    // Check all 8 directions
    for (int d = 0; d < 8; d++) {
      int dr = DIRECTIONS[d][0];
      int dc = DIRECTIONS[d][1];
      int r = row + dr;
      int c = col + dc;
      
      // Look for opponent's piece adjacent
      if (isOnBoard(r, c) && state.board[r][c] == opponent) {
        // Continue in this direction looking for player's piece
        r += dr;
        c += dc;
        while (isOnBoard(r, c)) {
          if (state.board[r][c] == EMPTY) {
            break; // Empty space, not valid in this direction
          }
          if (state.board[r][c] == piece) {
            validInAnyDirection = true; // Found own piece, valid move
            break;
          }
          // Continue checking
          r += dr;
          c += dc;
        }
      }
    }
    
    return validInAnyDirection;
  }
  
  // Count how many pieces would be flipped by a move
  int countFlips(const OthelloState& state, int row, int col, byte piece) {
    byte opponent = getOpponent(piece);
    int flips = 0;
    
    // Check all 8 directions
    for (int d = 0; d < 8; d++) {
      int dr = DIRECTIONS[d][0];
      int dc = DIRECTIONS[d][1];
      int r = row + dr;
      int c = col + dc;
      
      int flipsInThisDirection = 0;
      
      // Look for opponent's piece adjacent
      if (isOnBoard(r, c) && state.board[r][c] == opponent) {
        // Continue in this direction looking for player's piece
        r += dr;
        c += dc;
        while (isOnBoard(r, c)) {
          if (state.board[r][c] == EMPTY) {
            flipsInThisDirection = 0; // Not valid, reset count
            break;
          }
          if (state.board[r][c] == piece) {
            flips += flipsInThisDirection; // Valid, add to total
            break;
          }
          flipsInThisDirection++; // Count potential flip
          r += dr;
          c += dc;
        }
      }
    }
    
    return flips;
  }
  
  // Flip captured pieces after a move
  void flipPieces(OthelloState& state, int row, int col, byte piece) {
    byte opponent = getOpponent(piece);
    
    // Check all 8 directions
    for (int d = 0; d < 8; d++) {
      int dr = DIRECTIONS[d][0];
      int dc = DIRECTIONS[d][1];
      int r = row + dr;
      int c = col + dc;
      
      bool foundOpponent = false;
      
      // Look for opponent's piece adjacent
      if (isOnBoard(r, c) && state.board[r][c] == opponent) {
        foundOpponent = true;
        
        // Continue in this direction looking for player's piece
        r += dr;
        c += dc;
        while (isOnBoard(r, c)) {
          if (state.board[r][c] == EMPTY) {
            foundOpponent = false; // Not valid in this direction
            break;
          }
          if (state.board[r][c] == piece) {
            break; // Found own piece, valid direction
          }
          r += dr;
          c += dc;
        }
        
        // If we found a valid line to flip, go back and do the flipping
        if (foundOpponent && isOnBoard(r, c) && state.board[r][c] == piece) {
          r = row + dr;
          c = col + dc;
          while (state.board[r][c] == opponent) {
            state.board[r][c] = piece;
            r += dr;
            c += dc;
          }
        }
      }
    }
  }
  
  // Count pieces of a specific color on the board
  int countPieces(const OthelloState& state, byte piece) {
    int count = 0;
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (state.board[row][col] == piece) {
          count++;
        }
      }
    }
    return count;
  }
  
  // Check if the current player has any valid moves
  bool hasValidMoves(const OthelloState& state, byte piece) {
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (isValidMove(state, row, col, piece)) {
          return true;
        }
      }
    }
    return false;
  }
  
  // Evaluate board position from current player's perspective
  int evaluate(const OthelloState& state) override {
    // For the current player
    byte currentPiece = state.whiteTurn ? WHITE : BLACK;
    byte opponentPiece = getOpponent(currentPiece);
    
    // Count pieces
    int currentPieceCount = countPieces(state, currentPiece);
    int opponentPieceCount = countPieces(state, opponentPiece);
    
    // Basic score: difference in piece count
    int score = currentPieceCount - opponentPieceCount;
    
    // Bonus for corner pieces (very valuable in Othello)
    const int cornerValue = 25;
    if (state.board[0][0] == currentPiece) score += cornerValue;
    if (state.board[0][7] == currentPiece) score += cornerValue;
    if (state.board[7][0] == currentPiece) score += cornerValue;
    if (state.board[7][7] == currentPiece) score += cornerValue;
    
    if (state.board[0][0] == opponentPiece) score -= cornerValue;
    if (state.board[0][7] == opponentPiece) score -= cornerValue;
    if (state.board[7][0] == opponentPiece) score -= cornerValue;
    if (state.board[7][7] == opponentPiece) score -= cornerValue;
    
    // Penalty for positions adjacent to corners if corner is empty
    // (this can give opponent access to corners)
    const int badPositionValue = 8;
    
    // Top-left corner adjacents
    if (state.board[0][0] == EMPTY) {
      if (state.board[0][1] == currentPiece) score -= badPositionValue;
      if (state.board[1][0] == currentPiece) score -= badPositionValue;
      if (state.board[1][1] == currentPiece) score -= badPositionValue;
      
      if (state.board[0][1] == opponentPiece) score += badPositionValue;
      if (state.board[1][0] == opponentPiece) score += badPositionValue;
      if (state.board[1][1] == opponentPiece) score += badPositionValue;
    }
    
    // Top-right corner adjacents
    if (state.board[0][7] == EMPTY) {
      if (state.board[0][6] == currentPiece) score -= badPositionValue;
      if (state.board[1][7] == currentPiece) score -= badPositionValue;
      if (state.board[1][6] == currentPiece) score -= badPositionValue;
      
      if (state.board[0][6] == opponentPiece) score += badPositionValue;
      if (state.board[1][7] == opponentPiece) score += badPositionValue;
      if (state.board[1][6] == opponentPiece) score += badPositionValue;
    }
    
    // Bottom-left corner adjacents
    if (state.board[7][0] == EMPTY) {
      if (state.board[7][1] == currentPiece) score -= badPositionValue;
      if (state.board[6][0] == currentPiece) score -= badPositionValue;
      if (state.board[6][1] == currentPiece) score -= badPositionValue;
      
      if (state.board[7][1] == opponentPiece) score += badPositionValue;
      if (state.board[6][0] == opponentPiece) score += badPositionValue;
      if (state.board[6][1] == opponentPiece) score += badPositionValue;
    }
    
    // Bottom-right corner adjacents
    if (state.board[7][7] == EMPTY) {
      if (state.board[7][6] == currentPiece) score -= badPositionValue;
      if (state.board[6][7] == currentPiece) score -= badPositionValue;
      if (state.board[6][6] == currentPiece) score -= badPositionValue;
      
      if (state.board[7][6] == opponentPiece) score += badPositionValue;
      if (state.board[6][7] == opponentPiece) score += badPositionValue;
      if (state.board[6][6] == opponentPiece) score += badPositionValue;
    }
    
    // Bonus for edge pieces (also valuable)
    const int edgeValue = 5;
    for (int i = 2; i < 6; i++) {
      if (state.board[0][i] == currentPiece) score += edgeValue;
      if (state.board[7][i] == currentPiece) score += edgeValue;
      if (state.board[i][0] == currentPiece) score += edgeValue;
      if (state.board[i][7] == currentPiece) score += edgeValue;
      
      if (state.board[0][i] == opponentPiece) score -= edgeValue;
      if (state.board[7][i] == opponentPiece) score -= edgeValue;
      if (state.board[i][0] == opponentPiece) score -= edgeValue;
      if (state.board[i][7] == opponentPiece) score -= edgeValue;
    }
    
    // Bonus for mobility (having more valid moves)
    int currentMoveCount = 0;
    int opponentMoveCount = 0;
    
    // Temporarily change state to count opponent's moves
    OthelloState tempState = state;
    tempState.whiteTurn = !tempState.whiteTurn;
    
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (isValidMove(state, row, col, currentPiece)) {
          currentMoveCount++;
        }
        if (isValidMove(tempState, row, col, opponentPiece)) {
          opponentMoveCount++;
        }
      }
    }
    
    const int mobilityValue = 2;
    score += mobilityValue * (currentMoveCount - opponentMoveCount);
    
    // Terminal state considerations
    if (isTerminal(state)) {
      if (currentPieceCount > opponentPieceCount) {
        return 10000; // Win
      } else if (currentPieceCount < opponentPieceCount) {
        return -10000; // Loss
      } else {
        return 0; // Draw
      }
    }
    
    return score;
  }
  
  // Generate all valid moves from the current state
  int generateMoves(const OthelloState& state, OthelloMove moves[], int maxMoves) override {
    int moveCount = 0;
    byte currentPiece = state.whiteTurn ? WHITE : BLACK;
    
    // A move is valid if it flips at least one opponent's piece
    for (int row = 0; row < ROWS && moveCount < maxMoves; row++) {
      for (int col = 0; col < COLS && moveCount < maxMoves; col++) {
        if (isValidMove(state, row, col, currentPiece)) {
          moves[moveCount] = OthelloMove(row, col);
          moveCount++;
        }
      }
    }
    
    return moveCount;
  }
  
  // Apply a move to a state, modifying the state
  void applyMove(OthelloState& state, const OthelloMove& move) override {
    byte currentPiece = state.whiteTurn ? WHITE : BLACK;
    
    // Place the piece
    state.board[move.row][move.col] = currentPiece;
    
    // Flip captured pieces
    flipPieces(state, move.row, move.col, currentPiece);
    
    // Switch turns
    state.whiteTurn = !state.whiteTurn;
  }
  
  // Check if the game has reached a terminal state
  bool isTerminal(const OthelloState& state) override {
    // Game is over if neither player has valid moves
    bool blackHasMoves = hasValidMoves(state, BLACK);
    bool whiteHasMoves = hasValidMoves(state, WHITE);
    
    if (!blackHasMoves && !whiteHasMoves) {
      return true;
    }
    
    return false;
  }
  
  // Check if the current player is the maximizing player
  bool isMaximizingPlayer(const OthelloState& state) override {
    // WHITE is the maximizing player (AI)
    return state.whiteTurn;
  }
};

// Global variables
OthelloState gameState;
OthelloLogic gameLogic;
Minimax<OthelloState, OthelloMove, MAX_MOVES, MINIMAX_DEPTH> minimaxAI(gameLogic);

int gameMode = MODE_HUMAN_VS_AI;  // Default to Human vs AI

// Function to display the board with emoji symbols
void displayBoard(const OthelloState& state) {
  // Column numbers with emoji numbers
  Serial.print("  ");
  for (int col = 0; col < COLS; col++) {
    switch(col) {
      case 0: Serial.print(" 0️⃣ "); break;
      case 1: Serial.print("1️⃣ "); break;
      case 2: Serial.print("2️⃣ "); break;
      case 3: Serial.print("3️⃣ "); break;
      case 4: Serial.print("4️⃣ "); break;
      case 5: Serial.print("5️⃣ "); break;
      case 6: Serial.print("6️⃣ "); break;
      case 7: Serial.print("7️⃣ "); break;
    }
  }
  Serial.println();
  
  for (int row = 0; row < ROWS; row++) {
    // Row numbers with emoji numbers
    switch(row) {
      case 0: Serial.print("0️⃣ "); break;
      case 1: Serial.print("1️⃣ "); break;
      case 2: Serial.print("2️⃣ "); break;
      case 3: Serial.print("3️⃣ "); break;
      case 4: Serial.print("4️⃣ "); break;
      case 5: Serial.print("5️⃣ "); break;
      case 6: Serial.print("6️⃣ "); break;
      case 7: Serial.print("7️⃣ "); break;
    }
    
    for (int col = 0; col < COLS; col++) {
      byte currentPiece = state.whiteTurn ? WHITE : BLACK;
      
      if (state.board[row][col] == EMPTY) {
        // Show hint if this is a valid move for current player
        if (gameLogic.isValidMove(state, row, col, currentPiece)) {
          Serial.print("❓"); // Question mark for valid move
        } else {
          Serial.print("⬜"); // Empty square
        }
      } else if (state.board[row][col] == BLACK) {
        Serial.print("⚫"); // Black circle
      } else { // WHITE
        Serial.print("⚪"); // White circle
      }
      Serial.print(" ");
    }
    
    Serial.println();
  }
  
  // Display current player and piece counts
  int blackCount = gameLogic.countPieces(state, BLACK);
  int whiteCount = gameLogic.countPieces(state, WHITE);
  
  Serial.print("⚫ BLACK: ");
  Serial.print(blackCount);
  Serial.print("  ⚪ WHITE: ");
  Serial.println(whiteCount);
  
  Serial.print(state.whiteTurn ? "⚪ WHITE's turn" : "⚫ BLACK's turn");
  Serial.println();
}

// Function to get a move from human player
OthelloMove getHumanMove() {
  OthelloMove move;
  bool validMove = false;
  byte currentPiece = gameState.whiteTurn ? WHITE : BLACK;
  
  while (!validMove) {
    // Prompt for input
    Serial.println("Enter row and column (e.g., '3 4'):");
    
    // Wait for input
    while (!Serial.available()) {
      delay(100);
    }
    
    // Read the row and column
    move.row = Serial.parseInt();
    move.col = Serial.parseInt();
    
    // Clear the input buffer
    while (Serial.available()) {
      Serial.read();
    }
    
    // Check if the position is valid
    if (move.row < ROWS && move.col < COLS) {
      // Check if the move is legal
      if (gameLogic.isValidMove(gameState, move.row, move.col, currentPiece)) {
        validMove = true;
      } else {
        Serial.println("Invalid move. Try another position.");
      }
    } else {
      Serial.println("Invalid position. Please enter row (0-7) and column (0-7).");
    }
  }
  
  return move;
}

// Function to get AI move
OthelloMove getAIMove() {
  Serial.println("AI is thinking...");
  
  unsigned long startTime = millis();
  OthelloMove move = minimaxAI.findBestMove(gameState);
  unsigned long endTime = millis();
  
  Serial.print("AI chose position: ");
  Serial.print(move.row);
  Serial.print(", ");
  Serial.println(move.col);
  
  Serial.print("Nodes searched: ");
  Serial.println(minimaxAI.getNodesSearched());
  
  Serial.print("Time: ");
  Serial.print((endTime - startTime) / 1000.0);
  Serial.println(" seconds");
  
  return move;
}

// Function to check for game over
bool checkGameOver() {
  if (gameLogic.isTerminal(gameState)) {
    displayBoard(gameState);
    
    // Determine the winner
    int blackCount = gameLogic.countPieces(gameState, BLACK);
    int whiteCount = gameLogic.countPieces(gameState, WHITE);
    
    if (blackCount > whiteCount) {
      Serial.println("BLACK wins!");
    } else if (whiteCount > blackCount) {
      Serial.println("WHITE wins!");
    } else {
      Serial.println("Game ended in a draw!");
    }
    
    Serial.println("Enter 'r' to restart or 'm' to change mode.");
    return true;
  }
  
  return false;
}

// Function to handle game setup and restart
void setupGame() {
  gameState.init();
  
  Serial.println("\n=== OTHELLO / REVERSI ===");
  Serial.println("Game Modes:");
  Serial.println("1. Human (Black) vs. AI (White)");
  Serial.println("2. AI vs. AI");
  Serial.println("Select mode (1-2):");
  
  while (!Serial.available()) {
    delay(100);
  }
  
  char choice = Serial.read();
  
  // Clear the input buffer
  while (Serial.available()) {
    Serial.read();
  }
  
  if (choice == '2') {
    gameMode = MODE_AI_VS_AI;
    Serial.println("AI vs. AI mode selected.");
  } else {
    gameMode = MODE_HUMAN_VS_AI;
    Serial.println("Human vs. AI mode selected.");
    Serial.println("You play as Black, AI plays as White.");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  randomSeed(analogRead(0));
  setupGame();
}

void loop() {
  // Display the current board state
  displayBoard(gameState);
  
  // Check for game over
  if (checkGameOver()) {
    while (!Serial.available()) {
      delay(100);
    }
    
    char choice = Serial.read();
    
    // Clear input buffer
    while (Serial.available()) {
      Serial.read();
    }
    
    if (choice == 'r') {
      setupGame();
    } else if (choice == 'm') {
      gameMode = (gameMode == MODE_HUMAN_VS_AI) ? MODE_AI_VS_AI : MODE_HUMAN_VS_AI;
      setupGame();
    }
    return;
  }
  
  // Get current player piece
  byte currentPiece = gameState.whiteTurn ? WHITE : BLACK;
  
  // Check if current player has valid moves
  bool hasValidMove = gameLogic.hasValidMoves(gameState, currentPiece);
  
  if (!hasValidMove) {
    // Current player has no valid moves
    Serial.println("No valid moves for current player, skipping turn");
    
    // Switch turns
    gameState.whiteTurn = !gameState.whiteTurn;
    
    // Check if the other player also has no valid moves (game over)
    currentPiece = gameState.whiteTurn ? WHITE : BLACK;
    if (!gameLogic.hasValidMoves(gameState, currentPiece)) {
      Serial.println("Both players have no valid moves. Game over!");
      checkGameOver();
      return;
    }
    
    // Display updated board after turn skip
    displayBoard(gameState);
  }
  
  // Get and apply move based on game mode and current player
  OthelloMove move;
  
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
}