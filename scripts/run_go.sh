#!/bin/bash

GPU_NUMS=`nvidia-smi -L | wc -l`
PB=/workspace/problems/9x9Killall-Go
ANS=/workspace/result/9x9Killall-Go/MCTS
ANS_FDFPN=/workspace/result/9x9Killall-Go/FDFPN

if [ "$GPU_NUMS" -lt 1 ]; then
    echo "No GPU"
    exit 1
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

has_memory() {
    MEM_FREE=`free -g | grep Mem | awk '{print $7}'`
    if [ "$MEM_FREE" -lt ${1} ]; then
        echo "No enough free memory. Need at least ${1} GB. Find $MEM_FREE GB only."
        return 1
    else
        return 0
    fi
}

if has_memory 90; then /workspace/scripts/solve.sh go noNetwork $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go AZ $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go W-max $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go W-heur $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go B-max $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go B-heur $PB $ANS 0 mcts_solver; fi

if has_memory 30; then /workspace/scripts/solve.sh go noNetwork $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go AZ $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go W-max $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go W-heur $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go B-max $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh go B-heur $PB $ANS_FDFPN 0 dfpn_solver; fi

echo "Use \"./scripts/display_solver_results_go.sh $PB $ANS f\" to show the result for MCTS"
echo "Use \"./scripts/display_solver_results_go.sh $PB $ANS_FDFPN f\" to show the result for FDFPN"
