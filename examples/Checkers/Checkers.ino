/**
 * Checkers.ino - Checkers game implementation using Minimax library
 * 
 * This sketch implements a checkers game that can be played:
 * - Human vs. AI
 * - AI vs. AI (self-play)
 * 
 * The game interface uses Serial communication for display and input.
 * 
 * Mar, 2, 2025 ++ tmw
 */

#include "Minimax.h"

// Constants for board representation
#define EMPTY 0
#define WHITE 1
#define BLACK 2
#define WHITE_KING 3
#define BLACK_KING 4

// Game configuration
#define MINIMAX_DEPTH 2      // Search depth for AI
#define MAX_MOVES 40         // Maximum possible moves for one position

// Board size
#define BOARD_SIZE 8

// Game modes
#define MODE_HUMAN_VS_AI 0
#define MODE_AI_VS_AI 1

// Game state - represents the board
struct CheckersState {
  byte board[BOARD_SIZE][BOARD_SIZE];
  bool blackTurn;  // true if it's black's turn, false for white's turn
  
  // Initialize the board with starting position
  void init() {
    blackTurn = false;  // White goes first
    
    // Initialize empty board
    for (int row = 0; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        board[row][col] = EMPTY;
      }
    }
    
    // Set up black pieces (top of board)
    for (int row = 0; row < 3; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        if ((row + col) % 2 == 1) {  // Only on black squares
          board[row][col] = BLACK;
        }
      }
    }
    
    // Set up white pieces (bottom of board)
    for (int row = 5; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        if ((row + col) % 2 == 1) {  // Only on black squares
          board[row][col] = WHITE;
        }
      }
    }
  }
};

// Move structure
struct CheckersMove {
  byte fromRow, fromCol;
  byte toRow, toCol;
  bool isJump;  // true if this move captures a piece
  byte jumpRow, jumpCol;  // position of captured piece if isJump is true
  
  CheckersMove() : fromRow(0), fromCol(0), toRow(0), toCol(0), isJump(false), jumpRow(0), jumpCol(0) {}
  
  CheckersMove(byte fr, byte fc, byte tr, byte tc) 
    : fromRow(fr), fromCol(fc), toRow(tr), toCol(tc), isJump(false), jumpRow(0), jumpCol(0) {
    // Calculate if this is a jump move
    if (abs(tr - fr) == 2) {
      isJump = true;
      jumpRow = (fr + tr) / 2;
      jumpCol = (fc + tc) / 2;
    }
  }
};

// Game logic implementation
class CheckersLogic : public Minimax<CheckersState, CheckersMove, MAX_MOVES, MINIMAX_DEPTH>::GameLogic {
public:
  // Evaluate board position from current player's perspective
  int evaluate(const CheckersState& state) override {
    int score = 0;
    
    // Count material difference (pieces and kings)
    for (int row = 0; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        switch (state.board[row][col]) {
          case WHITE:
            score += 100;
            break;
          case BLACK:
            score -= 100;
            break;
          case WHITE_KING:
            score += 200;
            break;
          case BLACK_KING:
            score -= 200;
            break;
        }
      }
    }
    
