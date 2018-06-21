/*----------------------------------------------------------------------------*\
Real time audio streaming class, uses portaudio, libsndfile and mpg123.

Author: (c) Hagen Jaeger    January 2017 - Now
\*----------------------------------------------------------------------------*/

#include <stdexcept>
#include <sstream>
#include "CppRTA.h"
#include <iostream>

CppRTA::CppRTA(deviceContainerRTA inDev, deviceContainerRTA outDev, uint32_t blockLen, uint32_t fs)
    : paInStream(nullptr), paOutStream(nullptr), paDuplexStream(nullptr), blockLen(blockLen), fs(fs),
	  inDev(inDev), outDev(outDev) {

    if (this->blockLen < 0x20) {
        this->blockLen = 0x20;
    }

    limiter.resize(outDev.numChans);
    EQ.resize(outDev.numChans);
    for (uint32_t i=0; i<outDev.numChans; i++) {
        limiter.at(i).setSampleRate(fs);
        EQ.at(i).resize(1);
        EQ.at(i).at(0).setSampleRate(fs);
    }

    inData.resize(inDev.numChans);
    for (uint32_t i=0; i<inDev.numChans; i++) {
        inData.at(i).resize(blockLen, 0.0);
    }

    outData.resize(outDev.numChans);
    for (uint32_t i=0; i<outDev.numChans; i++) {
        outData.at(i).resize(blockLen, 0.0);
    }
}

void CppRTA::startStream() {
    PaStreamParameters inParams, outParams;
    PaError paErr = paNoError;

    paErr = Pa_Initialize();
    if (paErr != paNoError) {
        throw std::invalid_argument(std::string(Pa_GetErrorText(paErr)));
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

    paErr = Pa_OpenStream(&paDuplexStream, &inParams, &outParams, fs, blockLen, paNoFlag, duplexCallback, this);
    if(paErr != paNoError) {
    	paErr = Pa_OpenStream(&paInStream, &inParams, nullptr, fs, blockLen, paNoFlag, inCallback, this);
    	if(paErr != paNoError) {
    		throw std::invalid_argument(std::string(Pa_GetErrorText(paErr)));
    	}
        paErr = Pa_OpenStream(&paOutStream, nullptr, &outParams, fs, blockLen, paNoFlag, outCallback, this);
    	if(paErr != paNoError) {
    		throw std::invalid_argument(std::string(Pa_GetErrorText(paErr)));
    	}
    }

    paErr = Pa_StartStream(paDuplexStream);
    if(paErr != paNoError) {
    	paErr = Pa_StartStream(paInStream);
    	if(paErr != paNoError) {
			throw std::invalid_argument(std::string(Pa_GetErrorText(paErr)));
		}
    	paErr = Pa_StartStream(paOutStream);
    	if(paErr != paNoError) {
			throw std::invalid_argument(std::string(Pa_GetErrorText(paErr)));
		}
    }
}

void CppRTA::stopStream() {
    if (paDuplexStream != nullptr) {
        if (Pa_IsStreamActive(paDuplexStream)>0) {
            Pa_AbortStream(paDuplexStream);
        }
        Pa_CloseStream(paDuplexStream);
    }

    if (paInStream != nullptr) {
        if (Pa_IsStreamActive(paInStream)>0) {
            Pa_AbortStream(paInStream);
        }
        Pa_CloseStream(paInStream);
    }

    if (paOutStream != nullptr) {
        if (Pa_IsStreamActive(paOutStream)>0) {
            Pa_AbortStream(paOutStream);
        }
        Pa_CloseStream(paOutStream);
    }

    Pa_Terminate();
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
        for (uint32_t j = 0; j<obj->inDev.numChans; j++) {
            obj->inData[j][i] = recData[i*obj->inDev.numChans+j];
        }
    }

    for (uint32_t i = 0; i<obj->outDev.numChans; i++) {
        obj->outData[i] = obj->inData[i%obj->inDev.numChans];
        for (uint32_t j=0; j<obj->EQ[i].size(); j++) {
            obj->EQ[i][j].process(obj->outData[i]);
        }
        obj->limiter[i].process(obj->outData[i]);
    }

    for (uint32_t i = 0; i<obj->blockLen; i++) {
        for (uint32_t j = 0; j<obj->outDev.numChans; j++) {
            playData[i*obj->outDev.numChans+j] = (float) obj->outData[j][i];
        }
    }
    return paContinue;
}

