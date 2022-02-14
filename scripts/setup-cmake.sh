#!/bin/bash
set -e

usage() {
	echo "Usage: setup-cmake.sh [TIETACTOE|GOMOKU|HEX|GO] [debug|release]"
	exit 1
}

if [ $# -eq 2 ]; then
	gameType=$1
	if [ "${gameType}" != "TIETACTOE" ] && [ "${gameType}" != "GOMOKU" ] && [ "${gameType}" != "HEX" ] && [ "${gameType}" != "GO" ]; then
		usage
	fi
	
	mode=$2
	if [ "${mode}" != "debug" ] && [ "${mode}" != "release" ]; then
		usage
	fi
	
	MODEOPT=
	if [ "${mode}" == "release" ]; then
		MODEOPT="-DCMAKE_BUILD_TYPE=Release"
	fi
	
	cmake -DCMAKE_PREFIX_PATH=`python -c 'import torch;print(torch.utils.cmake_prefix_path)'` -DGAME_TYPE=${gameType} . ${MODEOPT}
else
	usage
fi