    // Positional evaluation (favor advancement and center control)
    for (int row = 0; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        if (state.board[row][col] == WHITE) {
          // Encourage white pieces to advance
          score += (BOARD_SIZE - 1 - row) * 5;
          // Favor center control
          if (col > 1 && col < 6 && row > 1 && row < 6) {
            score += 10;
          }
        } 
        else if (state.board[row][col] == BLACK) {
          // Encourage black pieces to advance
          score -= row * 5;
          // Favor center control
          if (col > 1 && col < 6 && row > 1 && row < 6) {
            score -= 10;
          }
        }
      }
    }
    
    // Invert score if it's black's turn (since we're using perspective of current player)
    return state.blackTurn ? -score : score;
  }
  
  // Generate all valid moves from the current state
  int generateMoves(const CheckersState& state, CheckersMove moves[], int maxMoves) override {
    int moveCount = 0;
    byte player = state.blackTurn ? BLACK : WHITE;
    byte king = state.blackTurn ? BLACK_KING : WHITE_KING;
    
    // Direction of movement (depends on player)
    int forwardDirection = state.blackTurn ? 1 : -1;
    
    // Check if jumps are available
    bool jumpAvailable = false;
    
    // First pass: check for jumps (captures)
    for (int row = 0; row < BOARD_SIZE && moveCount < maxMoves; row++) {
      for (int col = 0; col < BOARD_SIZE && moveCount < maxMoves; col++) {
        if (state.board[row][col] == player || state.board[row][col] == king) {
          // Check all four diagonal directions for jumps
          for (int dRow = -1; dRow <= 1; dRow += 2) {
            for (int dCol = -1; dCol <= 1; dCol += 2) {
              // Regular pieces can only move forward, kings can move any direction
              if (state.board[row][col] == player && dRow != forwardDirection) {
                continue;
              }
              
              // Check if jump is valid
              int jumpRow = row + dRow;
              int jumpCol = col + dCol;
              int landRow = row + 2 * dRow;
              int landCol = col + 2 * dCol;
              
              if (landRow >= 0 && landRow < BOARD_SIZE && landCol >= 0 && landCol < BOARD_SIZE) {
                byte jumpPiece = state.board[jumpRow][jumpCol];
                
                // Can only jump opponent's pieces
                bool isOpponent = false;
                if (state.blackTurn) {
                  isOpponent = (jumpPiece == WHITE || jumpPiece == WHITE_KING);
                } else {
                  isOpponent = (jumpPiece == BLACK || jumpPiece == BLACK_KING);
                }
                
                if (isOpponent && state.board[landRow][landCol] == EMPTY) {
                  moves[moveCount] = CheckersMove(row, col, landRow, landCol);
                  moveCount++;
                  jumpAvailable = true;
                }
              }
            }
          }
        }
      }
    }
    
    // If jumps are available, they are mandatory - return only jumps
    if (jumpAvailable) {
      return moveCount;
    }
    
    // Second pass: if no jumps, consider regular moves
    moveCount = 0;
    for (int row = 0; row < BOARD_SIZE && moveCount < maxMoves; row++) {
      for (int col = 0; col < BOARD_SIZE && moveCount < maxMoves; col++) {
        if (state.board[row][col] == player || state.board[row][col] == king) {
          // Check the two forward diagonal directions for regular moves
          for (int dCol = -1; dCol <= 1; dCol += 2) {
            // Regular pieces can only move forward, kings can move in any direction
            int startDir = (state.board[row][col] == king) ? -1 : forwardDirection;
            int endDir = (state.board[row][col] == king) ? 1 : forwardDirection;
            
            for (int dRow = startDir; dRow <= endDir; dRow += 2) {
              int toRow = row + dRow;
              int toCol = col + dCol;
              
              if (toRow >= 0 && toRow < BOARD_SIZE && toCol >= 0 && toCol < BOARD_SIZE) {
                if (state.board[toRow][toCol] == EMPTY) {
                  moves[moveCount] = CheckersMove(row, col, toRow, toCol);
                  moveCount++;
                }
              }
            }
          }
        }
      }
    }
    
    return moveCount;
  }
  
  // Apply a move to a state, modifying the state
  void applyMove(CheckersState& state, const CheckersMove& move) override {
    // Move the piece
    byte piece = state.board[move.fromRow][move.fromCol];
    state.board[move.fromRow][move.fromCol] = EMPTY;
    state.board[move.toRow][move.toCol] = piece;
    
    // If this is a jump, remove the captured piece
    if (move.isJump) {
      state.board[move.jumpRow][move.jumpCol] = EMPTY;
    }
    
    // Check for promotion to king
    if (piece == WHITE && move.toRow == 0) {
      state.board[move.toRow][move.toCol] = WHITE_KING;
    } else if (piece == BLACK && move.toRow == BOARD_SIZE - 1) {
      state.board[move.toRow][move.toCol] = BLACK_KING;
    }
    
    // Switch turns
    state.blackTurn = !state.blackTurn;
  }
  
  // Check if the game has reached a terminal state (win/loss/draw)
  bool isTerminal(const CheckersState& state) override {
    // Check if any moves are available for the current player
    CheckersMove moves[MAX_MOVES];
    int moveCount = generateMoves(state, moves, MAX_MOVES);
    
    if (moveCount == 0) {
      return true; // No moves available, game over
    }
    
    // Check for piece count
    int whitePieces = 0;
    int blackPieces = 0;
    
    for (int row = 0; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        if (state.board[row][col] == WHITE || state.board[row][col] == WHITE_KING) {
          whitePieces++;
        } else if (state.board[row][col] == BLACK || state.board[row][col] == BLACK_KING) {
          blackPieces++;
        }
      }
    }
    
    if (whitePieces == 0 || blackPieces == 0) {
      return true; // One player has no pieces left
    }
    
    return false;
  }
  
  // Check if the current player is the maximizing player
  bool isMaximizingPlayer(const CheckersState& state) override {
    // White is maximizing player
    return !state.blackTurn;
  }
};

