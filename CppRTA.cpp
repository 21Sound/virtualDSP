/*----------------------------------------------------------------------------*\
Real time audio streaming class, uses portaudio, libsndfile and mpg123.

Author: (c) Hagen Jaeger    January 2017 - Now
\*----------------------------------------------------------------------------*/

#include <stdexcept>
#include "CppRTA.h"

CppRTA::CppRTA() :
    paStream(nullptr), blockLen(0), fs(0), numInChans(0), numOutChans(0) {

}

CppRTA::CppRTA(deviceContainerRTA inDev, deviceContainerRTA outDev, uint32_t blockLen, uint32_t fs)
    : paStream(nullptr), blockLen(blockLen), fs(fs), numInChans(0), numOutChans(0) {
    const PaDeviceInfo *inDevInfo, *outDevInfo;
    PaStreamParameters inParams, outParams;
    PaError paErr = paNoError;

    if (this->blockLen < 0x20) {
        this->blockLen = 0x20;
    }

    paErr = Pa_Initialize();
    if(paErr != paNoError) {
        throw std::runtime_error(std::string(Pa_GetErrorText(paErr)));
    }

    this->numInChans = inDev.numChans;
    this->numOutChans = outDev.numChans;

    limiter.resize(numOutChans);
    EQ.resize(numOutChans);
    for (uint32_t i=0; i<numOutChans; i++) {
        limiter.at(i).setSampleRate(fs);
        EQ.at(i).resize(1);
        EQ.at(i).at(0).setSampleRate(fs);
    }

    inData.resize(numInChans);
    for (uint32_t i=0; i<numInChans; i++) {
        inData.at(i).resize(blockLen, 0.0);
    }

    outData.resize(numOutChans);
    for (uint32_t i=0; i<numOutChans; i++) {
        outData.at(i).resize(blockLen, 0.0);
    }

    inParams.device = inDev.ID;
    inParams.channelCount = inDev.numChans;
    inParams.sampleFormat = paFloat32;
    inParams.suggestedLatency = inDev.latency;
    inParams.hostApiSpecificStreamInfo = NULL;

    outParams.device = outDev.ID;
    outParams.channelCount = outDev.numChans;
    outParams.sampleFormat = paFloat32;
    outParams.suggestedLatency = outDev.latency;
    outParams.hostApiSpecificStreamInfo = NULL;

    paErr = Pa_OpenStream(&paStream, &inParams, &outParams, fs, blockLen, paNoFlag, duplexCallback, this);
    if(paErr != paNoError) {
        throw std::invalid_argument(std::string(Pa_GetErrorText(paErr)));
    }

    paErr = Pa_StartStream(paStream);
    if(paErr != paNoError) {
        throw std::runtime_error(std::string(Pa_GetErrorText(paErr)));
    }

    this->fs = fs;
}

int32_t CppRTA::duplexCallback(const void *inBuf, void *outBuf,
                           unsigned long framesPerBuf,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlag,
                           void *userData)
{
    (void) timeInfo; (void) statusFlag;

    CppRTA* obj = (CppRTA*) userData;
    float *playData = (float*) outBuf;
    float *recData = (float*) inBuf;

    for (uint32_t i = 0; i<obj->blockLen; i++) {
        for (uint32_t j = 0; j<obj->numInChans; j++) {
            obj->inData[j][i] = recData[i*obj->numInChans+j];
        }
    }

    for (uint32_t i = 0; i<obj->numOutChans; i++) {
        obj->outData[i] = obj->inData[i%obj->numInChans];
        for (uint32_t j=0; j<obj->EQ[i].size(); j++) {
            obj->EQ[i][j].process(obj->outData[i]);
        }
        obj->limiter[i].process(obj->outData[i]);
    }

    for (uint32_t i = 0; i<obj->blockLen; i++) {
        for (uint32_t j = 0; j<obj->numOutChans; j++) {
            playData[i*obj->numOutChans+j] = obj->outData[j][i];
        }
    }
    return paContinue;
}

