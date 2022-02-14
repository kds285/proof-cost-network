#!/bin/bash

if [ $# -ne 3 ]; then
	echo "./display_solver_results.sh problem_dir ans_dir show_depth[t|f]"
	exit
fi

PROBLEM_DIR=$1
ANS_DIR=$2
SHOW_DEPTH=$3

# get unique version & print header
# VERSIONS=$(ls ${ANS_DIR}/ 2>/dev/null | sort -V | awk -F "/" '{ print $NF; }' | awk -F "_" '{ print $1; }' | uniq)
VERSIONS='noNetwork AZ W-max W-heur B-max B-heur'
for version in ${VERSIONS}
do
	printf "\t"${version}
done
if [ "${SHOW_DEPTH}" == "t" ]; then
	printf "\tdepth"
fi
printf "\n"

# scan every problem for each version
for problem in $(ls ${PROBLEM_DIR}/ | sort -V)
do
	printf "${problem}"
	for version in ${VERSIONS}
	do
		if compgen -G "${ANS_DIR}/${version}_${problem}*.ans" > /dev/null; then
			ans_file=$(ls ${ANS_DIR}/${version}_${problem}*.ans 2>/dev/null)
			if [ "$(grep "unknown" ${ans_file} | wc -l)" == "1" ]; then
				printf "\t-"
			else
				printf "\t$(cat ${ans_file} | awk '{ print $4; }')"
			fi
		else
			printf "\t"
			continue
		fi
	done
	
	if [ "${SHOW_DEPTH}" == "t" ]; then
		printf "\t$(cat ${PROBLEM_DIR}/${problem} | awk -F "depth=|," '{ print $2 }')"
	fi
	printf "\n"
done
