/**
 * ConnectFour.ino - Connect Four game implementation using Minimax library
 * 
 * This sketch implements a Connect Four game that can be played:
 * - Human vs. AI
 * - AI vs. AI (self-play)
 * 
 * The game interface uses Serial communication for display and input.
 * Board visualization uses emoji symbols for better visual experience.
 * 
 * March 3, 2025 ++tmw
 */

#include "Minimax.h"

// Constants for board representation
#define    EMPTY   0
#define    RED     1    // Human player
#define    BLUE    2    // AI player

// Game configuration
#define    MINIMAX_DEPTH    4      // Search depth for AI
#define    MAX_MOVES        7      // Maximum possible moves (columns) for one position

// Board dimensions
#define    ROWS    6
#define    COLS    7

// Game modes
#define    MODE_HUMAN_VS_AI    0
#define    MODE_AI_VS_AI       1

// Game state - represents the board
struct ConnectFourState {
  byte board[ROWS][COLS];
  bool blueTurn;  // true if it's blue's turn, false for red's turn
  
  // Initialize the board with empty cells
  void init() {
    blueTurn = false;  // Red goes first
    
    // Initialize empty board
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        board[row][col] = EMPTY;
      }
    }
  }
};

// Move structure - for Connect Four, a move is just a column choice
struct ConnectFourMove {
  byte column;
  
  ConnectFourMove() : column(0) {}
  ConnectFourMove(byte col) : column(col) {}
};

// Game logic implementation
class ConnectFourLogic : public Minimax<ConnectFourState, ConnectFourMove, MAX_MOVES, MINIMAX_DEPTH>::GameLogic {
public:
  // Find the row where a piece would land if dropped in the given column
  int findDropRow(const ConnectFourState& state, int col) {
    for (int row = ROWS - 1; row >= 0; row--) {
      if (state.board[row][col] == EMPTY) {
        return row;
      }
    }
    return -1; // Column is full
  }
  
  // Check if there's a win starting from a specific position
  bool checkWin(const ConnectFourState& state, int startRow, int startCol, int piece) {
    // Check horizontal
    int count = 0;
    for (int c = max(0, startCol - 3); c < min(COLS, startCol + 4); c++) {
      if (state.board[startRow][c] == piece) {
        count++;
        if (count >= 4) return true;
      } else {
        count = 0;
      }
    }
    
    // Check vertical
    count = 0;
    for (int r = max(0, startRow - 3); r < min(ROWS, startRow + 4); r++) {
      if (state.board[r][startCol] == piece) {
        count++;
        if (count >= 4) return true;
      } else {
        count = 0;
      }
    }
    
    // Check diagonal (top-left to bottom-right)
    count = 0;
    for (int i = -3; i <= 3; i++) {
      int r = startRow + i;
      int c = startCol + i;
      if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
        if (state.board[r][c] == piece) {
          count++;
          if (count >= 4) return true;
        } else {
          count = 0;
        }
      }
    }
    
    // Check diagonal (top-right to bottom-left)
    count = 0;
    for (int i = -3; i <= 3; i++) {
      int r = startRow + i;
      int c = startCol - i;
      if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
        if (state.board[r][c] == piece) {
          count++;
          if (count >= 4) return true;
        } else {
          count = 0;
        }
      }
    }
    
    return false;
  }
  
  // Check for a win more efficiently (check entire board)
  bool hasWin(const ConnectFourState& state, int piece) {
    // Horizontal check
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col <= COLS - 4; col++) {
        if (state.board[row][col] == piece &&
            state.board[row][col+1] == piece &&
            state.board[row][col+2] == piece &&
            state.board[row][col+3] == piece) {
          return true;
        }
      }
    }
    
    // Vertical check
    for (int row = 0; row <= ROWS - 4; row++) {
      for (int col = 0; col < COLS; col++) {
        if (state.board[row][col] == piece &&
            state.board[row+1][col] == piece &&
            state.board[row+2][col] == piece &&
            state.board[row+3][col] == piece) {
          return true;
        }
      }
    }
    
    // Diagonal check (top-left to bottom-right)
    for (int row = 0; row <= ROWS - 4; row++) {
      for (int col = 0; col <= COLS - 4; col++) {
        if (state.board[row][col] == piece &&
            state.board[row+1][col+1] == piece &&
            state.board[row+2][col+2] == piece &&
            state.board[row+3][col+3] == piece) {
          return true;
        }
      }
    }
    
    // Diagonal check (top-right to bottom-left)
    for (int row = 0; row <= ROWS - 4; row++) {
      for (int col = 3; col < COLS; col++) {
        if (state.board[row][col] == piece &&
            state.board[row+1][col-1] == piece &&
            state.board[row+2][col-2] == piece &&
            state.board[row+3][col-3] == piece) {
          return true;
        }
      }
    }
    
    return false;
  }
  
  // Evaluate board position from current player's perspective
  int evaluate(const ConnectFourState& state) override {
    // Check for terminal states first (wins)
    if (hasWin(state, RED)) {
      return state.blueTurn ? 10000 : -10000; // Perspective of current player
    }
    if (hasWin(state, BLUE)) {
      return state.blueTurn ? -10000 : 10000; // Perspective of current player
    }
    
    int score = 0;
    
    // Evaluate potential threats and opportunities
    // For each cell, check how many pieces are in a row in each direction
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (state.board[row][col] != EMPTY) {
          continue; // Skip filled cells
        }
        
        // Create a temporary copy of state to modify
        ConnectFourState tempState = state;
        
        // Check potential for RED
        tempState.board[row][col] = RED;
        if (checkWin(tempState, row, col, RED)) {
          score -= 100; // Potential win for RED
        }
        
        // Check potential for BLUE
        tempState.board[row][col] = BLUE;
        if (checkWin(tempState, row, col, BLUE)) {
          score += 100; // Potential win for BLUE
        }
      }
    }
    
    // Favor center columns for better control
    for (int row = 0; row < ROWS; row++) {
      for (int col = 0; col < COLS; col++) {
        if (state.board[row][col] == RED) {
          // Penalize RED pieces (from BLUE's perspective)
          // Value center columns more
          score -= 3 * (COLS - abs(col - COLS/2));
        } else if (state.board[row][col] == BLUE) {
          // Reward BLUE pieces (from BLUE's perspective)
          // Value center columns more
          score += 3 * (COLS - abs(col - COLS/2));
        }
      }
    }
    
    // Invert score if it's red's turn (adjust for perspective)
    return state.blueTurn ? score : -score;
  }
  
  // Generate all valid moves from the current state
  int generateMoves(const ConnectFourState& state, ConnectFourMove moves[], int maxMoves) override {
    int moveCount = 0;
    
    // A move is valid if the column is not full
    for (int col = 0; col < COLS && moveCount < maxMoves; col++) {
      if (findDropRow(state, col) >= 0) {
        moves[moveCount] = ConnectFourMove(col);
        moveCount++;
      }
    }
    
    return moveCount;
  }
  
  // Apply a move to a state, modifying the state
  void applyMove(ConnectFourState& state, const ConnectFourMove& move) override {
    // Find the lowest empty row in the selected column
    int row = findDropRow(state, move.column);
    
    if (row >= 0) {
      // Place the piece
      state.board[row][move.column] = state.blueTurn ? BLUE : RED;
      
      // Switch turns
      state.blueTurn = !state.blueTurn;
    }
  }
  
  // Check if the game has reached a terminal state (win/loss/draw)
  bool isTerminal(const ConnectFourState& state) override {
    // Check if either player has won
    if (hasWin(state, RED) || hasWin(state, BLUE)) {
      return true;
    }
    
    // Check for a draw (board is full)
    for (int col = 0; col < COLS; col++) {
      if (findDropRow(state, col) >= 0) {
        return false; // There's still at least one valid move
      }
    }
    
    return true; // Board is full, it's a draw
  }
  
  // Check if the current player is the maximizing player
  bool isMaximizingPlayer(const ConnectFourState& state) override {
    // BLUE is the maximizing player (AI)
    return state.blueTurn;
  }
};

