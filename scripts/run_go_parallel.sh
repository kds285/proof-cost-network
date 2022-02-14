#!/bin/bash

GPU_NUMS=`nvidia-smi -L | wc -l`
RUN_NUMS=$GPU_NUMS
PB=/workspace/problems/9x9Killall-Go
ANS=/workspace/result/9x9Killall-Go/MCTS
ANS_FDFPN=/workspace/result/9x9Killall-Go/FDFPN

if [ "$GPU_NUMS" -lt 1 ]; then
    echo "No GPU"
    exit 1
fi

if ! command -v tmux &> /dev/null ; then
	echo "install tmux"
	apt update ; apt install -y tmux
fi

if [ -z "${TMUX}" ]; then
    tmux new-session -n parallel_fight \
		"tmux set-option -g history-limit 1000000 ; tmux set-option -g mouse on ; $0 $@"
    exit 0
fi

if [ ! -f "CMakeCache.txt" ] || [ -f "CMakeCache.txt" ] && [ -z `grep -e 'GAME_TYPE:UNINITIALIZED=GO$' CMakeCache.txt` ]; then
    echo "compiling... MiniZero GO"
    ./scripts/clean-up.sh
    ./scripts/setup-cmake.sh GO release
    make -j
fi

if [ ! -d "$ANS" ]; then
    echo "create $ANS"
    mkdir -p $ANS
fi

if [ ! -d "$ANS_FDFPN" ]; then
    echo "create $ANS_FDFPN"
    mkdir -p $ANS_FDFPN
fi

create_window () {
    tmux new-window -n run${1} "bash"
}

run_cmd () {
    tmux send-keys -t run${1} "${2}" Enter
}

has_memory() {
    MEM_FREE=`free -g | grep Mem | awk '{print $7}'`
    if [ "$MEM_FREE" -lt ${1} ]; then
        echo "No enough free memory. Need at least ${1} GB. Find $MEM_FREE GB only."
        return 1
    else
        return 0
    fi
}

tmux new-window -n result_MCTS "bash"
tmux send-keys -t result_MCTS "watch \"/workspace/scripts/display_solver_results_go.sh $PB $ANS f\"" Enter
tmux new-window -n result_FDFPN "bash"
tmux send-keys -t result_FDFPN "watch \"/workspace/scripts/display_solver_results_go.sh $PB $ANS_FDFPN f\"" Enter
for i in `seq 0 $(($RUN_NUMS - 1))`; do
    create_window $i
done

for i in `seq 0 $(($RUN_NUMS - 1))`; do
    g=$(($i % $GPU_NUMS))
    if has_memory 90; then run_cmd $i "/workspace/scripts/solve.sh go noNetwork $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go AZ $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go W-max $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go W-heur $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go B-max $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go B-heur $PB $ANS $g mcts_solver"; fi

    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go noNetwork $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go AZ $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go W-max $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go W-heur $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go B-max $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh go B-heur $PB $ANS_FDFPN $g dfpn_solver"; fi
done
