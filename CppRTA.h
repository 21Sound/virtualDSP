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
    CppRTA();

    CppRTA(deviceContainerRTA inDev, deviceContainerRTA outDev, uint32_t blockLen, uint32_t fs);

    static int32_t getDevices(std::vector<deviceContainerRTA> &inDevices,
                              std::vector<deviceContainerRTA> &outDevices);

    inline int32_t setNumEQs(uint32_t chanID, uint32_t  newSize) {
        if (chanID<EQ.size()) {
            EQ.at(chanID).resize(newSize);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setGain(uint32_t chanID, uint32_t eqID, double gain) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setGain(gain);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setFreq(uint32_t chanID, uint32_t eqID, double freq) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setFreq(freq);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setQFactor(uint32_t chanID, uint32_t eqID, double Q) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setQFactor(Q);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setType(uint32_t chanID, uint32_t eqID, int32_t type) {
        if (chanID<EQ.size() && eqID<EQ.at(chanID).size()) {
            EQ.at(chanID).at(eqID).setType(type);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setThreshold(uint32_t chanID, double thres) {
        if (chanID<limiter.size()) {
            limiter.at(chanID).setThreshold(thres);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setMakeupGain(uint32_t chanID, double makeupGainLog)  {
        if (chanID<limiter.size()) {
            limiter.at(chanID).setMakeupGain(makeupGainLog);
            return 0;
        } else {
            return -1;
        }
    }

    inline int32_t setReleaseTime(uint32_t chanID, double secRel) {
        if (chanID<limiter.size()) {
            limiter.at(chanID).setReleaseTime(secRel);
            return 0;
        } else {
            return -1;
        }
    }

    ~CppRTA(void);

protected:

    static int32_t duplexCallback(const void *vInputBuffer, void *vOutputBuffer,
                              unsigned long iFramesPerBuffer,
                              const PaStreamCallbackTimeInfo* sTimeInfo,
                              PaStreamCallbackFlags iStatusFlags,
                              void *userData);

private:
    PaStream *paStream;
    std::vector< std::vector<CppEQ> > EQ;
    std::vector< std::vector<double> > inData, outData;
    std::vector<CppLimiter> limiter;
    uint32_t fs, blockLen, numInChans, numOutChans;
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
