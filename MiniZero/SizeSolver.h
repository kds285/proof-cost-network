#pragma once

#include "Configure.h"
#include <iostream>
#include <algorithm>

/*
   Both-side pruning version. 
   Or node pruning: alpha-beta.
   And node pruning: math (Pigeonhole principle).
 */

#define SIZE_SOLVER_PRINT_DEBUG true

class SizeSolver {
private:
  int problemSize;

public:
  SizeSolver(int _problemSize): problemSize(_problemSize) { }

  void solveLossMoves(const Game& g) {
    const Color player = g.getTurnColor();
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      Game dupGame = g;
      dupGame.play(move);
      //assuming a lossing move is made.
      pair<int, int> subtreeInfo = findWinTreeSize(dupGame, problemSize);
      subtreeInfo.first = -subtreeInfo.first;
      cerr << move.toSgfString(Game::getBoardSize()) << ":" << subtreeInfo << endl;
    }
  }

  void solveWinMoves(const Game& g) {
    const Color player = g.getTurnColor();
    for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      Game dupGame = g;
      dupGame.play(move);
      //assuming a winning move is made.
      pair<int, int> subtreeInfo = findLossTreeSize(dupGame, problemSize);
      subtreeInfo.first = -subtreeInfo.first;
      cerr << move.toSgfString(Game::getBoardSize()) << ":" << subtreeInfo << endl;
    }
  }

private:
  //Find the minimum win tree. Try all possible moves, whether a winning move is found or not.
  pair<int, int> findWinTreeSize(Game& g, int sizeLimit) {
    if (sizeLimit <=0) return make_pair(0, 0);

    const Color player = g.getTurnColor();
    if(g.isTerminal()) {
      if(g.eval() == player) {
        return make_pair(1, 1);
      }
      else if(g.eval() == COLOR_NONE) {
        return make_pair(0, 1);
      }
      else {
        return make_pair(-1, 1);
      }
    }

    int bestOutcome = -1;
    int treeSize = 1;
    int subtreeSizeLimit = sizeLimit - 1;
    int winSubtreeSize = -1;//default value: "-1" means undefined 
    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      pair<int, int> subtreeInfo = findLossTreeSize(g, subtreeSizeLimit);
      g.undo();
      subtreeInfo.first = -subtreeInfo.first;
      if(subtreeInfo.first == 1){
        if(bestOutcome == 1){
          winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
        }
        else{
          winSubtreeSize = subtreeInfo.second;
        }
        subtreeSizeLimit = winSubtreeSize - 1;
      }
      treeSize += subtreeInfo.second;
      bestOutcome = max(bestOutcome, subtreeInfo.first);
    }

    if(bestOutcome == 1) {
      return make_pair(1, 1 + winSubtreeSize);
    }
    else {
      return make_pair(bestOutcome, treeSize);
    }
  }

  //Find the minimum loss tree. Stop if a winning move or unsolvable move is found.
  pair<int, int> findLossTreeSize(Game& g, int sizeLimit) {
    if (sizeLimit <=0) return make_pair(0, 0);

    const Color player = g.getTurnColor();
    if(g.isTerminal()) {
      if(g.eval() == player) {
        return make_pair(1, 1);
      }
      else if(g.eval() == COLOR_NONE) {
        return make_pair(0, 1);
      }
      else {
        return make_pair(-1, 1);
      }
    }

    int nLegalAction = 0;
    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(g.isLegalMove(move)) {
        nLegalAction++;
      }
    }

    if((sizeLimit - 1) / nLegalAction == 0) {
      return make_pair(0, 0);
    }

    bool passMinBranchTest = false;
    bool passTable[300] = {0};
    int treeSize = 1;
    int passCount = 0;
    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      g.play(move);
      pair<int, int> subtreeInfo = findWinTreeSize(g, (sizeLimit - 1) / nLegalAction);
      g.undo();
      subtreeInfo.first = -subtreeInfo.first;
      if(subtreeInfo.first == -1) {
        passMinBranchTest = true;
        passTable[i] = true;
        treeSize += subtreeInfo.second;
        passCount++;
      }
    }

    if(!passMinBranchTest){
      return make_pair(0, 0);
    }
