/*------------------------------------------------------------------*\
Implmentation of a Peak Equalizer Class
the design is based on the EQ cookbook by Robert bristow Johnson
public domain

Author: Joerg Bitzer (TGM) (Jade-Hochschule) 

Modified by Hagen Jaeger, 22.09.2014, now with all cookbook-variations of a BiQuad
(LoPass, HiPass, LoShelv, HiShelf, pEQ, ...). Modification is based on Joerg Bitzers 
'filterdesign.m', i.e. RBJ Cookbook, for MATLAB.

Version 1.0.1 (debuuged and tested, 22.09.2014).
\*------------------------------------------------------------------*/

#include <algorithm>
#include "fft.h"
#include "complex_float64.h"
#include "CppDSP.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

CppEQ::CppEQ(void)
    : m_fs(48000.), m_gain(0.0), m_freq(1000.0), m_Q(1.0), m_type(5) {

    m_b.resize(3, 0.0);
    m_a.resize(3, 0.0);
    m_states.resize(2);
    m_states.at(0).resize(2);
    m_states.at(1).resize(2);

    int32_t error = DesignPeakEQ();
    this->reset();
    if (error <0) {
        setCoeffs(1.,0.,0.,0.,0.);
    }
}

CppEQ::CppEQ(double sampleRate, double gain, double freq, double Q, int32_t type)
    : m_fs(sampleRate), m_gain(gain), m_freq(freq), m_Q(Q), m_type(type) {

    m_b.resize(3, 0.0);
    m_a.resize(3, 0.0);
    m_states.resize(2);
    m_states.at(0).resize(2);
    m_states.at(1).resize(2);

    int32_t error = DesignPeakEQ();
    this->reset();
    if (error <0) {
		setCoeffs(1.,0.,0.,0.,0.);
	}	
}

CppEQ::~CppEQ(void) {

}


