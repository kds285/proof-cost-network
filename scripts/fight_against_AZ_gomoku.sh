#!/bin/bash

echo "compiling... MiniZero GOMOKU"
/workspace/scripts/clean-up.sh
/workspace/scripts/setup-cmake.sh GOMOKU release
make -j

BOARD_SIZE=15
NUM_FIGHT=1
NUM_GAMES=250
FORCE="force"

function program_str () {
  PROGRAM_STR="/workspace/Release/MiniZero -conf_file /workspace/fight_cfg/$1.cfg"
  echo ${PROGRAM_STR}
}


# =============================Do not change following argument=============================

BLACK_PROGRAM=$(program_str gomoku_max)
WHITE_PROGRAM=$(program_str gomoku_AZ)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} gomoku_max_vs_gomoku_AZ "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
BLACK_PROGRAM=$(program_str gomoku_heur)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} gomoku_heur_vs_gomoku_AZ "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
BLACK_PROGRAM=$(program_str gomoku_max-4T)
WHITE_PROGRAM=$(program_str gomoku_AZ-4T)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} gomoku_max-4T_vs_gomoku_AZ-4T "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
BLACK_PROGRAM=$(program_str gomoku_heur-4T)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} gomoku_heur-4T_vs_gomoku_AZ-4T "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
