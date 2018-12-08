/*------------------------------------------------------------------*\
Implmentation of a Cutoff, BiQuad Equalizer and Limiter Class. The EQ design
is based on the EQ cookbook by Robert Bristow Johnson, the cutoff design
is performed via matched z-transform / analogue design with bilinear
transformation of poles/zeros. Cutoffs are calculated in second order
sections.

Author: Hagen Jaeger, 21Sound.

Version 0.1.0 (debugged and tested, 05.07.2019).
\*------------------------------------------------------------------*/

#include <algorithm>
#include "fft.h"
#include "complex_float64.h"
#include "CppDSP.h"

#ifndef M_PI
#define M_PI 3.141592653589793
#endif

CppXover::CppXover(void)
    : fs(44100.), freq(1000.0), charac(FLAT_THRU), type(LOWPASS), ord(2), nSOS(1) {
	int error;

    error = designFilter(this->ord);
    reset();
    if (error <0) {
    	for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
    		setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
    	}
	}
}

CppXover::CppXover(double sampleRate, double freq, filterChar charac, filterType type, uint32_t order)
    : fs(sampleRate), freq(freq), charac(charac), type(type), ord(order), nSOS((order+1)/2) {

	int error;

    error = designFilter(this->ord);
    reset();
    if (error <0) {
    	for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
    		setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
    	}
	}
}

CppXover::~CppXover(void) {

}

