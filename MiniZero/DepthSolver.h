#pragma once

#include "Configure.h"
#include <iostream>
#include <algorithm>

/*
   Iterative deepening version. 
   both side pruning: alpha-beta.
   look ahead for finding win moves.
 */

#define DEPTH_SOLVER_PRINT_DEBUG true
#define DEPTH_SOLVER_USING_NETWORK true
#define ONLY_GENERATE_FIRST_SUBPROBLEM true
#if DEPTH_SOLVER_USING_NETWORK
#include "Network.h"
#define DEPTH_FOR_NETWORK 4
#endif

class DepthSolver {
private:
  int problemSize;
#if DEPTH_SOLVER_USING_NETWORK
	Network* m_net;
#endif
  int m_problem_count;
  string m_problem_prefix;

  void reset_problem_setting (string problem_prefix){
    m_problem_count = 0;
    m_problem_prefix = problem_prefix;
  }
public:
#if DEPTH_SOLVER_USING_NETWORK
  DepthSolver(int _problemSize): problemSize(_problemSize), m_net(NULL) { }
#else
  DepthSolver(int _problemSize): problemSize(_problemSize) { }
#endif

  void solveGame(const Game& g, string problem_prefix) {
#if DEPTH_SOLVER_USING_NETWORK
    if(!m_net) {
	    m_net = new Network(Configure::GPU_LIST[0] - '0', Configure::MODEL_FILE);
	    m_net->initialize();
    }
#endif
    reset_problem_setting (problem_prefix);

    vector<int> win_moves;
    int answerDepth = -1;
    for (int i = 0; i <= problemSize; i++) {
      Game dupGame = g;
      win_moves = findWinMoves(dupGame, i);
      if (!win_moves.empty()) {
        answerDepth = i;
        break;
      }
    }

    dumpProblem(g, answerDepth, win_moves);
    
    if (answerDepth == -1) cerr << problem_prefix << "has no answer" << endl;
    const Color player = g.getTurnColor();
    for (int i : win_moves) {
      if (answerDepth == -1) cerr << "but" <<problem_prefix << "try to gen sub-problems!" << endl;
      Move move = Move(player, i);
      Game subGame = g;
      subGame.play(move);
      generateLossSubproblem(subGame, answerDepth - 1);
    }
  }

private:

  void dumpProblem(const Game& g, int answerDepth, vector<int>& best_moves) {
    //generate answer string
    string answerInfo = "depth=" + to_string(answerDepth) + "," + "pos=";
    const Color player = g.getTurnColor();
    for (int position: best_moves) {
      Move move(player, position);
      answerInfo += move.toGtpString(Game::getBoardSize());
      answerInfo += "|";
    }
    answerInfo.pop_back(); // remove the redundant "|"

    //dump to .pb file
    ofstream fout(m_problem_prefix + "_" + to_string(m_problem_count++) + ".pb");
    map <string, string> tag;
    tag["EV"] = answerInfo;
    fout << g.getGameRecord(tag, Game::getBoardSize()) << endl;
  }

  void generateWinSubproblem(Game& g, int answerDepth) {
    if(answerDepth < 0) return;
    const Color player = g.getTurnColor();
    vector<int> win_moves;
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      //assuming winning move is made.
      bool opponent_loss = mustLossWithin(g, answerDepth);
      g.undo();
      if (opponent_loss) win_moves.push_back(i);
    }

    dumpProblem(g, answerDepth, win_moves);

    for (int i: win_moves) {
      Move move = Move(player, i);
      g.play(move);
      generateLossSubproblem(g, answerDepth - 1);
      g.undo();
#if ONLY_GENERATE_FIRST_SUBPROBLEM 
      break;
#endif
    }
  }

  void generateLossSubproblem(Game& g, int answerDepth) {
    if(answerDepth < 0) return;
    const Color player = g.getTurnColor();
    vector<int> non_loss_moves;
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      //assuming loss move is made.
      bool opponent_win = findWinningPath(g, answerDepth - 1);
      g.undo();
      if (!opponent_win) non_loss_moves.push_back(i);
    }

    dumpProblem(g, answerDepth, non_loss_moves);

    for (int i: non_loss_moves) {
      Move move = Move(player, i);
      g.play(move);
      generateWinSubproblem(g, answerDepth - 1);
      g.undo();
#if ONLY_GENERATE_FIRST_SUBPROBLEM 
      break;
#endif
    }
  }