// Global variables
CheckersState gameState;
CheckersLogic gameLogic;
Minimax<CheckersState, CheckersMove, MAX_MOVES, MINIMAX_DEPTH> minimaxAI(gameLogic);

int gameMode = MODE_HUMAN_VS_AI;  // Default to Human vs AI

// Function to display the board
void displayBoard(const CheckersState& state) {
  Serial.println("\n    0  1  2  3  4  5  6  7 ");
  Serial.println("  +------------------------+");
  
  for (int row = 0; row < BOARD_SIZE; row++) {
    Serial.print(row);
    Serial.print(" |");
    
    for (int col = 0; col < BOARD_SIZE; col++) {
      switch (state.board[row][col]) {
        case EMPTY:
          // Use 3-character width consistently
          Serial.print((row + col) % 2 == 0 ? " . " : "   ");
          break;
        case WHITE:
          Serial.print(" w ");
          break;
        case BLACK:
          Serial.print(" b ");
          break;
        case WHITE_KING:
          Serial.print(" W ");
          break;
        case BLACK_KING:
          Serial.print(" B ");
          break;
      }
    }
    
    Serial.println("|");
  }
  
  Serial.println("  +------------------------+");
  Serial.print(state.blackTurn ? "Black's turn" : "White's turn");
  Serial.println();
}

// Function to get a move from human player
CheckersMove getHumanMove() {
  CheckersMove move;
  bool validMove = false;
  
  while (!validMove) {
    // Prompt for input
    Serial.println("Enter your move (fromRow fromCol toRow toCol):");
    
    // Wait for input
    while (!Serial.available()) {
      delay(100);
    }
    
    // Read the move
    move.fromRow = Serial.parseInt();
    move.fromCol = Serial.parseInt();
    move.toRow = Serial.parseInt();
    move.toCol = Serial.parseInt();
    
    // Clear the input buffer
    while (Serial.available()) {
      Serial.read();
    }
    
    // Calculate jump information
    if (abs(move.toRow - move.fromRow) == 2) {
      move.isJump = true;
      move.jumpRow = (move.fromRow + move.toRow) / 2;
      move.jumpCol = (move.fromCol + move.toCol) / 2;
    }
    
    // Validate move
    CheckersMove moves[MAX_MOVES];
    int moveCount = gameLogic.generateMoves(gameState, moves, MAX_MOVES);
    
    for (int i = 0; i < moveCount; i++) {
      CheckersMove &m = moves[i];
      if (m.fromRow == move.fromRow && m.fromCol == move.fromCol && 
          m.toRow == move.toRow && m.toCol == move.toCol) {
        validMove = true;
        break;
      }
    }
    
    if (!validMove) {
      Serial.println("Invalid move. Try again.");
    }
  }
  
  return move;
}

// Function to get AI move
CheckersMove getAIMove() {
  Serial.println("AI is thinking...");
  
  unsigned long startTime = millis();
  CheckersMove move = minimaxAI.findBestMove(gameState);
  unsigned long endTime = millis();
  
  Serial.print("AI move: ");
  Serial.print(move.fromRow);
  Serial.print(",");
  Serial.print(move.fromCol);
  Serial.print(" to ");
  Serial.print(move.toRow);
  Serial.print(",");
  Serial.println(move.toCol);
  
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
    
    // Count pieces to determine winner
    int whitePieces = 0;
    int blackPieces = 0;
    
    for (int row = 0; row < BOARD_SIZE; row++) {
      for (int col = 0; col < BOARD_SIZE; col++) {
        if (gameState.board[row][col] == WHITE || gameState.board[row][col] == WHITE_KING) {
          whitePieces++;
        } else if (gameState.board[row][col] == BLACK || gameState.board[row][col] == BLACK_KING) {
          blackPieces++;
        }
      }
    }
    
    if (whitePieces > blackPieces) {
      Serial.println("White wins!");
    } else if (blackPieces > whitePieces) {
      Serial.println("Black wins!");
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
  
  Serial.println("\n=== CHECKERS GAME ===");
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
  CheckersMove move;
  
  if (gameMode == MODE_HUMAN_VS_AI) {
    if (gameState.blackTurn) {
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