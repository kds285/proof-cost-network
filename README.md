# AlphaZero-based Proof Cost Network to Aid Game Solving

This repository is the official implementation for ICLR 2022 paper [AlphaZero-based Proof Cost Network to Aid Game Solving].

## Requirements

All the experiments, including the training part and the evaluation part, are done in a container. The dockerfile for building the container is placed in the directory "docker/Dockerfile". You can use either docker or podman commands to set up/attach the environment.

In this instruction, we use podman commands to demonstrate how to reproduce our experiment results. If you need to install podman, please refer to the commands below (skip this step if you already installed podman):
```install podman (optional)
sudo apt-get update 
sudo apt-get install -y curl

source /etc/os-release
echo "deb https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_${VERSION_ID}/ /" | sudo tee /etc/apt/sources.list.d/devel:kubic:libcontainers:stable.list
curl -L https://download.opensuse.org/repositories/devel:/kubic:/libcontainers:/stable/xUbuntu_${VERSION_ID}/Release.key | sudo apt-key add -
sudo apt-get update
sudo apt-get -y install podman
```

To setup the container, run the commands below to build the image for our container and run the container in the background.
```setup
cd docker
podman build . --tag minizero --no-cache
cd ..
podman run --name minizero --cap-add=SYS_PTRACE --security-opt seccomp=unconfined --network=host --ipc=host --rm -it -d -v .:/workspace minizero bash
```

## Training

Before training, you need to select a game (either 9x9 Killall-Go or 15x15 Gomoku) to experiment, and compile the corresponding executables.
Note that if you compile for one game, the executables for the other game will be removed.

To compile 15x15 Gomoku version:
```compile executables for 15x15 Gomoku
podman exec -it minizero ./scripts/clean-up.sh
podman exec -it minizero ./scripts/setup-cmake.sh GOMOKU release
podman exec -it minizero make -j
```

To compile 9x9 Killall-Go version:
```compile executables for 9x9 killall Go
podman exec -it minizero ./scripts/clean-up.sh
podman exec -it minizero ./scripts/setup-cmake.sh GO release
podman exec -it minizero make -j
```

To train the model(s) in the paper, run the following three processes at the same time. (Server, Optimizer, and Self-Play Worker.)
You may use three terminals to run three processes at the same time.

### Run Server

The server listens to the 9999 port, please make sure that this port is not being used by other programs. 
Note that you can only train one model at the same time. 
To train a model in the paper, run the corresponding commands below (only run the one you are training). The second last argument (e.g. "training/gomoku_AZ") indicates the directory to place the training logs and models.

15x15 Gomoku (NDK) α0:
```train gomoku_AZ
podman exec -it minizero ./scripts/zero-server.sh training_cfg/gomoku_AZ.cfg training/gomoku_AZ 300
# press 'y'
```

15x15 Gomoku (NDK) PCN-b_max:
```train gomoku_max
podman exec -it minizero ./scripts/zero-server.sh training_cfg/gomoku_max.cfg training/gomoku_max 300
# press 'y'
```

15x15 Gomoku (NDK) PCN-b_heur:
```train gomoku_heur
podman exec -it minizero ./scripts/zero-server.sh training_cfg/gomoku_heur.cfg training/gomoku_heur 300
# press 'y'
```

15x15 Gomoku (4T) α0:
```train gomoku_AZ-4T
podman exec -it minizero ./scripts/zero-server.sh training_cfg/gomoku_AZ-4T.cfg training/gomoku_AZ-4T 300
# press 'y'
```

15x15 Gomoku (4T) PCN-b_max:
```train gomoku_max-4T
podman exec -it minizero ./scripts/zero-server.sh training_cfg/gomoku_max-4T.cfg training/gomoku_max-4T 300
# press 'y'
```

15x15 Gomoku (4T) PCN-b_heur:
```train gomoku_heur-4T
podman exec -it minizero ./scripts/zero-server.sh training_cfg/gomoku_heur-4T.cfg training/gomoku_heur-4T 300
# press 'y'
```

9x9 Killall-Go α0:
```train go_AZ
podman exec -it minizero ./scripts/zero-server.sh training_cfg/go_AZ.cfg training/go_AZ 300
# press 'y'
```

9x9 Killall-Go PCN-b_max (OR: White):
```train go_W-max
podman exec -it minizero ./scripts/zero-server.sh training_cfg/go_W-max.cfg training/go_W-max 300
# press 'y'
```

9x9 Killall-Go PCN-b_heur (OR: White):
```train go_W-heur
podman exec -it minizero ./scripts/zero-server.sh training_cfg/go_W-heur.cfg training/go_W-heur 300
# press 'y'
```

9x9 Killall-Go PCN-b_max (OR: Black):
```train go_B-max
podman exec -it minizero ./scripts/zero-server.sh training_cfg/go_B-max.cfg training/go_B-max 300
# press 'y'
```

9x9 Killall-Go PCN-b_heur (OR: Black):
```train go_B-heur
podman exec -it minizero ./scripts/zero-server.sh training_cfg/go_B-heur.cfg training/go_B-heur 300
# press 'y'
```

### Run Optimizer

To run the Optimizer, run the command below.
```optimizer
podman exec -it minizero ./scripts/worker.sh localhost 9999 op
```


### Run Self-Play Worker

You can run multiple Self-play Workers on different machines to speed up training. 

To run a Self-Play Worker, run the command below. You may replace "localhost" by the hostname of the machine where the server is running if you are using multiple machines.
```self-play worker
podman exec -it minizero ./scripts/worker.sh localhost 9999 sp
```

The training results will be placed in the directory "training/".
For example, if you trained gomoku_AZ, you can find the model under "training/gomoku_AZ/model/".
Training logs including "Training.log" & "sgf/" files can also be found under "training/gomoku_AZ/".


