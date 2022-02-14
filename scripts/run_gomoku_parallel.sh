#!/bin/bash

GPU_NUMS=`nvidia-smi -L | wc -l`
RUN_NUMS=$GPU_NUMS
PB=/workspace/problems/Gomoku
ANS=/workspace/result/Gomoku/MCTS
ANS_FDFPN=/workspace/result/Gomoku/FDFPN

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

if [ ! -f "CMakeCache.txt" ] || [ -f "CMakeCache.txt" ] && [ -z `grep -e 'GAME_TYPE:UNINITIALIZED=GOMOKU$' CMakeCache.txt` ]; then
    echo "compiling... MiniZero GOMOKU"
    ./scripts/clean-up.sh
    ./scripts/setup-cmake.sh GOMOKU release
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
tmux send-keys -t result_MCTS "watch \"/workspace/scripts/display_solver_results_gomoku.sh $PB $ANS f\"" Enter
tmux new-window -n result_FDFPN "bash"
tmux send-keys -t result_FDFPN "watch \"/workspace/scripts/display_solver_results_gomoku.sh $PB $ANS_FDFPN f\"" Enter


for i in `seq 0 $(($RUN_NUMS - 1))`; do
    create_window $i
done

for i in `seq 0 $(($RUN_NUMS - 1))`; do
    g=$(($i % $GPU_NUMS))
    if has_memory 500; then run_cmd $i "/workspace/scripts/solve.sh gomoku noNetwork $PB $ANS $g mcts_solver  _mcts"; fi
    if has_memory 30; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku AZ $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku max $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku heur $PB $ANS $g mcts_solver"; fi
    if has_memory 500; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku noNetwork-4T $PB $ANS $g mcts_solver  _mcts"; fi
    if has_memory 30; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku AZ-4T $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku max-4T $PB $ANS $g mcts_solver"; fi
    if has_memory 30; then run_cmd $i run_cmd $i "/workspace/scripts/solve.sh gomoku heur-4T $PB $ANS $g mcts_solver"; fi

    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku noNetwork $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku AZ $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku max $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku heur $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku noNetwork-4T $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku AZ-4T $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku max-4T $PB $ANS_FDFPN $g dfpn_solver"; fi
    if has_memory 30; then run_cmd $i "/workspace/scripts/solve.sh gomoku heur-4T $PB $ANS_FDFPN $g dfpn_solver"; fi
done