int CppXover::designFilter(double ripple, double attenuation) {

	std::vector<complex_float64> poles(ord), zeros(ord);
	complex_float64 one, tmpCmplx;
	double realPart, imagPart, theta, eta, tmp;
	double fg;
	uint32_t ord;

    if (freq < 0) {
		return -1;
    } else if (freq >= fs/2) {
		return -2;
    }

    if (charac == LINKWITZ) {
    	ord = (this->ord+1)/2;
    } else {
    	ord = this->ord;
    }

    if (ord < 0) {
		return -3;
    } else if (ord > 32) {
		return -4;
    }

    if (charac == FLAT_THRU) {
        coeffs.resize(0);
        states.resize(0);
    	return 0;
    }

    fg = fs/M_PI * tan(M_PI * freq / fs);

    nSOS = (ord+1)/2;

    coeffs.resize(nSOS);
    for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
    	coeffs.at(i).resize(NUM_COEFFS_PER_BIQUAD);
    }

    states.resize(nSOS);
    for (uint32_t i=0; i<((uint32_t)states.size()); i++) {
    	states.at(i).resize(NUM_STATES_PER_BIQUAD);
    }

	one.re = 1.0;
	one.im = 0.0;

	if (charac == BUTTERWORTH || charac == LINKWITZ) {
		for (uint32_t i = 0; i<nSOS; i++) {
			theta = (2*i+1)*M_PI / (2*ord);
			realPart = -sin(theta);
			imagPart = cos(theta);
			poles[2*i].re = realPart;
			poles[2*i].im = imagPart;
			if ((2*i+1)<poles.size()) {
				poles[2*i+1].re = realPart;
				poles[2*i+1].im = -imagPart;
			}
		}

		for (uint32_t i = 0; i<poles.size(); i++) {
			poles[i] = complex_mul(poles[i], 2*M_PI*fg);
			tmpCmplx = complex_div(poles[i], 2.0*fs);
			poles[i] = complex_div(complex_add(one, tmpCmplx), complex_add(one, complex_neg(tmpCmplx)));
			zeros[i].re = -1.0;
			zeros[i].im = 0.0;
		}

		if (type == HIGHPASS) {
			for (uint32_t i = 0; i<poles.size(); i++) {
				zeros[i] = complex_neg(zeros[i]);
			}
		}
	} else if (charac == CHEBYSHEV1) {
		for (uint32_t i = 0; i<nSOS; i++) {
				theta = (2*i+1)*M_PI / (2*ord);
				eta = sqrt(pow(10,0.1*ripple)-1.0);
				realPart = -sinh(1.0/ord*asinh(1.0/eta))*sin(theta);
				imagPart = cosh(1.0/ord*asinh(1.0/eta))*cos(theta);
				poles[2*i].re = realPart;
				poles[2*i].im = imagPart;
				if ((2*i+1)<poles.size()) {
					poles[2*i+1].re = realPart;
					poles[2*i+1].im = -imagPart;
				}
			}

			if (type == HIGHPASS) {
				for (uint32_t i = 0; i<poles.size(); i++) {
					poles[i] = complex_div(one, poles[i]);
				}
			}

			for (uint32_t i = 0; i<poles.size(); i++) {
				poles[i] = complex_mul(poles[i], 2*M_PI*fg);
				tmpCmplx = complex_div(poles[i], 2.0*fs);
				poles[i] = complex_div(complex_add(one, tmpCmplx), complex_add(one, complex_neg(tmpCmplx)));
				zeros[i].re = -1.0;
				zeros[i].im = 0.0;
			}

			if (type == HIGHPASS) {
				for (uint32_t i = 0; i<poles.size(); i++) {
					zeros[i] = complex_neg(zeros[i]);
				}
			}
	} else if (charac == CHEBYSHEV2) {
		for (uint32_t i = 0; i<nSOS; i++) {
				theta = (2*i+1)*M_PI / (2*ord);
				eta = 1.0/sqrt(pow(10,0.1*attenuation)-1.0);
				realPart = -sinh(1.0/ord*asinh(1.0/eta))*sin(theta);
				imagPart = cosh(1.0/ord*asinh(1.0/eta))*cos(theta);
				poles[2*i].re = realPart;
				poles[2*i].im = imagPart;
				zeros[2*i].re = 0.0;
				zeros[2*i].im = -cos(theta);
				if ((2*i+1)<poles.size()) {
					poles[2*i+1].re = realPart;
					poles[2*i+1].im = -imagPart;
					zeros[2*i+1].re = 0.0;
					zeros[2*i+1].im = cos(theta);
				}
			}

		if (type == LOWPASS) {
			for (uint32_t i = 0; i<poles.size(); i++) {
				poles[i] = complex_div(one, poles[i]);
				zeros[i] = complex_div(one, zeros[i]);
			}
		}

		for (uint32_t i = 0; i<poles.size(); i++) {
			poles[i] = complex_mul(poles[i], 2*M_PI*fg);
			zeros[i] = complex_mul(zeros[i], 2*M_PI*fg);
			tmpCmplx = complex_div(poles[i], 2.0*fs);
			poles[i] = complex_div(complex_add(one, tmpCmplx), complex_add(one, complex_neg(tmpCmplx)));
			tmpCmplx = complex_div(zeros[i], 2.0*fs);
			zeros[i] = complex_div(complex_add(one, tmpCmplx), complex_add(one, complex_neg(tmpCmplx)));
		}
	}

	for (uint32_t i = 0; i<ord/2; i++) {
		coeffs[i][0] = 1.0;
		coeffs[i][1] = -complex_real(complex_add(zeros[2*i],zeros[2*i+1]));
		coeffs[i][2] = complex_real(complex_mul(zeros[2*i],zeros[2*i+1]));
		coeffs[i][3] = -complex_real(complex_add(poles[2*i],poles[2*i+1]));
		coeffs[i][4] = complex_real(complex_mul(poles[2*i],poles[2*i+1]));
		if (type == LOWPASS) {
			tmp = (1.0 + coeffs[i][3] + coeffs[i][4])/(coeffs[i][0] + coeffs[i][1] + coeffs[i][2]);
		} else if (type == HIGHPASS) {
			tmp = (1.0 - coeffs[i][3] + coeffs[i][4])/(coeffs[i][0] - coeffs[i][1] + coeffs[i][2]);
		}
		for (uint32_t j = 0; j<3; j++) {
			coeffs[i][j] *= tmp;
		}
	}

	if (nSOS*2 != ord) {
		coeffs[nSOS-1][0] = 1.0;
		coeffs[nSOS-1][1] = -complex_real(zeros[ord-1]);
		coeffs[nSOS-1][2] = 0.0;
		coeffs[nSOS-1][3] = -complex_real(poles[ord-1]);
		coeffs[nSOS-1][4] = 0.0;
		if (type == LOWPASS) {
			tmp = (1.0 + coeffs[nSOS-1][3])/(coeffs[nSOS-1][0] + coeffs[nSOS-1][1]);
		} else if (type == HIGHPASS) {
			tmp = (1.0 - coeffs[nSOS-1][3])/(coeffs[nSOS-1][0] - coeffs[nSOS-1][1]);
		}
		for (uint32_t j = 0; j<3; j++) {
			coeffs[nSOS-1][j] *= tmp;
		}
	}

	if (charac == LINKWITZ) {
        coeffs.resize(2*nSOS);
        for (uint32_t i=0; i<2*nSOS; i++) {
        	coeffs.at(i).resize(NUM_COEFFS_PER_BIQUAD, 0.0);
        }
        states.resize(2*nSOS);
        for (uint32_t i=0; i<2*nSOS; i++) {
        	states.at(i).resize(NUM_STATES_PER_BIQUAD, 0.0);
        }
        for (uint32_t i=0; i<nSOS; i++) {
        	for (uint32_t j=0; j<5; j++) {
        		coeffs.at(nSOS+i).at(j) = coeffs.at(i).at(j);
        	}
        }
        nSOS *= 2;
	}

	return 0;
}