#if SIZE_SOLVER_PRINT_DEBUG
    cerr << "passMinBranchTest, limit = " << sizeLimit << ", used = " 
      << treeSize << ", unknown branches = " << (nLegalAction - passCount) << endl;
#endif
    for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
      Move move = Move(player, i);
      if(!g.isLegalMove(move)) {
        continue;
      }
      if(passTable[i]){
        continue;
      }
      g.play(move);
      pair<int, int> subtreeInfo = findWinTreeSize(g, sizeLimit - treeSize);
      g.undo();
      subtreeInfo.first = -subtreeInfo.first;
      if(subtreeInfo.first == -1){
        treeSize += subtreeInfo.second;
      }
      else if(subtreeInfo.first == 1){
        return make_pair(1, 1 + subtreeInfo.second);
      }
      else{
        return make_pair(0, 0);
      }
    }

    return make_pair(-1, treeSize);
  }

};


/*
//brute-force version
pair<int, int> findTreeSize(Game& g) {
const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int bestOutcome = -1;
int treeSize = 1;
int winSubtreeSize = -1;//default value: "-1" means undefined 
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
Move move = Move(player, i);
if(!g.isLegalMove(move)) {
continue;
}
Game dupGame = g;
dupGame.play(move);
pair<int, int> subtreeInfo = findTreeSize(dupGame);
subtreeInfo.first = -subtreeInfo.first;
if(subtreeInfo.first == 1){
if(bestOutcome == 1){
winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
}
else{
winSubtreeSize = subtreeInfo.second;
}
}
treeSize += subtreeInfo.second;
bestOutcome = max(bestOutcome, subtreeInfo.first);
}

if(bestOutcome == 1) {
return make_pair(1, 1 + winSubtreeSize);
}
else {
return make_pair(bestOutcome, treeSize);
}
}
void solve(Game& g) {
// TODO: use bfs or dfs to solve the game
const Color player = g.getTurnColor();
for (int i = 0; i < Game::getMaxNumLegalAction(); i++) {
Move move = Move(player, i);
if(!g.isLegalMove(move)) {
continue;
}
Game dupGame = g;
dupGame.play(move);
pair<int, int> subtreeInfo = findTreeSize(dupGame);
subtreeInfo.first = -subtreeInfo.first;
cerr << move.toSgfString(Game::getBoardSize()) << ":" << subtreeInfo << endl;
}
}
 */

