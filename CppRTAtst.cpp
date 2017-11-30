/*----------------------------------------------------------------------------*\
Test routine for the C++ real time audio class. Lowpassfilters the stream for
the first half of the time.

Author: (c) Uwe Simmer, Hagen Jaeger    January 2017 - Now
\*----------------------------------------------------------------------------*/

#include <chrono>
#include <thread>
#include <stdio.h>
#include "CppRTA.h"

#define BLOCKLEN 128
#define NUM_BUFFERS 1

int effect = 1;

void lowPass(double *buffer, int numChannels, int numFrames)
{
    static double State[2];
    int iChanCnt, iSampCnt;

    if (effect == 0)
        return;

    for (iSampCnt = 0; iSampCnt<(numChannels*numFrames); iSampCnt += numChannels)
    {
        for (iChanCnt = 0; iChanCnt<numChannels; iChanCnt++)
        {
            buffer[iSampCnt + iChanCnt] = 0.05*buffer[iSampCnt + iChanCnt] + 0.95*State[iChanCnt];
            State[iChanCnt] = buffer[iSampCnt + iChanCnt];
        }
    }
}

int main()
{
    int count, watchDog;
    int deviceNr = -1;
    const char* sFilePath = "D:/Qt/QtProjects/Philip George - Wish You Were Mine (Original Mix).mp3";

    CppRTA *cppRTA = new CppRTA(BLOCKLEN, NUM_BUFFERS, lowPass);

    for (count = 0; count<cppRTA->getNumDevices(); count++)
        printf("Device %i: %s \n", count, cppRTA->getDeviceInfoStr(count)->c_str());

    cppRTA->streamFromFile(sFilePath, deviceNr);

    for (watchDog = 0; watchDog < 1000; watchDog++) //1000*0.01 sec play time
    {
        if (watchDog == 500)
            effect = 0;

        std::this_thread::sleep_for(std::chrono::microseconds(10000));
    }

    delete cppRTA;

    printf("\n");
    return 0;
}

//--------------------- License ------------------------------------------------

// Copyright (c) 2017 Uwe Simmer, Hagen Jaeger

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
