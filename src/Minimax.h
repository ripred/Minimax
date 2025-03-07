/**
 * @file Minimax.h
 * @brief A templated Minimax algorithm implementation for Arduino with alpha-beta pruning
 * 
 * This library implements the minimax algorithm for two-player turn-based games
 * while respecting Arduino constraints: 32K flash limit, no STL, and avoiding
 * dynamic memory allocation.
 * 
 * Mar, 2, 2025 ++ tmw
 * 
 */

#ifndef MINIMAX_H
#define MINIMAX_H

#include <Arduino.h>

/**
 * @brief The core Minimax algorithm implementation with alpha-beta pruning
 * 
 * @tparam GameState Type representing the game state (board, positions, etc.)
 * @tparam Move Type representing a valid move in the game
 * @tparam MaxMoves Maximum number of possible moves to consider at any position
 * @tparam MaxDepth Maximum search depth for the algorithm
 */
template <typename GameState, typename Move, int MaxMoves = 64, int MaxDepth = 5>
class Minimax {
public:
    /**
     * @brief Game-specific logic interface that must be implemented by the user
     */
    class GameLogic {
    public:
        /**
         * @brief Evaluate a game state from current player's perspective
         * Higher values indicate better positions for the current player
         */
        virtual int evaluate(const GameState& state) = 0;
        
        /**
         * @brief Generate all valid moves from the current state
         * @return Number of moves generated
         */
        virtual int generateMoves(const GameState& state, Move moves[], int maxMoves) = 0;
        
        /**
         * @brief Apply a move to a state, modifying the state
         */
        virtual void applyMove(GameState& state, const Move& move) = 0;
        
        /**
         * @brief Check if the game has reached a terminal state (win/loss/draw)
         */
        virtual bool isTerminal(const GameState& state) = 0;
        
        /**
         * @brief Check if the current player is the maximizing player
         * Typically alternates between players in turn-based games
         */
        virtual bool isMaximizingPlayer(const GameState& state) = 0;
    };
    
    /**
     * @brief Constructor
     * @param logic Game-specific logic implementation
     */
    Minimax(GameLogic& logic) : _logic(logic), _nodesSearched(0) {}
    
    /**
     * @brief Find the best move for the current game state
     */
    Move findBestMove(const GameState& state) {
        Move bestMove;
        Move moves[MaxMoves];
        int moveCount = _logic.generateMoves(state, moves, MaxMoves);
        
        if (moveCount == 0) {
            return bestMove; // No moves available
        }
        
        bool isMax = _logic.isMaximizingPlayer(state);
        _bestScore = isMax ? -32000 : 32000;
        _nodesSearched = 0;
        
        for (int i = 0; i < moveCount; i++) {
            GameState newState = state;
            _logic.applyMove(newState, moves[i]);
            
            int score = minimax(newState, MaxDepth - 1, -32000, 32000, !isMax);
            
            if (isMax) {
                if (score > _bestScore) {
                    _bestScore = score;
                    bestMove = moves[i];
                }
            } else {
                if (score < _bestScore) {
                    _bestScore = score;
                    bestMove = moves[i];
                }
            }
        }
        
        return bestMove;
    }
    
    /**
     * @brief Get the score of the best move 
     */
    int getBestScore() const { return _bestScore; }
    
    /**
     * @brief Get the number of nodes searched (for performance analysis)
     */
    int getNodesSearched() const { return _nodesSearched; }
    
private:
    GameLogic& _logic;
    int _bestScore;
    int _nodesSearched;
    
    /**
     * @brief The minimax algorithm with alpha-beta pruning
     */
    int minimax(const GameState& state, int depth, int alpha, int beta, bool maximizingPlayer) {
        _nodesSearched++;
        
        if (depth == 0 || _logic.isTerminal(state)) {
            return _logic.evaluate(state);
        }
        
        Move moves[MaxMoves];
        int moveCount = _logic.generateMoves(state, moves, MaxMoves);
        
        if (maximizingPlayer) {
            int maxEval = -32000;
            for (int i = 0; i < moveCount; i++) {
                GameState newState = state;
                _logic.applyMove(newState, moves[i]);
                int eval = minimax(newState, depth - 1, alpha, beta, false);
                
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) {
                    break; // Beta cutoff
                }
            }
            return maxEval;
        } else {
            int minEval = 32000;
            for (int i = 0; i < moveCount; i++) {
                GameState newState = state;
                _logic.applyMove(newState, moves[i]);
                int eval = minimax(newState, depth - 1, alpha, beta, true);
                
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) {
                    break; // Alpha cutoff
                }
            }
            return minEval;
        }
    }
};

#endif // MINIMAX_H
