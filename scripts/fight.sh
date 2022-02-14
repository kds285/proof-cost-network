#!/bin/bash

function usage {
	echo "./fight.sh [BOARD_SIZE] [NUM_FIGHT] [TOTAL_GAMES] [FIGHT_NAME] [BLACK_PROGRAM] [WHITE_PROGRAM] [force/noforce]"
	exit
}

if [ $# -eq 7 ]; then
	BOARD_SIZE=$1
	NUM_FIGHT=$2
	TOTAL_GAMES=$3
	FIGHT_NAME=$4
	BLACK_PROGRAM=$5
	WHITE_PROGRAM=$6
	if [[ "$7" == "force" ]]; then
		FORCE="-force"
	elif [[ "$7" == "noforce" ]]; then
		FORCE=""
	else
      		usage
	fi
else
  usage
fi

# =============================Do not change following argument=============================

# some argument
permissions=$(stat -c %a fight_results)
mkdir fight_results/${FIGHT_NAME}
chmod -R ${permissions} fight_results/${FIGHT_NAME}
SGF_PREFIX_NAME=fight_results/${FIGHT_NAME}/${FIGHT_NAME}

# fight command
# gogui-twogtp documents: https://www.kayufu.com/gogui/reference-twogtp.html
gogui-twogtp -black "${BLACK_PROGRAM}" -white "${WHITE_PROGRAM}" -auto ${FORCE} -games ${TOTAL_GAMES} -sgffile ${SGF_PREFIX_NAME} -size ${BOARD_SIZE} -threads ${NUM_FIGHT} -verbose -alternate
gogui-twogtp -analyze $SGF_PREFIX_NAME.dat -force
chmod -R ${permissions} fight_results/${FIGHT_NAME}
# ==========================================================================================