// Global variables
ConnectFourState gameState;
ConnectFourLogic gameLogic;
Minimax<ConnectFourState, ConnectFourMove, MAX_MOVES, MINIMAX_DEPTH> minimaxAI(gameLogic);

int gameMode = MODE_HUMAN_VS_AI;  // Default to Human vs AI

// Function to display the board with emoji symbols
void displayBoard(const ConnectFourState& state) {
  // Column numbers with emoji numbers for consistent spacing
  Serial.println("\n 0️⃣ 1️⃣ 2️⃣ 3️⃣ 4️⃣ 5️⃣ 6️⃣");
  
  for (int row = 0; row < ROWS; row++) {
    Serial.print(" ");
    
    for (int col = 0; col < COLS; col++) {
      switch (state.board[row][col]) {
        case EMPTY:
          Serial.print("⚪"); // White circle for empty
          break;
        case RED:
          Serial.print("🔴"); // Red circle
          break;
        case BLUE:
          Serial.print("🔵"); // Blue circle
          break;
      }
      Serial.print(" ");
    }
    
    Serial.println();
  }
  
  // Display column numbers again at the bottom with emoji numbers
  Serial.println(" 0️⃣ 1️⃣ 2️⃣ 3️⃣ 4️⃣ 5️⃣ 6️⃣");
  Serial.print(state.blueTurn ? "Blue's turn" : "Red's turn");
  Serial.println();
}

// Function to get a move from human player
ConnectFourMove getHumanMove() {
  ConnectFourMove move;
  bool validMove = false;
  
  while (!validMove) {
    // Prompt for input
    Serial.println("Enter column (0-6):");
    
    // Wait for input
    while (!Serial.available()) {
      delay(100);
    }
    
    // Read the column
    move.column = Serial.parseInt();
    
    // Clear the input buffer
    while (Serial.available()) {
      Serial.read();
    }
    
    // Check if the column is valid
    if (move.column < COLS) {
      // Check if the column is not full
      if (gameLogic.findDropRow(gameState, move.column) >= 0) {
        validMove = true;
      } else {
        Serial.println("Column is full. Try another one.");
      }
    } else {
      Serial.println("Invalid column. Please enter a number between 0 and 6.");
    }
  }
  
  return move;
}

// Function to get AI move
ConnectFourMove getAIMove() {
  Serial.println("AI is thinking...");
  
  unsigned long startTime = millis();
  ConnectFourMove move = minimaxAI.findBestMove(gameState);
  unsigned long endTime = millis();
  
  Serial.print("AI chose column: ");
  Serial.println(move.column);
  
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
    if (gameLogic.hasWin(gameState, RED)) {
      Serial.println("Red wins!");
    } else if (gameLogic.hasWin(gameState, BLUE)) {
      Serial.println("Blue wins!");
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
  
  Serial.println("\n=== CONNECT FOUR ===");
  Serial.println("Game Modes:");
  Serial.println("1. Human (Red) vs. AI (Blue)");
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
    Serial.println("You play as Red, AI plays as Blue.");
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
  
  // Get and apply move based on game mode and current player
  ConnectFourMove move;
  
  if (gameMode == MODE_HUMAN_VS_AI) {
    if (!gameState.blueTurn) {
      // Human's turn (Red)
      move = getHumanMove();
    } else {
      // AI's turn (Blue)
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