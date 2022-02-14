import time
import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
from Configuration import *
from Network import AlphaZeroWinLoss
from Network import AlphaZeroSpaceComplexity
from torch.utils.data import DataLoader
from torch.utils.data import IterableDataset
from torch.utils.data import get_worker_info

import os
import sys
sys.path.append("/workspace/Release")
import miniZeroPy

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

class MiniZeroDataset(IterableDataset):
    def __init__(self, trainDir, start, end, seed, train_win_loss):
        self.trainDir = trainDir
        self.start = start
        self.end = end
        self.seed = seed
        self.train_win_loss = train_win_loss
    
    def __iter__(self):
        dataLoader = miniZeroPy.DataLoader(self.trainDir, self.seed + get_worker_info().id)
        dataLoader.load(self.start, self.end)

        while True:
            dataLoader.calculateFeaturesAndLabels()
            
            features = torch.FloatTensor(dataLoader.getFeatures()).view(conf.getNNInputChannel(), conf.getBoardSize(), conf.getBoardSize())
            policy = torch.FloatTensor(dataLoader.getPolicy())
            if self.train_win_loss:
                value = torch.FloatTensor([dataLoader.getValueWinLoss()])
                yield features, policy, value
            else:
                blackV = torch.FloatTensor(dataLoader.getBlackValue())
                whiteV = torch.FloatTensor(dataLoader.getWhiteValue())
                yield features, policy, blackV, whiteV

def lossOfWinLoss(outputP, outputV, labelP, labelV):
    lossP = -(labelP * nn.functional.log_softmax(outputP, dim=1)).sum() / outputP.shape[0]
    lossV = torch.nn.functional.mse_loss(outputV, labelV)
    return lossP, lossV

def lossOfSpaceComplexity(outputP, outputPN, outputDN, labelP, labelPN, labelDN):
    lossP = -(labelP * nn.functional.log_softmax(outputP, dim=1)).sum() / outputP.shape[0]
    lossPN = -(labelPN * nn.functional.log_softmax(outputPN, dim=1)).sum() / outputPN.shape[0]
    lossDN = -(labelDN * nn.functional.log_softmax(outputDN, dim=1)).sum() / outputDN.shape[0]
    return lossP, lossPN, lossDN

def addTrainInfo(trainInfo, key, value):
    if key not in trainInfo: trainInfo[key] = 0
    trainInfo[key] += value

def calAccuracy(output, label):
    maxPolicy = np.argmax(output.to('cpu').detach().numpy(), axis=1)
    maxLabel = np.argmax(label.to('cpu').detach().numpy(), axis=1)
    return (maxPolicy==maxLabel).sum() / TRAIN_BATCH_SIZE
    
def loadModel(trainDir, modelFile):
    step = 0
    if conf.isTrainWinLoss(): net = AlphaZeroWinLoss(conf.getNNInputChannel(), conf.getNNFilterSize(), conf.getBoardSize(), conf.getMaxNumLegalAction())
    else: net = AlphaZeroSpaceComplexity(conf.getNNInputChannel(), conf.getNNFilterSize(), conf.getBoardSize(), conf.getMaxNumLegalAction(), conf.getNNNumOuputValue())
    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    net.to(device)
    optimizer = optim.SGD(net.parameters(), lr=LEARNING_RATE, momentum=MOMENTUM, weight_decay=WEIGHT_DECAY)
    scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=200000, gamma=0.1)
    
    if not modelFile == "":
        eprint("load model "+trainDir+"/model/"+modelFile)
        model = torch.load(trainDir+"/model/"+modelFile, map_location=torch.device('cpu'))
        step = model['step']
        net.load_state_dict(model['netWeights'])
        optimizer.load_state_dict(model['optimizer'])
        scheduler.load_state_dict(model['scheduler'])
    
    return step, net, device, optimizer, scheduler

def saveModel(step, net, device, optimizer, scheduler, trainDir):
    state = {'step': step, 'netWeights': net.module.state_dict(), 'optimizer': optimizer.state_dict(), 'scheduler': scheduler.state_dict() }
    torch.save(state, trainDir+"/model/weight_iter_"+str(step)+".pkl")

    traced_script_module = torch.jit.script(net.module)
    traced_script_module.save(trainDir+"/model/weight_iter_"+str(step)+".pt")
    
if __name__ == '__main__':
    if len(sys.argv) == 6:
        trainDir = sys.argv[1]
        modelFile = sys.argv[2]
        start = int(sys.argv[3])
        end = int(sys.argv[4])
        conf = miniZeroPy.Conf(sys.argv[5])
    else:
        eprint("python Train.py trainDir modelFile start end conf_file")
        exit(0)

    # initialize
    step, net, device, optimizer, scheduler = loadModel(trainDir, modelFile)
    net = nn.DataParallel(net)
    
    if start == -1:
        saveModel(step, net, device, optimizer, scheduler, trainDir)
        exit(0)
    
    # create dataset & dataloader
    dataset = MiniZeroDataset(trainDir, start, end, SEED, conf.isTrainWinLoss())
    dataloader = DataLoader(dataset, batch_size=TRAIN_BATCH_SIZE, num_workers=NUM_PROCESS)
    dataloaderIter = iter(dataloader)

    loss_accumulation = {}
    for i in range(1, NUM_TRAIN_STEP+1):
        # update network
        optimizer.zero_grad()

        if conf.isTrainWinLoss():
            features, labelP, labelV = next(dataloaderIter)
            outputP, outputV = net(features.to(device))
            lossP, lossV = lossOfWinLoss(outputP, outputV, labelP.to(device), labelV.to(device))
            loss = lossP + lossV
        else:
            features, labelP, labelBlackV, labelWhiteV = next(dataloaderIter)
            outputP, outputBlackV, outputWhiteV = net(features.to(device))
            lossP, lossBlackV, lossWhiteV = lossOfSpaceComplexity(outputP, outputBlackV, outputWhiteV, labelP.to(device), labelBlackV.to(device), labelWhiteV.to(device))
            loss = lossP + lossBlackV + lossWhiteV
        
        loss.backward()
        optimizer.step()
        scheduler.step()

        # record train info
        addTrainInfo(loss_accumulation, 'lossP', lossP.item())
        addTrainInfo(loss_accumulation, 'accP', calAccuracy(outputP, labelP))

        if conf.isTrainWinLoss():
            addTrainInfo(loss_accumulation, 'lossV', lossV.item())
        else:
            addTrainInfo(loss_accumulation, 'lossBlackV', lossBlackV.item())
            addTrainInfo(loss_accumulation, 'accBlackV', calAccuracy(outputBlackV, labelBlackV))
            addTrainInfo(loss_accumulation, 'lossWhiteV', lossWhiteV.item())
            addTrainInfo(loss_accumulation, 'accWhiteV', calAccuracy(outputWhiteV, labelWhiteV))

        # display stattistics
        step += 1
        if step != 0 and step % TRAIN_DISPLAY_STEP == 0:
            eprint("[{}] nn step {}, lr: {}.".format(time.strftime("%Y-%m-%d %H:%M:%S", time.localtime()), step, round(optimizer.param_groups[0]["lr"], 6)), end=" ")
            for loss in loss_accumulation: eprint("{}: {}".format(loss, round(loss_accumulation[loss]/TRAIN_DISPLAY_STEP, 5)), end=" ")
            eprint()
            loss_accumulation = {}

    # save weights
    saveModel(step, net, device, optimizer, scheduler, trainDir)
    print("Optimization_Done", step)
    eprint("Optimization_Done", step)