/*
//proto-type for and-tree side pruning version. 
pair<int, int> findWinTreeSize(Game& g, int sizeLimit);
pair<int, int> findLossTreeSize(Game& g, int sizeLimit);

//Find the minimun win tree. Try all possible moves, whether a winning move is found or not.
pair<int, int> findWinTreeSize(Game& g, int sizeLimit) {
if (sizeLimit <=0) return make_pair(0, 0);

const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int bestOutcome = -1;
int treeSize = 1;
int winSubtreeSize = -1;//default value: "-1" means undefined 
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
Move move = Move(player, i);
if(!g.isLegalMove(move)) {
continue;
}
Game dupGame = g;
dupGame.play(move);
pair<int, int> subtreeInfo = findLossTreeSize(dupGame, sizeLimit - 1);
subtreeInfo.first = -subtreeInfo.first;
if(subtreeInfo.first == 1){
if(bestOutcome == 1){
winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
}
else{
winSubtreeSize = subtreeInfo.second;
}
}
treeSize += subtreeInfo.second;
bestOutcome = max(bestOutcome, subtreeInfo.first);
}

if(bestOutcome == 1) {
return make_pair(1, 1 + winSubtreeSize);
}
else {
return make_pair(bestOutcome, treeSize);
}
}

//Find the minimun loss tree. Stop if a winning move or unsolvable move is found.
pair<int, int> findLossTreeSize(Game& g, int sizeLimit) {
if (sizeLimit <=0) return make_pair(0, 0);

const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int bestOutcome = -1;
int treeSize = 1;
int winSubtreeSize = -1;//default value: "-1" means undefined 
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(!g.isLegalMove(move)) {
    continue;
  }
  Game dupGame = g;
  dupGame.play(move);
  pair<int, int> subtreeInfo = findWinTreeSize(dupGame, sizeLimit - treeSize);
  subtreeInfo.first = -subtreeInfo.first;
  if(subtreeInfo.first == 1){
    if(bestOutcome == 1){
      winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
    }
    else{
      winSubtreeSize = subtreeInfo.second;
    }
  }
  treeSize += subtreeInfo.second;
  bestOutcome = max(bestOutcome, subtreeInfo.first);
  if(bestOutcome != -1) return make_pair(0, 0);
}

if(bestOutcome == 1) {
  return make_pair(1, 1 + winSubtreeSize);
}
else {
  return make_pair(bestOutcome, treeSize);
}
}
*/
/*
//proto-type for math-based pruning version. 
pair<int, int> findWinTreeSize(Game& g, int sizeLimit);
pair<int, int> findLossTreeSize(Game& g, int sizeLimit);

//Find the minimun win tree. Try all possible moves, whether a winning move is found or not.
pair<int, int> findWinTreeSize(Game& g, int sizeLimit) {
if (sizeLimit <=0) return make_pair(0, 0);

const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int bestOutcome = -1;
int treeSize = 1;
int winSubtreeSize = -1;//default value: "-1" means undefined 
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
Move move = Move(player, i);
if(!g.isLegalMove(move)) {
continue;
}
Game dupGame = g;
dupGame.play(move);
pair<int, int> subtreeInfo = findLossTreeSize(dupGame, sizeLimit - 1);
subtreeInfo.first = -subtreeInfo.first;
if(subtreeInfo.first == 1){
if(bestOutcome == 1){
winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
}
else{
winSubtreeSize = subtreeInfo.second;
}
}
treeSize += subtreeInfo.second;
bestOutcome = max(bestOutcome, subtreeInfo.first);
}

if(bestOutcome == 1) {
return make_pair(1, 1 + winSubtreeSize);
}
else {
return make_pair(bestOutcome, treeSize);
}
}

//Find the minimun loss tree. Stop if a winning move or unsolvable move is found.
pair<int, int> findLossTreeSize(Game& g, int sizeLimit) {
if (sizeLimit <=0) return make_pair(0, 0);

const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int nLegalAction = 0;
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(g.isLegalMove(move)) {
    nLegalAction++;
  }
}

if((sizeLimit - 1) / nLegalAction == 0) {
  return make_pair(0, 0);
}

bool passMinBranchTest = false;
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(!g.isLegalMove(move)) {
    continue;
  }
  Game dupGame = g;
  dupGame.play(move);
  pair<int, int> subtreeInfo = findWinTreeSize(dupGame, (sizeLimit - 1) / nLegalAction);
  subtreeInfo.first = -subtreeInfo.first;
  if(subtreeInfo.first == -1){
    passMinBranchTest = true;
  }
}

if(!passMinBranchTest){
  return make_pair(0, 0);
}
cerr << "passMinBranchTest" << endl;

int bestOutcome = -1;
int treeSize = 1;
int winSubtreeSize = -1;//default value: "-1" means undefined 
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(!g.isLegalMove(move)) {
    continue;
  }
  Game dupGame = g;
  dupGame.play(move);
  pair<int, int> subtreeInfo = findWinTreeSize(dupGame, sizeLimit - treeSize);
  subtreeInfo.first = -subtreeInfo.first;
  if(subtreeInfo.first == 1){
    if(bestOutcome == 1){
      winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
    }
    else{
      winSubtreeSize = subtreeInfo.second;
    }
  }
  treeSize += subtreeInfo.second;
  bestOutcome = max(bestOutcome, subtreeInfo.first);
  if(bestOutcome != -1) return make_pair(0, 0);
}

if(bestOutcome == 1) {
  return make_pair(1, 1 + winSubtreeSize);
}
else {
  return make_pair(bestOutcome, treeSize);
}
}
*/

