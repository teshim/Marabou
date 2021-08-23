
import sys
import os
import tensorflow as tf
import numpy as np
import onnx
import onnxruntime
import argparse
import json

from maraboupy import MarabouNetworkONNX as monnx
from maraboupy import MarabouCore
from maraboupy import MarabouUtils
from maraboupy import Marabou

import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--sample", type=int, default=0, help="Sample number")
parser.add_argument("--dist", type=float, default=0.03, help="Property distance")
parser.add_argument("--onnx", type=str, default='fullVanilla.onnx', help="Onnx net file")
parser.add_argument("--gurobi", type=str, default='lp', choices=['lp', 'milp'], help="Gurbi solver type")
parser.add_argument("--flag", type=str, default='', help="Addition to the dump file's name")
args = parser.parse_args()

(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
x_train, x_test = x_train / 255.0, x_test / 255.0
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)

xAdv = x_test[args.sample]
mbouNet  = monnx.MarabouNetworkONNX(args.onnx)
epsilon = args.dist
for i,x in enumerate(np.nditer(xAdv)):
    mbouNet.setUpperBound(i,max(x + epsilon, 0))    
    mbouNet.setLowerBound(i,max(x - epsilon, 0))

options = Marabou.createOptions(verbosity=0, timeoutInSeconds=0, milpTightening=args.gurobi, dumpBounds=True, tighteningStrategy='none')
ipq = MarabouCore.preprocess(mbouNet.getMarabouQuery(), options)
print("Number of variables in IPQ={}".format(ipq.getNumberOfVariables()))
if ipq.getNumberOfVariables() == 0:
    print("UNSAT on first LP bound tightening")
    exit()
else:
    newName = "dumpBounds_{}_{}{}.json".format(args.sample, str(args.dist).replace('.','-'), "_" + args.flag if args.flag else "")
    os.rename("dumpBounds.json", newName)
    print("dumped={}".format(newName))