int32_t CppRTA::inCallback(const void *inBuf, void *outBuf,
                           unsigned long framesPerBuf,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlag,
                           void *userData)
{
    (void) timeInfo; (void) statusFlag; (void) outBuf;

    CppRTA* obj = (CppRTA*) userData;
    float *recData = (float*) inBuf;

    for (uint32_t i = 0; i<obj->blockLen; i++) {
        for (uint32_t j = 0; j<obj->inDev.numChans; j++) {
            obj->inData[j][i] = recData[i*obj->inDev.numChans+j];
        }
    }
    return paContinue;
}

int32_t CppRTA::outCallback(const void *inBuf, void *outBuf,
                           unsigned long framesPerBuf,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlag,
                           void *userData)
{
    (void) timeInfo; (void) statusFlag; (void) inBuf;

    CppRTA* obj = (CppRTA*) userData;
    float *playData = (float*) outBuf;

    for (uint32_t i = 0; i<obj->outDev.numChans; i++) {
        obj->outData[i] = obj->inData[i%obj->inDev.numChans];
        for (uint32_t j=0; j<obj->EQ[i].size(); j++) {
            obj->EQ[i][j].process(obj->outData[i]);
        }
        obj->limiter[i].process(obj->outData[i]);
    }

    for (uint32_t i = 0; i<obj->blockLen; i++) {
        for (uint32_t j = 0; j<obj->outDev.numChans; j++) {
            playData[i*obj->outDev.numChans+j] = (float) obj->outData[j][i];
        }
    }
    return paContinue;
}

int32_t CppRTA::getHostAPIs(std::vector<std::string> &apis) {
    PaError paErr;
    const PaDeviceInfo *paDevInfo;
    PaDeviceIndex numDevices;
    PaHostApiIndex numAPIs;

    paErr = Pa_Initialize();
    if (paErr != paNoError) {
        return -1;
    }

    numDevices = Pa_GetDeviceCount();
    if (Pa_GetHostApiCount() < 1) {
        return -2;
    }

    numAPIs = Pa_GetHostApiCount();
    for (PaHostApiIndex i=0; i<numAPIs; i++) {
        apis.push_back(std::string(Pa_GetHostApiInfo(i)->name));
    }

    Pa_Terminate();

	return 0;
}

int32_t CppRTA::getDevices(std::vector<deviceContainerRTA> &inDevices,
                           std::vector<deviceContainerRTA> &outDevices) {
    PaError paErr;
    const PaDeviceInfo *paDevInfo;
    PaDeviceIndex numDevices;
    deviceContainerRTA devInfo;
    const uint32_t maxNameLen = 50;

    paErr = Pa_Initialize();
    if (paErr != paNoError) {
        return -1;
    }

    numDevices = Pa_GetDeviceCount();
    if (numDevices < 1) {
        return -2;
    }

    for (PaDeviceIndex i=0; i<numDevices; i++) {
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
            devInfo.name = std::string("(") + std::to_string(devInfo.ID) + std::string(") ") + devInfo.name
            		+ std::string("  |  ") + std::to_string(devInfo.numChans) + std::string(" channels");
            inDevices.push_back(devInfo);
        }

        if (paDevInfo->maxOutputChannels>0) {
            devInfo.numChans = paDevInfo->maxOutputChannels;
             devInfo.latency = paDevInfo->defaultLowOutputLatency;
            devInfo.inputFlag = false;
            devInfo.name = std::string("(") + std::to_string(devInfo.ID) + std::string(") ") + devInfo.name
            		+ std::string("  |  ") + std::to_string(devInfo.numChans) + std::string(" channels");
            outDevices.push_back(devInfo);
        }
    }

    Pa_Terminate();

	return 0;
}

int CppRTA::getTransferFunction(std::vector<double> &tf, uint32_t chanID, uint32_t nfft) {
    int returnID = 0;
    if (chanID>=EQ.size()) {
        return -1;
    }
    if (tf.size() != nfft/2+1) {
        tf.resize(nfft/2+1, 0.0);
    }

    for (uint32_t i=0; i<EQ.at(chanID).size(); i++) {
        returnID += EQ.at(chanID).at(i).addTransferFunction(tf, nfft);
    }

	return 0;
}

CppRTA::~CppRTA(void) {
    this->stopStream();
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
