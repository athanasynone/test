import time
import threading
import os
import subprocess
import time
import json
import inspect
import ctypes
import copy 
import numpy as np

x=np.array([1,2,3,1,2,3,1,2,3,1,2,3])
h=np.array([4,5,6,4,5,6,4,5,6,4,5,6])
import scipy.signal
y = scipy.signal.convolve(x,h)
print(y[0:12])
print(y)
