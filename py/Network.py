import torch
import torch.nn as nn
import torch.nn.functional as F

class ResBlock(nn.Module):
    def __init__(self, channels):
        super(ResBlock, self).__init__()

        self.conv1 = nn.Conv2d(channels, channels, 3, padding=1)
        self.bn1 = nn.BatchNorm2d(channels)
        self.conv2 = nn.Conv2d(channels, channels, 3, padding=1)
        self.bn2 = nn.BatchNorm2d(channels)

    def forward(self, x):
        input = x

        x = self.conv1(x)
        x = F.relu(self.bn1(x))
        x = self.conv2(x)
        x = self.bn2(x)
        x = F.relu(x+input)
        return x

class PolicyHead(nn.Module):
    def __init__(self, inputC, boardSize, outputP):
        super(PolicyHead, self).__init__()

        self.boardSize = boardSize
        self.conv = nn.Conv2d(inputC, 2, 1)
        self.bn = nn.BatchNorm2d(2)
        self.fc = nn.Linear(boardSize**2*2, outputP)

    def forward(self, x):
        x = self.conv(x)
        x = F.relu(self.bn(x))
        x = x.view(-1, int(self.boardSize**2*2))
        x = self.fc(x)
        return x

class ValueHeadWinLoss(nn.Module):
    def __init__(self, inputC, boardSize):
        super(ValueHeadWinLoss, self).__init__()

        self.boardSize = boardSize
        self.conv = nn.Conv2d(inputC, 1, 1)
        self.bn = nn.BatchNorm2d(1)
        self.fc1 = nn.Linear(boardSize**2, 256)
        self.fc2 = nn.Linear(256, 1)
        self.tanh = nn.Tanh()

    def forward(self, x):
        x = self.conv(x)
        x = F.relu(self.bn(x))
        x = x.view(-1, int(self.boardSize**2))
        x = F.relu(self.fc1(x))
        x = self.fc2(x)
        x = self.tanh(x)
        return x

class ValueHeadSpaceComplexity(nn.Module):
    def __init__(self, inputC, boardSize, outputC):
        super(ValueHeadSpaceComplexity, self).__init__()

        self.boardSize = boardSize
        self.conv = nn.Conv2d(inputC, 2, 1)
        self.bn = nn.BatchNorm2d(2)
        self.fc = nn.Linear(boardSize**2*2, outputC)

    def forward(self, x):
        x = self.conv(x)
        x = F.relu(self.bn(x))
        x = x.view(-1, int(self.boardSize**2*2))
        x = self.fc(x)
        return x

class AlphaZeroWinLoss(nn.Module):
    def __init__(self, inputC, filter, boardSize, outputP):
        super(AlphaZeroWinLoss, self).__init__()

        self.conv = nn.Conv2d(inputC, filter, 3, padding=1)
        self.bn = nn.BatchNorm2d(filter)
        self.res1 = ResBlock(filter)
        self.res2 = ResBlock(filter)
        self.res3 = ResBlock(filter)
        self.res4 = ResBlock(filter)
        self.res5 = ResBlock(filter)
        self.policy = PolicyHead(filter, boardSize, outputP)
        self.value = ValueHeadWinLoss(filter, boardSize)

    def forward(self, x):
        x = self.conv(x)
        x = F.relu(self.bn(x))
        x = self.res1(x)
        x = self.res2(x)
        x = self.res3(x)
        x = self.res4(x)
        x = self.res5(x)
        policy = self.policy(x)
        value = self.value(x)
        return policy, value

class AlphaZeroSpaceComplexity(nn.Module):
    def __init__(self, inputC, filter, boardSize, outputP, outputV):
        super(AlphaZeroSpaceComplexity, self).__init__()

        self.conv = nn.Conv2d(inputC, filter, 3, padding=1)
        self.bn = nn.BatchNorm2d(filter)
        self.res1 = ResBlock(filter)
        self.res2 = ResBlock(filter)
        self.res3 = ResBlock(filter)
        self.res4 = ResBlock(filter)
        self.res5 = ResBlock(filter)
        self.policy = PolicyHead(filter, boardSize, outputP)
        self.blackV = ValueHeadSpaceComplexity(filter, boardSize, outputV)
        self.whiteV = ValueHeadSpaceComplexity(filter, boardSize, outputV)

    def forward(self, x):
        x = self.conv(x)
        x = F.relu(self.bn(x))
        x = self.res1(x)
        x = self.res2(x)
        x = self.res3(x)
        x = self.res4(x)
        x = self.res5(x)
        policy = self.policy(x)
        blackV = self.blackV(x)
        whiteV = self.whiteV(x)
        return policy, blackV, whiteV

