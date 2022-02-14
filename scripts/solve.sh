#!/bin/bash

if [ $# -ne 6 ] && [ $# -ne 7 ]; then
	echo "./run_MCTS_solver.sh <game> <version> <problem_dir> <ans_dir> <gpu> <mode> [cfg]"
	exit
fi

GAME=$1
VERSION=$2
PROBLEM_DIR=$3
ANS_DIR=$4
GPU=$5
MODE=$6
CFG=$7

for problem in ${PROBLEM_DIR}/*; do
	OUTPUT_FILE=".input_"$(tr -dc A-Za-z0-9 </dev/urandom | head -c 13 ; echo '')
	short_problem=$(echo ${problem} | awk -F "/" '{ print $NF; }')
	if [ -f "${ANS_DIR}/${VERSION}_${short_problem}_0.ans" ] || [ -f "${ANS_DIR}/${VERSION}_${short_problem}_150000.ans" ] ; then
		echo "skip ${ANS_DIR}/${VERSION}_${short_problem}"
	else
		echo "output_name ${ANS_DIR}/${VERSION}_${short_problem}" > /tmp/${OUTPUT_FILE}
		echo "solve ${problem}" >> /tmp/${OUTPUT_FILE}
		/workspace/Release/MiniZero -conf_file /workspace/solver_cfg/${GAME}_${VERSION}${CFG}.cfg -conf_str GPU_LIST=${GPU} -mode ${MODE} < /tmp/${OUTPUT_FILE}
		rm /tmp/${OUTPUT_FILE}
	fi
done