/* original version: batch-size == 1 */

  vector<int> findWinMoves(Game& g, int depthLimit) {
    const Color player = g.getTurnColor();
    vector<int> win_moves;
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      //assuming winning move is made.
      bool opponent_loss =  mustLossWithin(g, depthLimit);
      g.undo();
      if (opponent_loss) win_moves.push_back(i);
    }
    return win_moves;
  }

  int findWinDepth(Game& g, int depthLimit) {
    for (int i = 0; i <= depthLimit; i++) {
      if (findWinningPath(g, i)) return i;
    }
    return -1;
  }

  int findLossDepth(Game& g, int depthLimit) {
    for (int i = 0; i <= depthLimit; i++) {
      if (mustLossWithin(g, i)) return i;
    }
    return -1;
  }

#if DEPTH_SOLVER_USING_NETWORK
  //For iterative deepening.
  //Return true when the first winning move is found
  bool findWinningPath(Game& g, int depthLimit) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) return (g.eval() == player);
    
    if (depthLimit == 0) return false;

    //look ahead
    if (depthLimit > 1) {
      for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, 0);
        g.undo();
        if (opponent_loss) return true;
      }
    }

    if (depthLimit >= DEPTH_FOR_NETWORK) {
      SymmetryType type = static_cast<SymmetryType>(0);
      m_net->set_data(0, g, type);
      m_net->forward();
      vector<pair<Move, float>> vProb = m_net->getProbability(0);

      for (const auto &p : vProb) {
        const Move &move = p.first;
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, depthLimit - 1);
        g.undo();
        if (opponent_loss) return true;
      }

      return false;
    }
    else {
      for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, depthLimit - 1);
        g.undo();
        if (opponent_loss) return true;
      }

      return false;
    }
  }

  //For iterative deepening.
  //Return true if all moves lead to loss.
  bool mustLossWithin(Game& g, int depthLimit) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) {
      if(g.eval() == player) {
        return false;
      }
      else if(g.eval() == COLOR_NONE) {
#if DEPTH_SOLVER_PRINT_DEBUG
        cerr << "winner == COLOR_NONE?" << std::endl;
#endif
        return false;
      }
      else {
        return true;
      }
    }

    if (depthLimit == 0) return false;
    if (depthLimit >= DEPTH_FOR_NETWORK) {
      SymmetryType type = static_cast<SymmetryType>(0);
      m_net->set_data(0, g, type);
      m_net->forward();
      vector<pair<Move, float>> vProb = m_net->getProbability(0);

      for (const auto &p : vProb) {
        const Move &move = p.first;
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_win = findWinningPath(g, depthLimit - 1);
        g.undo();
        if(!opponent_win) return false;
      }

      return true;
    }
    else {
      for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_win = findWinningPath(g, depthLimit - 1);
        g.undo();
        if(!opponent_win) return false;
      }

      return true;
    }
  }

#else
  
  //For iterative deepening.
  //Return true when the first winning move is found
  bool findWinningPath(Game& g, int depthLimit) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) return (g.eval() == player);
    
    if (depthLimit == 0) return false;

    //look ahead
    if (depthLimit > 1) {
      for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, 0);
        g.undo();
        if (opponent_loss) return true;
      }
    }

    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      bool opponent_loss =  mustLossWithin(g, depthLimit - 1);
      g.undo();
      if (opponent_loss) return true;
    }

    return false;
  }

  //For iterative deepening.
  //Return true if all moves lead to loss.
  bool mustLossWithin(Game& g, int depthLimit) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) {
      if(g.eval() == player) {
        return false;
      }
      else if(g.eval() == COLOR_NONE) {
#if DEPTH_SOLVER_PRINT_DEBUG
        cerr << "winner == COLOR_NONE?" << std::endl;
#endif
        return false;
      }
      else {
        return true;
      }
    }
    
    if (depthLimit == 0) return false;

    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      bool opponent_win = findWinningPath(g, depthLimit - 1);
      g.undo();
      if(!opponent_win) return false;
    }

    return true;
  }
#endif

