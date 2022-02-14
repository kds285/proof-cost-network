#!/bin/bash

if [ $# -ne 6 ]; then
	echo "./run_MCTS_solver.sh <game> <version> <problem_dir> <ans_dir> <gpu> <mode>"
	exit
fi

GAME=$1
VERSION=$2
PROBLEM_DIR=$3
ANS_DIR=$4
GPU=$5
MODE=$6
OUTPUT_FILE=".input_"$(tr -dc A-Za-z0-9 </dev/urandom | head -c 13 ; echo '')

for problem in ${PROBLEM_DIR}/*; do
	short_problem=$(echo ${problem} | awk -F "/" '{ print $NF; }')
	echo "output_name ${ANS_DIR}/${VERSION}_${short_problem}"
	echo "solve ${problem}"
done > /tmp/${OUTPUT_FILE}

/workspace/Release/MiniZero -conf_file /workspace/solver_cfg/${GAME}_${VERSION}.cfg -conf_str GPU_LIST=${GPU} -mode ${MODE} < /tmp/${OUTPUT_FILE}
rm /tmp/${OUTPUT_FILE}