std::string CppXover::getTypeName(filterType type)  {
	std::string tmp;
	if(type == LOWPASS) { //LoPass
		tmp = std::string("Lowpass");
	} else if(type == HIGHPASS) { //HiPass
		tmp = std::string("Highpass");
	} else {
		tmp = std::string("Unknown type");
	}
	return tmp;
}

std::string CppXover::getCharName(filterChar charac)  {
	std::string tmp;
	if(charac == FLAT_THRU) { //Flat
		tmp = std::string("Flat");
	} else if(charac == BUTTERWORTH) { //Butterworth
		tmp = std::string("Butter");
	} else if(charac == LINKWITZ) { //Linkwitz Riley
		tmp = std::string("Linkwitz");
	} else if(charac == CHEBYSHEV1) { //Chebyshev I
		tmp = std::string("Cheby 1");
	} else if(charac == CHEBYSHEV2) { //Chebyshev II
		tmp = std::string("Cheby 2");
	} else {
		tmp = std::string("Unknown char.");
	}
	return tmp;
}

void CppXover::process(std::vector<double> &data) {
	  double tmp;
	  for (unsigned int i = 0; i < coeffs.size(); i++) {
		  for (unsigned int j = 0; j < data.size(); j++) {
				tmp = data[j]-states[i][0]*coeffs[i][3]-states[i][1]*coeffs[i][4];
				data[j] = tmp*coeffs[i][0]+states[i][0]*coeffs[i][1]+states[i][1]*coeffs[i][2];
				states[i][1] = states[i][0];
				states[i][0] = tmp;
		  }
	  }
}

int CppXover::addTransferFunction(std::vector<double> &tf, uint32_t nfft) {
    if (ilog2(nfft) == 0){
        return -1;
    }

    if (charac == FLAT_THRU) {
    	return 0;
    }

	std::vector<double> bVec(nfft+2, 0.0), aVec(nfft+2, 0.0);
    double *bp = bVec.data(), *ap = aVec.data();
    complex_float64 *bFreq = (complex_float64*) bp, *aFreq = (complex_float64*) ap;

    for (uint32_t i = 0; i < (uint32_t) coeffs.size(); i++) {
    	std::fill(bVec.begin(), bVec.end(), 0.0);
    	std::fill(aVec.begin(), aVec.end(), 0.0);
		bp[0] = coeffs[i][0];
		bp[1] = coeffs[i][1];
		bp[2] = coeffs[i][2];
		ap[0] = 1.0;
		ap[1] = coeffs[i][3];
		ap[2] = coeffs[i][4];

		fft_double(bp, bFreq, nfft);
		fft_double(ap, aFreq, nfft);

		for (uint32_t i=0; i<nfft/2+1; i++) {
			tf[i]+= 20*log10(complex_abs(complex_div(bFreq[i], aFreq[i])));
		}
    }

	return 0;
}

CppEQ::CppEQ(void)
    : fs(44100.), gain(0.0), freq(1000.0), Q(0.71), type(PEAKEQ) {
	int error;
    coeffs.resize(5, 0.0);
    states.resize(2, 0.0);

    error = designBiquad();
    reset();
    if (error <0) {
        setCoeffs(1.,0.,0.,0.,0.);
    }
}


CppEQ::CppEQ(double sampleRate, double gain, double freq, double Q, eqType type)
    : fs(sampleRate), gain(gain), freq(freq), Q(Q), type(type) {
	int error;
    coeffs.resize(5, 0.0);
    states.resize(2, 0.0);

    error = designBiquad();
    reset();
    if (error <0) {
		setCoeffs(1.,0.,0.,0.,0.);
	}	
}

CppEQ::~CppEQ(void) {

}