/*
  //For iterative deepening.
  //Return true when the first winning move is found
  bool findWinningPath(Game& g, int depthLimit) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) return (g.eval() == player);
    
    if (depthLimit == 0) return false;

    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      bool opponent_loss =  mustLossWithin(g, depthLimit - 1);
      g.undo();
      if (opponent_loss) return true;
    }

    return false;
  }

  //For iterative deepening.
  //Return true if all moves lead to loss.
  bool mustLossWithin(Game& g, int depthLimit) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) {
      if(g.eval() == player) {
        return false;
      }
      else if(g.eval() == COLOR_NONE) {
#if DEPTH_SOLVER_PRINT_DEBUG
        cerr << "winner == COLOR_NONE?" << std::endl;
#endif
        return false;
      }
      else {
        return true;
      }
    }
    
    if (depthLimit == 0) return false;

    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      bool opponent_win = findWinningPath(g, depthLimit - 1);
      g.undo();
      if(!opponent_win) return false;
    }

    return true;
  }
*/

  /* batch forward version: too ugly, so won't be used before it is refactored.*/
  /*
  vector<int> findWinMoves(Game& g, int depthLimit) {
    const Color player = g.getTurnColor();

    //move ordering for children
    SymmetryType type = static_cast<SymmetryType>(0);
    vector<vector<pair<Move, float>>> vProb_for_child(Game::getMaxNumLegalAction());
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      m_net->set_data(i, g, type);
      g.undo();
    }
    m_net->forward();
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      vProb_for_child[i] = m_net->getProbability(i);
    }

    if (depthLimit >= DEPTH_FOR_NETWORK) {
      vector<int> win_moves;
      for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        //assuming winning move is made.
        bool opponent_loss =  mustLossWithin(g, depthLimit, vProb_for_child[i]);
        g.undo();
        if (opponent_loss) win_moves.push_back(i);
      }
      return win_moves;
    }
    else {
      vector<int> win_moves;
      for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        //assuming winning move is made.
        bool opponent_loss =  mustLossWithin(g, depthLimit);
        g.undo();
        if (opponent_loss) win_moves.push_back(i);
      }
      return win_moves;
    }
  }


  //For iterative deepening.
  //Return true when the first winning move is found
  bool findWinningPath(Game& g, int depthLimit, const vector<pair<Move, float>>& vProb) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) return (g.eval() == player);
    
    if (depthLimit == 0) return false;

    //look ahead
    if (depthLimit > 1) {
      for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, 0);
        g.undo();
        if (opponent_loss) return true;
      }
    }

    if (depthLimit - 1 >= DEPTH_FOR_NETWORK) {
      SymmetryType type = static_cast<SymmetryType>(0);
      vector<vector<pair<Move, float>>> vProb_for_child(Game::getMaxNumLegalAction());
      for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        m_net->set_data(i, g, type);
        g.undo();
      }
      m_net->forward();
      for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        vProb_for_child[i] = m_net->getProbability(i);
      }

      for (const auto &p : vProb) {
        const Move &move = p.first;
        if(!g.isLegalMove(move)) {
          continue;
        }
        int id = move.getPosition();
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, depthLimit - 1, vProb_for_child[id]);
        g.undo();
        if (opponent_loss) return true;
      }

      return false;
    }
    else {
      for (const auto &p : vProb) {
        const Move &move = p.first;
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_loss =  mustLossWithin(g, depthLimit - 1);
        g.undo();
        if (opponent_loss) return true;
      }

      return false;
    }
  }

  //For iterative deepening.
  //Return true if all moves lead to loss.
  bool mustLossWithin(Game& g, int depthLimit, vector<pair<Move, float>>& vProb) {
    if (depthLimit < 0) return false;

    const Color player = g.getTurnColor();
    if(g.isTerminal()) {
      if(g.eval() == player) {
        return false;
      }
      else if(g.eval() == COLOR_NONE) {
#if DEPTH_SOLVER_PRINT_DEBUG
        cerr << "winner == COLOR_NONE?" << std::endl;
#endif
        return false;
      }
      else {
        return true;
      }
    }

    if (depthLimit == 0) return false;
    if (depthLimit - 1 >= DEPTH_FOR_NETWORK) {
      SymmetryType type = static_cast<SymmetryType>(0);
      vector<vector<pair<Move, float>>> vProb_for_child(Game::getMaxNumLegalAction());
      for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        m_net->set_data(i, g, type);
        g.undo();
      }
      m_net->forward();
      for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
        Move move = Move(player, i);
        if(!g.isLegalMove(move)) {
          continue;
        }
        vProb_for_child[i] = m_net->getProbability(i);
      }

      for (const auto &p : vProb) {
        const Move &move = p.first;
        if(!g.isLegalMove(move)) {
          continue;
        }
        int id = move.getPosition();
        g.play(move);
        bool opponent_win = findWinningPath(g, depthLimit - 1, vProb_for_child[id]);
        g.undo();
        if(!opponent_win) return false;
      }

      return true;
    }
    else {
      for (const auto &p : vProb) {
        const Move &move = p.first;
        if(!g.isLegalMove(move)) {
          continue;
        }
        g.play(move);
        bool opponent_win = findWinningPath(g, depthLimit - 1);
        g.undo();
        if(!opponent_win) return false;
      }

      return true;
    }
  }
  */
};
