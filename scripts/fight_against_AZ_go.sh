#!/bin/bash

echo "compiling... MiniZero GO"
/workspace/scripts/clean-up.sh
/workspace/scripts/setup-cmake.sh GO release
make -j

BOARD_SIZE=9
NUM_FIGHT=1
NUM_GAMES=250
FORCE="force"

function program_str () {
  PROGRAM_STR="/workspace/Release/MiniZero -conf_file /workspace/fight_cfg/$1.cfg"
  echo ${PROGRAM_STR}
}

# =============================Do not change following argument=============================

BLACK_PROGRAM=$(program_str go_W-max)
WHITE_PROGRAM=$(program_str go_AZ)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} go_W-max_vs_go_AZ "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
BLACK_PROGRAM=$(program_str go_W-heur)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} go_W-heur_vs_go_AZ "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
BLACK_PROGRAM=$(program_str go_B-max)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} go_B-max_vs_go_AZ "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
BLACK_PROGRAM=$(program_str go_B-heur)
/workspace/scripts/fight.sh ${BOARD_SIZE} ${NUM_FIGHT} ${NUM_GAMES} go_B-heur_vs_go_AZ "${BLACK_PROGRAM}" "${WHITE_PROGRAM}" ${FORCE}
