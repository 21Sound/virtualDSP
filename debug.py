# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt

with open('debug.raw', 'rb') as file:
    inData = np.frombuffer(file.read(), dtype=np.float64)
    
nfft = 2*np.size(inData,0)
xAx = np.linspace(0, 48000, int(nfft/2+1))
plt.semilogx(xAx, 20*np.log10(np.abs(np.fft.rfft(inData, nfft))))