int CppEQ::designBiquad() {
    if (freq < 0) {
		return -1;
    }

    if (freq >= fs/2) {
		return -2;
    }

    double w0 = 2*M_PI*freq/fs;
    double alpha = sin(w0)/(2*Q);
    double tmp1, tmp2;

    if(type == LOWPASSEQ) { //LoPass
            tmp1 = (1+alpha);
            coeffs[1] = (1-cos(w0))/tmp1;
            coeffs[0] = (coeffs[1]*0.5);
            coeffs[2] = coeffs[0];
            coeffs[3] = (-2*cos(w0))/tmp1;
            coeffs[4] = (1-alpha)/tmp1;
    } else if(type == HIGHPASSEQ) { //HiPass
            tmp1 = (1+alpha);
            coeffs[1] = -(1+cos(w0))/tmp1;
            coeffs[0] = -coeffs[1]*0.5;
            coeffs[2] = coeffs[0];
            coeffs[3] = (-2*cos(w0))/tmp1;
            coeffs[4] = (1-alpha)/tmp1;
    } else if(type == ALLPASS) { //AllPass
            tmp1 = (1+alpha);
            coeffs[1] = (-2*cos(w0))/tmp1;
            coeffs[0] = (1-alpha)/tmp1;
            coeffs[2] = 1.0;
            coeffs[3] = coeffs[1];
            coeffs[4] = coeffs[0];
    } else if(type == BANDPASS) { //BandPass
            tmp1 = (1+alpha);
            coeffs[1] = 0.0;
            coeffs[0] = (Q*alpha)/tmp1;
            coeffs[2] = -coeffs[0];
            coeffs[3] = (-2*cos(w0))/tmp1;
            coeffs[4] = (1-alpha)/tmp1;
    } else if(type == NOTCH) { //Notch
            tmp1 = (1+alpha);
            coeffs[1] = (-2*cos(w0))/tmp1;
            coeffs[0] = 1/tmp1;
            coeffs[2] = coeffs[0];
            coeffs[3] = coeffs[1];
            coeffs[4] = (1-alpha)/tmp1;
    } else if(type == PEAKEQ) { //peakEqualizer
            tmp1 = pow(10, gain*0.025);
            tmp2 = 1+alpha/tmp1;
            coeffs[1] = (-2*cos(w0))/tmp2;
            coeffs[0] = (1+alpha*tmp1)/tmp2;
            coeffs[2] = (1-alpha*tmp1)/(1+alpha/tmp1);
            coeffs[3] = coeffs[1];
            coeffs[4] = (1-alpha/tmp1)/(1+alpha/tmp1);
    } else if(type == LOWSHELV) { //LowShelv
        tmp1 = pow(10, gain*0.025);
        tmp2 = (tmp1+1)+(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha;
        coeffs[1] = 2*tmp1/tmp2*((tmp1-1)-(tmp1+1)*cos(w0));
        coeffs[0] = tmp1/tmp2*((tmp1+1)-(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha);
        coeffs[2] = tmp1/tmp2*((tmp1+1)-(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha);
        coeffs[3] = (-2*((tmp1-1)+(tmp1+1)*cos(w0)))/tmp2;
        coeffs[4] = ((tmp1+1)+(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha)/tmp2;
    } else if(type == HIGHSHELV) { //HighShelv
        tmp1 = pow(10, gain*0.025);
        tmp2 = (tmp1+1)-(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha;
        coeffs[1] = -2*tmp1/tmp2*((tmp1-1)+(tmp1+1)*cos(w0));
        coeffs[0] = tmp1/tmp2*((tmp1+1)+(tmp1-1)*cos(w0)+2*sqrt(tmp1)*alpha);
        coeffs[2] = tmp1/tmp2*((tmp1+1)+(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha);
        coeffs[3] = (2*((tmp1-1)-(tmp1+1)*cos(w0)))/tmp2;
        coeffs[4] = ((tmp1+1)-(tmp1-1)*cos(w0)-2*sqrt(tmp1)*alpha)/tmp2;
    } else {
        coeffs[0] = 1.0;
        coeffs[1] = 0.0;
        coeffs[2] = 0.0;
        coeffs[1] = 0.0;
        coeffs[2] = 0.0;
        reset();
        return -3;
	}
	return 0;
}

std::string CppEQ::getTypeName(eqType type) {
	std::string tmp;
	if(type == LOWPASSEQ) { //LoPass
		tmp = std::string("Lowpass");
	} else if(type == HIGHPASSEQ) { //HiPass
		tmp = std::string("Highpass");
	} else if(type == ALLPASS) { //AllPass
		tmp = std::string("Allpass");
	} else if(type == BANDPASS) { //BandPass
		tmp = std::string("Bandpass");
	} else if(type == NOTCH) { //Notch
		tmp = std::string("Notch");
	} else if(type == PEAKEQ) { //peakEqualizer
		tmp = std::string("Peak EQ");
	} else if(type == LOWSHELV) { //LowShelv
		tmp = std::string("Lowshelv");
	} else if(type == HIGHSHELV) { //HighShelv
		tmp = std::string("Highshelv");
	} else {
		tmp = std::string("Unknown type");
	}
	return tmp;
}

void CppEQ::process(std::vector<double> &data) {
	  double tmp;
	  for (unsigned int i = 0; i < data.size(); i++) {
	        tmp = data[i]-states[0]*coeffs[3]-states[1]*coeffs[4];
	        data[i] = tmp*coeffs[0]+states[0]*coeffs[1]+states[1]*coeffs[2];
	        states[1] = states[0];
	        states[0] = tmp;
	  }
}

int CppEQ::addTransferFunction(std::vector<double> &tf, uint32_t nfft) {
    if (ilog2(nfft) == 0){
        return -1;
    }

	std::vector<double> bVec(nfft+2, 0.0), aVec(nfft+2, 0.0);
    double *bp = bVec.data(), *ap = aVec.data();
    complex_float64 *bFreq = (complex_float64*) bp, *aFreq = (complex_float64*) ap;

    bp[0] = coeffs[0];
    bp[1] = coeffs[1];
    bp[2] = coeffs[2];
    ap[0] = 1.0;
    ap[1] = coeffs[3];
    ap[2] = coeffs[4];

    fft_double(bp, bFreq, nfft);
    fft_double(ap, aFreq, nfft);

    for (uint32_t i=0; i<nfft/2+1; i++) {
        tf[i]+= 20*log10(complex_abs(complex_div(bFreq[i], aFreq[i])));
    }

	return 0;
}

CppLimiter::CppLimiter(void)
    : fs(44100.0), thres(0.0), makeup(1.0), aRel(1.0-1.0/88200.0),
      lookaheadSamps(96), holdSamps(480), memCnt(0), holdCnt(576),
      logAbsSigRel(0), logAbsSigSmooth(0), compGainLog(0) {

    mem.resize(lookaheadSamps, 0.0);
}

CppLimiter::CppLimiter(double sampleRate, double thres, double makeup, double secRel)
    : fs(sampleRate), thres(thres), makeup(pow(10,makeup*0.05)),
      aRel(1.0-1.0/(secRel*sampleRate)), lookaheadSamps((uint32_t)(0.002*sampleRate)),
      holdSamps((uint32_t)(0.01*sampleRate)), memCnt(0), holdCnt(holdSamps+lookaheadSamps),
      logAbsSigRel(0), logAbsSigSmooth(0), compGainLog(0) {

    mem.resize(lookaheadSamps, 0.0);
}

CppLimiter::~CppLimiter(void) {

}

void CppLimiter::process(std::vector<double> &data)
{
    double tmp, aRelHold, logAbsSig;

    for (uint32_t i = 0; i < data.size(); i++)
    {
        data[i] *= makeup;

        logAbsSig = fmax(0., 20*log10(fabs(data[i]))-thres);

        if (holdCnt > 0) {
            aRelHold = 1;
        } else {
            aRelHold = aRel;
        }

        logAbsSigRel = fmax(logAbsSig, (1-aRelHold)*logAbsSig+aRelHold*logAbsSigRel);

        if (logAbsSigRel-logAbsSig > 0.001) {
            holdCnt = (int32_t) fmax(0, holdCnt-1);
        } else {
            holdCnt = holdSamps+lookaheadSamps;
        }

        compGainLog = fmin(logAbsSigRel, compGainLog+logAbsSigRel/lookaheadSamps);

        tmp = data[i];
        data[i] = mem[memCnt]*pow(10, -compGainLog*0.05);
        mem[memCnt] = tmp;

        memCnt++;

        if (memCnt >= lookaheadSamps) {
            memCnt = 0;
        }
    }
}