int32_t CppRTA::getDevices(std::vector<deviceContainerRTA> &inDevices,
                           std::vector<deviceContainerRTA> &outDevices) {
    PaError paErr;
    const PaDeviceInfo *paDevInfo;
    const PaHostApiInfo *paApiInfo;
    PaDeviceIndex numDevices;
    deviceContainerRTA devInfo;
    const uint32_t maxNameLen = 25;

    paErr = Pa_Initialize();
    if (paErr != paNoError) {
        return -1;
    }

    numDevices = Pa_GetDeviceCount();
    if (numDevices < 1) {
        return -2;
    }

    for (uint32_t i=0; i<numDevices; i++) {
        paDevInfo = Pa_GetDeviceInfo(i);

        devInfo.name = paDevInfo->name;
        if (devInfo.name.size()>maxNameLen) {
            devInfo.name = devInfo.name.substr(0, maxNameLen) + std::string("...");
        }
        devInfo.ID = i;
        devInfo.hostAPI = Pa_GetHostApiInfo(paDevInfo->hostApi)->name;

        if (paDevInfo->maxInputChannels>0) {
            devInfo.numChans = paDevInfo->maxInputChannels;
            devInfo.latency = paDevInfo->defaultLowInputLatency;
            devInfo.inputFlag = true;
            devInfo.name = devInfo.hostAPI + std::string("  |  ") + devInfo.name + std::string("  |  ")
                    + std::to_string(devInfo.numChans) + std::string(" channels");
            inDevices.push_back(devInfo);
        }

        if (paDevInfo->maxOutputChannels>0) {
            devInfo.numChans = paDevInfo->maxOutputChannels;
             devInfo.latency = paDevInfo->defaultLowOutputLatency;
            devInfo.inputFlag = false;
            devInfo.name = devInfo.hostAPI + std::string("  |  ") + devInfo.name + std::string("  |  ")
                    + std::to_string(devInfo.numChans) + std::string(" channels");
            outDevices.push_back(devInfo);
        }
    }

    Pa_Terminate();
}

/*
int32_t CppRTA::storeParams() {
    uint32_t tmpInt;
    double tmpDouble;
    std::ofstream fStr("params.vdsp", std::ios::binary | std::ios::trunc);

    if (fStr.good()) {
        fStr.write((char*)&numOutChans, sizeof(uint32_t));
        fStr.write((char*)&numEQsPerChan, sizeof(uint32_t));
        for (uint32_t i=0; i<numOutChans; i++) {
            for (uint32_t j=0; j<numEQsPerChan; j++) {
                tmpDouble = EQ.at(i).at(j).getSampleRate();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = EQ.at(i).at(j).getGain();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = EQ.at(i).at(j).getFreq();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpDouble = EQ.at(i).at(j).getQFactor();
                fStr.write((char*)&tmpDouble, sizeof(double));
                tmpInt = EQ.at(i).at(j).getType();
                fStr.write((char*)&tmpInt, sizeof(uint32_t));
            }
            tmpDouble = limiter.at(i).getThreshold();
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = limiter.at(i).getMakeupGain();
            fStr.write((char*)&tmpDouble, sizeof(double));
            tmpDouble = limiter.at(i).getReleaseTime();
            fStr.write((char*)&tmpDouble, sizeof(double));
        }
        return 0;
    } else {
        return -1;
    }
}

int32_t CppRTA::readParams() {
    uint32_t tmpInt, numOutChansFile, numEQsPerChanFile;
    double tmpDouble;
    std::ifstream fStr("params.vdsp", std::ios::binary);

    if (fStr.good()) {
        fStr.read((char*)&numOutChansFile, sizeof(uint32_t));
        fStr.read((char*)&numEQsPerChanFile, sizeof(uint32_t));
        for (uint32_t i=0; i<numOutChans && i<numOutChansFile; i++) {
            for (uint32_t j=0; j<numEQsPerChan && j<numEQsPerChanFile; j++) {
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setSampleRate(tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setGain(tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setFreq(tmpDouble);
                fStr.read((char*)&tmpDouble, sizeof(double));
                EQ.at(i).at(j).setQFactor(tmpDouble);
                fStr.read((char*)&tmpInt, sizeof(uint32_t));
                EQ.at(i).at(j).setType(tmpInt);
            }
            fStr.read((char*)&tmpDouble, sizeof(double));
            limiter.at(i).setThreshold(tmpDouble);
            fStr.read((char*)&tmpDouble, sizeof(double));
            limiter.at(i).setMakeupGain(tmpDouble);
            fStr.read((char*)&tmpDouble, sizeof(double));
            limiter.at(i).setReleaseTime(tmpDouble);
        }
        return 0;
    } else {
        return -1;
    }
}
*/

CppRTA::~CppRTA(void) {
    if (paStream != nullptr) {
        if (Pa_IsStreamActive(paStream)>0) {
            Pa_AbortStream(paStream);
        }
        Pa_CloseStream(paStream);
    }

    Pa_Terminate();
}

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

//------------------------------------------------------------------------------
