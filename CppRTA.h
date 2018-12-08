/*----------------------------------------------------------------------------*\
Header of CppRTA.cpp

Author: (c) Hagen Jaeger    January 2017 - Now
\*----------------------------------------------------------------------------*/

#ifndef _CPPRTA_H
#define _CPPRTA_H

#include <vector>
#include <fstream>
#include <cstdint>
#include "portaudio.h"
#include "CppDSP.h"

struct deviceContainerRTA {
    std::string name, hostAPI;
    double latency;
    uint32_t ID = 0;
    uint32_t numChans = 0;
    bool inputFlag = false;
};

class CppRTA {

public:

    CppRTA(deviceContainerRTA inDev, deviceContainerRTA outDev, uint32_t blockLen, uint32_t fs);

    static int getHostAPIs(std::vector<std::string> &apis);

    static int getDevices(std::vector<deviceContainerRTA> &inDevices,
                              std::vector<deviceContainerRTA> &outDevices);

    void startStream();

    void stopStream();

    inline int setNumEQs(uint32_t chanID, uint32_t newSize) {
        if (chanID<EQ.size()) {
            EQ.at(chanID).resize(newSize);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setEqGain(uint32_t chanID, uint32_t eqID, double gain) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setGain(gain);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setEqFrequency(uint32_t chanID, uint32_t eqID, double freq) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setFreq(freq);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setEqQFactor(uint32_t chanID, uint32_t eqID, double Q) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setQFactor(Q);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setEqType(uint32_t chanID, uint32_t eqID, eqType type) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setType(type);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setCutCharacteristic(filterType type, uint32_t chanID, filterChar charac) {
        if (chanID<hiPass.size() && chanID<loPass.size()) {
        	if (type == HIGHPASS) {
        		hiPass.at(chanID).setChar(charac);
        	} else if (type == LOWPASS) {
        		loPass.at(chanID).setChar(charac);
        	}
            return 0;
        } else {
            return -1;
        }
    }

    inline int setCutFrequency(filterType type, uint32_t chanID, double freq) {
    	if (chanID<hiPass.size() && chanID<loPass.size()) {
        	if (type == HIGHPASS) {
        		hiPass.at(chanID).setFreq(freq);
        	} else if (type == LOWPASS) {
        		loPass.at(chanID).setFreq(freq);
        	}
            return 0;
        } else {
            return -1;
        }
    }

    inline int setCutOrder(filterType type, uint32_t chanID, uint32_t ord) {
    	if (chanID<hiPass.size() && chanID<loPass.size()) {
        	if (type == HIGHPASS) {
        		hiPass.at(chanID).setOrder(ord);
        	} else if (type == LOWPASS) {
        		loPass.at(chanID).setOrder(ord);
        	}
            return 0;
        } else {
            return -1;
        }
    }

    inline int setThreshold(uint32_t chanID, double thres) {
        if (chanID<limiter.size()) {
            limiter.at(chanID).setThreshold(thres);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setMakeupGain(uint32_t chanID, double makeupGainLog)  {
        if (chanID<limiter.size()) {
            limiter.at(chanID).setMakeupGain(makeupGainLog);
            return 0;
        } else {
            return -1;
        }
    }

    inline int setReleaseTime(uint32_t chanID, double secRel) {
        if (chanID<limiter.size()) {
            limiter.at(chanID).setReleaseTime(secRel);
            return 0;
        } else {
            return -1;
        }
    }

    inline uint32_t getNumChans() {
		return (uint32_t) EQ.size();
    }

    inline uint32_t getNumEQs(uint32_t chanID) {
		return (uint32_t) EQ.at(chanID).size();
    }

    inline double getEqGain(uint32_t chanID, uint32_t eqID) {
		return EQ.at(chanID).at(eqID).getGain();
    }

    inline double getEqFrequency(uint32_t chanID, uint32_t eqID) {
		return EQ.at(chanID).at(eqID).getFreq();
    }

    inline double getEqQFactor(uint32_t chanID, uint32_t eqID) {
		return EQ.at(chanID).at(eqID).getQFact();
    }

    inline eqType getEqType(uint32_t chanID, uint32_t eqID) {
		return EQ.at(chanID).at(eqID).getType();
    }

    inline filterChar getCutCharacteristic(filterType type, uint32_t chanID) {
    	if (type == HIGHPASS) {
    		return hiPass.at(chanID).getChar();
    	} else if (type == LOWPASS) {
    		return loPass.at(chanID).getChar();
    	} else
    		return UNKNOWN_FILTERCHAR;
    }

    inline double getCutFrequency(filterType type, uint32_t chanID) {
    	if (type == HIGHPASS) {
    		return hiPass.at(chanID).getFreq();
    	} else if (type == LOWPASS) {
    		return loPass.at(chanID).getFreq();
    	} else
    		return -1.0;
    }

    inline uint32_t getCutOrder(filterType type, uint32_t chanID) {
    	if (type == HIGHPASS) {
    		return hiPass.at(chanID).getOrd();
    	} else if (type == LOWPASS) {
    		return loPass.at(chanID).getOrd();
    	} else
    		return 0;
    }

    inline double getThreshold(uint32_t chanID) {
		return limiter.at(chanID).getThres();
    }

    inline double getMakeupGain(uint32_t chanID)  {
		return limiter.at(chanID).getMakeup();
    }

    inline double getReleaseTime(uint32_t chanID) {
		return limiter.at(chanID).getReleaseTime();
    }

    int getTransferFunction(std::vector<double> &tf, uint32_t chanID, uint32_t nfft);

    ~CppRTA(void);

protected:

    static int duplexCallback(const void *vInputBuffer, void *vOutputBuffer,
                              unsigned long iFramesPerBuffer,
                              const PaStreamCallbackTimeInfo* sTimeInfo,
                              PaStreamCallbackFlags iStatusFlags,
                              void *userData);

    static int inCallback(const void *vInputBuffer, void *vOutputBuffer,
                              unsigned long iFramesPerBuffer,
                              const PaStreamCallbackTimeInfo* sTimeInfo,
                              PaStreamCallbackFlags iStatusFlags,
                              void *userData);

    static int outCallback(const void *vInputBuffer, void *vOutputBuffer,
                              unsigned long iFramesPerBuffer,
                              const PaStreamCallbackTimeInfo* sTimeInfo,
                              PaStreamCallbackFlags iStatusFlags,
                              void *userData);

private:
    PaStream *paInStream, *paOutStream, *paDuplexStream;
    deviceContainerRTA inDev, outDev;
    std::vector< std::vector<CppEQ> > EQ;
    std::vector<CppXover> hiPass, loPass;
    std::vector<CppLimiter> limiter;
    std::vector< std::vector<double> > inData, outData;
    uint32_t fs, blockLen;
};

#endif

//--------------------- License ------------------------------------------------

// Copyright (c) 2017 Hagen Jaeger

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files
// (the "Software"), to deal in the Software without restriction,
// including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software,
// and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
