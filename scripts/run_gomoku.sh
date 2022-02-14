#!/bin/bash

GPU_NUMS=`nvidia-smi -L | wc -l`
PB=/workspace/problems/Gomoku
ANS=/workspace/result/Gomoku/MCTS
ANS_FDFPN=/workspace/result/Gomoku/FDFPN

if [ "$GPU_NUMS" -lt 1 ]; then
    echo "No GPU"
    exit 1
fi

if [ "$MEM_FREE" -lt 20 ]; then
    echo "No enough free memory. Need at least 20 GB. Find $MEM_FREE GB only."
    exit 1
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

has_memory() {
    MEM_FREE=`free -g | grep Mem | awk '{print $7}'`
    if [ "$MEM_FREE" -lt ${1} ]; then
        echo "No enough free memory. Need at least ${1} GB. Find $MEM_FREE GB only."
        return 1
    else
        return 0
    fi
}

if has_memory 500; then /workspace/scripts/solve.sh gomoku noNetwork $PB $ANS 0 mcts_solver _mcts; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku AZ $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku max $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku heur $PB $ANS 0 mcts_solver; fi
if has_memory 500; then /workspace/scripts/solve.sh gomoku noNetwork-4T $PB $ANS 0 mcts_solver _mcts; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku AZ-4T $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku max-4T $PB $ANS 0 mcts_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku heur-4T $PB $ANS 0 mcts_solver; fi

if has_memory 30; then /workspace/scripts/solve.sh gomoku noNetwork $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku AZ $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku max $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku heur $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku noNetwork-4T $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku AZ-4T $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku max-4T $PB $ANS_FDFPN 0 dfpn_solver; fi
if has_memory 30; then /workspace/scripts/solve.sh gomoku heur-4T $PB $ANS_FDFPN 0 dfpn_solver; fi

echo "Use \"./scripts/display_solver_results_gomoku.sh $PB $ANS f\" to show the result for MCTS"
echo "Use \"./scripts/display_solver_results_gomoku.sh $PB $ANS_FDFPN f\" to show the result for FDFPN"