## Evaluation


The models we used in this paper are placed under the directory "models/". To reproduce the experiment results from these models, run the commands below.

### Part A. 15x15 Gomoku

To evaluate each model on solving problems with MCTS and FDFPN solvers, run:
``` MCTS & FDFPN solver for 15x15 Gomoku.
podman exec -it minizero ./scripts/run_gomoku.sh
```

To display MCTS solver results, run:
``` MCTS solver result for 15x15 Gomoku.
podman exec -it minizero ./scripts/display_solver_results_gomoku.sh /workspace/problems/Gomoku /workspace/result/Gomoku/MCTS f
```


To display FDFPN solver results, run:
``` FDFPN solver result for 15x15 Gomoku.
podman exec -it minizero ./scripts/display_solver_results_gomoku.sh /workspace/problems/Gomoku /workspace/result/Gomoku/FDFPN f
```


To evaluate the playing strength of PCNs against AlphaZero, run:
``` Play against AlphaZero on 15x15 Gomoku.
podman exec -it minizero ./scripts/fight_against_AZ_gomoku.sh
```

To display the playing results of PCNs against AlphaZero, run:
``` display 15x15 Gomoku playing strength.
podman exec -it minizero ./scripts/display_fight_results_gomoku.sh
```

### Part B. 9x9 Killall-Go

To evaluate each model on solving problems with MCTS and FDFPN solvers, run:
``` MCTS & FDFPN solver for 9x9 Killall-Go.
podman exec -it minizero ./scripts/run_go.sh
```

To display MCTS solver results, run:
``` MCTS solver result for 9x9 Killall-Go.
podman exec -it minizero ./scripts/display_solver_results_go.sh /workspace/problems/9x9Killall-Go /workspace/result/9x9Killall-Go/MCTS f
```

To display FDFPN solver results, run:
``` FDFPN solver result for 9x9 Killall-Go.
podman exec -it minizero ./scripts/display_solver_results_go.sh /workspace/problems/9x9Killall-Go /workspace/result/9x9Killall-Go/FDFPN f
```

To evaluate the playing strength of PCNs against AlphaZero, run:
``` Play against AlphaZero on 9x9 Killall-Go.
podman exec -it minizero ./scripts/fight_against_AZ_go.sh
```

To display the playing results of PCNs against AlphaZero, run:
``` display 15x15 Gomoku playing strength.
podman exec -it minizero ./scripts/display_fight_results_go.sh
```

To evaluate the model trained by yourself, simply replace the model under "models/" with the one under "training/".
For example, replacing "models/gomoku_AZ/weight_iter_150000.pt" with "training/gomoku_AZ/model/weight_iter_150000.pt".


## Results

For both 15x15 Gomoku and 9x9 Killall-Go, PCN outperforms AlphaZero on problem sets placed under the folder "problems/".
In terms of playing strength, PCN has win rates near or even higher than 50% against AlphaZero.
The following tables show the number of problems that can be solved within 30 minutes with each model.


### MCTS


| Game & Setup  \    Model      |   No Network       |         α0        |    PCN-b_max        |    PCN-b_heur     |
| ----------------------------- |------------------- | ----------------- | ------------------- | ----------------- |
| 15x15 Gomoku (NDK)            |      1 / 77        |      23 / 77      |      43 / 77        |      38 / 77      |
| 15x15 Gomoku (4T)             |     22 / 77        |      64 / 77      |      77 / 77        |      73 / 77      |
| 9x9 Killall-Go (OR: White)    |      1 / 81        |      28 / 81      |      79 / 81        |      76 / 81      |
| 9x9 Killall-Go (OR: Black)    |      1 / 81        |      28 / 81      |      38 / 81        |      46 / 81      |


### FDFPN


| Game & Setup  \   Model     |   No Network       |         α0        |    PCN-b_max        |    PCN-b_heur     |
| --------------------------- |------------------- | ----------------- | ------------------- | ----------------- |
| 15x15 Gomoku (NDK)          |      6 / 77        |      15 / 77      |      45 / 77        |      48 / 77      |
| 15x15 Gomoku (4T)           |     34 / 77        |      41 / 77      |      71 / 77        |      69 / 77      |
| 9x9 Killall-Go (OR: White)  |     31 / 81        |      76 / 81      |      79 / 81        |      77 / 81      |
| 9x9 Killall-Go (OR: Black)  |     31 / 81        |      76 / 81      |      66 / 81        |      68 / 81      |


### Playing Strength


| Game & Our Model \  Result against α0  |    Win Rate (WR)   |     Black WR      |      White WR      |
| -------------------------------------- |------------------- | ----------------- | ------------------ |
| 15x15 Gomoku (NDK)    PCN-b_max        |   50.40% ± 6.21%   |      100.00%      |         0.80%      |
| 15x15 Gomoku (NDK)    PCN-b_heur       |   50.80% ± 6.21%   |      100.00%      |         1.60%      |
| 15x15 Gomoku (4T)     PCN-b_max        |   50.40% ± 6.21%   |       96.80%      |         4.00%      |
| 15x15 Gomoku (4T)     PCN-b_heur       |   50.00% ± 6.21%   |      100.00%      |         0.80%      |
| 9x9 Killall-Go  PCN-b_max  (OR: White) |   50.00% ± 6.21%   |       34.40%      |        65.60%      |
| 9x9 Killall-Go  PCN-b_heur (OR: White) |   60.80% ± 6.07%   |       66.40%      |        55.20%      |
| 9x9 Killall-Go  PCN-b_max  (OR: White) |   62.25% ± 6.03%   |       60.00%      |        64.52%      |
| 9x9 Killall-Go  PCN-b_heur (OR: White) |   60.08% ± 6.06%   |       53.23%      |        66.94%      |