int32_t CppEQ::DesignPeakEQ() {
    if (m_freq < 0) {
		return -1;
    }

    if (m_freq >= m_fs/2) {
		return -2;
    }

    double w0 = 2*M_PI*m_freq/m_fs;
    double alpha = sin(w0)/(2*m_Q);
    double tmp1, tmp2;

    m_a[0] = 1;
    if(m_type == 0) { //LoPass
            tmp1 = (1+alpha);
            m_b[1] = (1-cos(w0))/tmp1;
            m_b[0] = (m_b[1]*0.5);
            m_b[2] = m_b[0];
            m_a[1] = (-2*cos(w0))/tmp1;
            m_a[2] = (1-alpha)/tmp1;
    } else if(m_type == 1) { //HiPass
            tmp1 = (1+alpha);
            m_b[1] = -(1+cos(w0))/tmp1;
            m_b[0] = -m_b[1]*0.5;
            m_b[2] = m_b[0];
            m_a[1] = (-2*cos(w0))/tmp1;
            m_a[2] = (1-alpha)/tmp1;
    } else if(m_type == 2) { //AllPass
            tmp1 = (1+alpha);
            m_b[1] = (-2*cos(w0))/tmp1;
            m_b[0] = (1-alpha)/tmp1;
            m_b[2] = 1.0;
            m_a[1] = m_b[1];
            m_a[2] = m_b[0];
    } else if(m_type == 3) { //BandPass
            tmp1 = (1+alpha);
            m_b[1] = 0.0;
            m_b[0] = (m_Q*alpha)/tmp1;
            m_b[2] = -m_b[0];
            m_a[1] = (-2*cos(w0))/tmp1;
            m_a[2] = (1-alpha)/tmp1;
    } else if(m_type == 4) { //Notch
            tmp1 = (1+alpha);
            m_b[1] = (-2*cos(w0))/tmp1;
            m_b[0] = 1/tmp1;
            m_b[2] = m_b[0];
            m_a[1] = m_b[1];
            m_a[2] = (1-alpha)/tmp1;
    } else if(m_type == 5) { //peakEqualizer
            tmp1 = std::pow(10, m_gain*0.025);
            tmp2 = 1+alpha/tmp1;
            m_b[1] = (-2*cos(w0))/tmp2;
            m_b[0] = (1+alpha*tmp1)/tmp2;
            m_b[2] = (1-alpha*tmp1)/(1+alpha/tmp1);
            m_a[1] = m_b[1];
            m_a[2] = (1-alpha/tmp1)/(1+alpha/tmp1);
    } else if(m_type == 6) { //LowShelv
        tmp1 = std::pow(10, m_gain*0.025);
        tmp2 = (tmp1+1)+(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha;
        m_b[1] = 2*tmp1/tmp2*((tmp1-1)-(tmp1+1)*cos(w0));
        m_b[0] = tmp1/tmp2*((tmp1+1)-(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha);
        m_b[2] = tmp1/tmp2*((tmp1+1)-(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha);
        m_a[1] = (-2*((tmp1-1)+(tmp1+1)*cos(w0)))/tmp2;
        m_a[2] = ((tmp1+1)+(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha)/tmp2;
    } else if(m_type == 7) { //HighShelv
        tmp1 = std::pow(10, m_gain*0.025);
        tmp2 = (tmp1+1)-(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha;
        m_b[1] = -2*tmp1/tmp2*((tmp1-1)+(tmp1+1)*cos(w0));
        m_b[0] = tmp1/tmp2*((tmp1+1)+(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha);
        m_b[2] = tmp1/tmp2*((tmp1+1)+(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha);
        m_a[1] = (2*((tmp1-1)-(tmp1+1)*cos(w0)))/tmp2;
        m_a[2] = ((tmp1+1)-(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha)/tmp2;
    } else {
        m_b[0] = 1.0;
        m_b[1] = 0.0;
        m_b[2] = 0.0;
        m_a[1] = 0.0;
        m_a[2] = 0.0;
        this->reset();
        return -3;
	}
	return 0;
}

void CppEQ::process(std::vector<double> &data) {
    double tmp;
    for (uint32_t i = 0; i < data.size(); i++) {
        tmp = m_b[0]*data[i] + m_b[1]*m_states[0][0] + m_b[2]*m_states[0][1]
            - m_a[1]*m_states[1][0] - m_a[2]*m_states[1][1];

        m_states[0][1] = m_states[0][0];
        m_states[0][0] = data[i];
        m_states[1][1] = m_states[1][0];
        m_states[1][0] = tmp;

        data[i] = tmp;
    }
}

int CppEQ::addTransferFunction(std::vector<double> &tf, uint32_t nfft) {
    if (ilog2(nfft) == 0){
        return -1;
    }

	std::vector<double> bVec(nfft+2, 0.0), aVec(nfft+2, 0.0);
    double *b = bVec.data(), *a = aVec.data();
    complex_float64 *bFreq = (complex_float64*) b, *aFreq = (complex_float64*) a;

    b[0] = m_b[0];
    b[1] = m_b[1];
    b[2] = m_b[2];
    a[0] = m_a[0];
    a[1] = m_a[1];
    a[2] = m_a[2];

    fft_double(b, (complex_float64*) bFreq, nfft);
    fft_double(a, (complex_float64*) aFreq, nfft);

    for (uint32_t i=0; i<nfft/2+1; i++) {
        tf[i]+= 20*std::log10(complex_abs(complex_div(bFreq[i], aFreq[i])));
    }

	return 0;
}

CppLimiter::CppLimiter(void)
    : m_fs(48000.0), m_thres(0.0), m_makeup(1.0), m_aRel(1.0-1.0/96000.0),
      m_lookaheadSamps(96), m_holdSamps(480), m_memCnt(0), m_holdCnt(576),
      m_logAbsSigRel(0), m_logAbsSigSmooth(0), m_compGainLog(0) {

    m_mem.resize(m_lookaheadSamps, 0.0);
}

CppLimiter::CppLimiter(double sampleRate, double thres, double makeup, double secRel)
    : m_fs(sampleRate), m_thres(thres), m_makeup(std::pow(10,makeup*0.05)),
      m_aRel(1.0-1.0/(secRel*sampleRate)), m_lookaheadSamps((uint32_t)(0.002*sampleRate)),
      m_holdSamps((uint32_t)(0.01*sampleRate)), m_memCnt(0), m_holdCnt(m_holdSamps+m_lookaheadSamps),
      m_logAbsSigRel(0), m_logAbsSigSmooth(0), m_compGainLog(0) {

    m_mem.resize(m_lookaheadSamps, 0.0);
}

CppLimiter::~CppLimiter(void) {

}

void CppLimiter::process(std::vector<double> &data)
{
    double tmp, aRelHold, logAbsSig;

    for (uint32_t i = 0; i < data.size(); i++)
    {
        data[i] *= m_makeup;

        logAbsSig = std::max(0., 20*std::log10(std::abs(data[i]))-m_thres);

        if (m_holdCnt > 0) {
            aRelHold = 1;
        } else {
            aRelHold = m_aRel;
        }

        m_logAbsSigRel = std::max(logAbsSig, (1-aRelHold)*logAbsSig+aRelHold*m_logAbsSigRel);

        if (m_logAbsSigRel-logAbsSig > 0.001) {
            m_holdCnt = std::max(0, m_holdCnt-1);
        } else {
            m_holdCnt = m_holdSamps+m_lookaheadSamps;
        }

        m_compGainLog = std::min(m_logAbsSigRel, m_compGainLog+m_logAbsSigRel/m_lookaheadSamps);

        tmp = data[i];
        data[i] = m_mem[m_memCnt]*std::pow(10, -m_compGainLog*0.05);
        m_mem[m_memCnt] = tmp;

        m_memCnt++;

        if (m_memCnt >= m_lookaheadSamps) {
            m_memCnt = 0;
        }
    }
}