/*
//proto-type for math-based pruning + undo-based search version. 
pair<int, int> findWinTreeSize(Game& g, int sizeLimit);
pair<int, int> findLossTreeSize(Game& g, int sizeLimit);

//Find the minimun win tree. Try all possible moves, whether a winning move is found or not.
pair<int, int> findWinTreeSize(Game& g, int sizeLimit) {
if (sizeLimit <=0) return make_pair(0, 0);

const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int bestOutcome = -1;
int treeSize = 1;
int winSubtreeSize = -1;//default value: "-1" means undefined 
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
Move move = Move(player, i);
if(!g.isLegalMove(move)) {
continue;
}
g.play(move);
pair<int, int> subtreeInfo = findLossTreeSize(g, sizeLimit - 1);
g.undo();
subtreeInfo.first = -subtreeInfo.first;
if(subtreeInfo.first == 1){
if(bestOutcome == 1){
winSubtreeSize = min(winSubtreeSize, subtreeInfo.second);
}
else{
winSubtreeSize = subtreeInfo.second;
}
}
treeSize += subtreeInfo.second;
bestOutcome = max(bestOutcome, subtreeInfo.first);
}

if(bestOutcome == 1) {
return make_pair(1, 1 + winSubtreeSize);
}
else {
return make_pair(bestOutcome, treeSize);
}
}

//Find the minimun loss tree. Stop if a winning move or unsolvable move is found.
pair<int, int> findLossTreeSize(Game& g, int sizeLimit) {
if (sizeLimit <=0) return make_pair(0, 0);

const Color player = g.getTurnColor();
if(g.isTerminal()) {
if(g.eval() == player) {
return make_pair(1, 1);
}
else if(g.eval() == COLOR_NONE) {
return make_pair(0, 1);
}
else {
return make_pair(-1, 1);
}
}

int nLegalAction = 0;
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(g.isLegalMove(move)) {
    nLegalAction++;
  }
}

if((sizeLimit - 1) / nLegalAction == 0) {
  return make_pair(0, 0);
}

bool passMinBranchTest = false;
bool passTable[300] = {0};
int treeSize = 1;
for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(!g.isLegalMove(move)) {
    continue;
  }
  g.play(move);
  pair<int, int> subtreeInfo = findWinTreeSize(g, (sizeLimit - 1) / nLegalAction);
  g.undo();
  subtreeInfo.first = -subtreeInfo.first;
  if(subtreeInfo.first == -1) {
    passMinBranchTest = true;
    passTable[i] = true;
    treeSize += subtreeInfo.second;
  }
}

if(!passMinBranchTest){
  return make_pair(0, 0);
}
cerr << "passMinBranchTest, limit = " << sizeLimit << ", used = " << treeSize << endl;

for (int i = 0; i < Game::getMaxNumLegalAction(); ++i) {
  Move move = Move(player, i);
  if(!g.isLegalMove(move)) {
    continue;
  }
  if(passTable[i]){
    continue;
  }
  g.play(move);
  pair<int, int> subtreeInfo = findWinTreeSize(g, sizeLimit - treeSize);
  g.undo();
  subtreeInfo.first = -subtreeInfo.first;
  if(subtreeInfo.first == -1){
    treeSize += subtreeInfo.second;
  }
  else if(subtreeInfo.first == 1){
    return make_pair(1, 1 + subtreeInfo.second);
  }
  else{
    return make_pair(0, 0);
  }
}

return make_pair(-1, treeSize);
}
*/

pair<int, int> findWinTreeSize(Game& g, int sizeLimit);
pair<int, int> findLossTreeSize(Game& g, int sizeLimit);

