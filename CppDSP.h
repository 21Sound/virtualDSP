/*------------------------------------------------------------------*\
Interface to a Cutoff, BiQuad Equalizer and Limiter Class. The EQ design
is based on the EQ cookbook by Robert Bristow Johnson, the cutoff design
is performed via matched z-transform / analogue design with bilinear
transformation of poles/zeros. Cutoffs are calculated in second order
sections.

public domain 

Version 0.1.0 (debugged and tested, 05.07.2019).
\*------------------------------------------------------------------*/

#ifndef _DSP_H // include guard
#define _DSP_H

#define NUM_COEFFS_PER_BIQUAD 5
#define NUM_STATES_PER_BIQUAD 2

#include <vector>
#include <string>
#include <cmath>
#include <cstdint>

typedef enum {
	FLAT_THRU = 0x0,
    BUTTERWORTH,
	LINKWITZ,
    CHEBYSHEV1,
    CHEBYSHEV2,
	UNKNOWN_FILTERCHAR
} filterChar;

typedef enum {
	LOWPASS = 0x10,
	HIGHPASS,
	UNKNOWN_FILTERTYPE
} filterType;

typedef enum {
    LOWPASSEQ = 0x20,
    HIGHPASSEQ,
    ALLPASS,
    BANDPASS,
	NOTCH,
	PEAKEQ,
	LOWSHELV,
	HIGHSHELV,
	UNKNOWN_EQTYPE
} eqType;

class CppXover {

public:
    CppXover(void);

    CppXover(double sampleRate, double freq, filterChar charac, filterType type, uint32_t order);

    ~CppXover(void);

    inline int setSampleRate(double fs) {
    	int error;
    	this->fs = fs;
        error = designFilter();
        if (error <0) {
        	for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
        		setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
        	}
        	return -1;
        }
        return 0;
    }

    inline int setFreq(double freq) {
    	int error;
    	this->freq = freq;
        error = designFilter();
        if (error <0) {
        	for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
        		setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
        	}
        	return -1;
		}
		return 0;
    }

    inline int setOrder(uint32_t ord) {
    	int error;
		this->ord = ord;

        this->nSOS = (ord+1)/2;

        error = designFilter();
        if (error <0) {
        	for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
        		setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
        	}
        	return -1;
		}
		return 0;
    }

    inline int setChar(filterChar charac, double ripple = 1.0, double attenuation = 40.0) {
    	int error = 0;
    	this->charac = charac;

		error = designFilter(ripple, attenuation);
		if (error <0) {
			for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
				setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
			}
			return -1;
		}
		return 0;
	}

    inline int setType(filterType type) {
    	int error;
    	this->type = type;

        error = designFilter(this->ord);
        if (error <0) {
        	for (uint32_t i=0; i<((uint32_t)coeffs.size()); i++) {
        		setCoeffsOfSOS(i,1.,0.,0.,0.,0.);
        	}
        	return -1;
		}
		return 0;
    }

    uint32_t getOrd() const { return this->ord; }
    double getFreq() const { return this->freq; }
    filterType getType() const { return this->type; }
    filterChar getChar() const { return this->charac; }

    static std::string getTypeName(filterType type);
    static std::string getCharName(filterChar charac);

    void process(std::vector<double> &data);

    int addTransferFunction(std::vector<double> &tf, uint32_t nfft);

protected:
    int designFilter(double ripple = 1.0, double attenuation = 40.0);

    inline void reset() {
    	for (uint32_t i = 0 ; i < states.size(); i++) {
			for (uint32_t j = 0 ; j < NUM_STATES_PER_BIQUAD ; j++) {
				states[i][j] = 0.0;
			}
    	}
    }

    inline int setCoeffsOfSOS(int sosID, double b0, double b1, double b2, double a1, double a2) {
    	if (sosID > coeffs.size()) {
    		return -1;
    	} else {
			coeffs[sosID][0] = b0;
			coeffs[sosID][1] = b1;
			coeffs[sosID][2] = b2;
			coeffs[sosID][3] = a1;
			coeffs[sosID][4] = a2;
    	}
    	return 0;
    }

private:
    std::vector< std::vector<double> > states;
    std::vector< std::vector<double> > coeffs;
    double freq, fs;
    uint32_t ord, nSOS;
    filterChar charac;
    filterType type ;

};

class CppEQ {

public:
    CppEQ(void);

    CppEQ(double sampleRate, double gain, double freq, double Q, eqType type);

    ~CppEQ(void);

    inline int setSampleRate(double fs) {
    	int error;
    	this->fs = fs;
        error = designBiquad();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        	return -1;
		}
		return 0;
    }

    inline int setGain(double gain) {
    	int error;
        this->gain = gain;
        error = designBiquad();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        	return -1;
		}
		return 0;
    }

    inline int setFreq(double freq) {
    	int error;
        this->freq = freq;
        error = designBiquad();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        	return -1;
		}
		return 0;
    }

    inline int setQFactor(double Q) {
    	int error;
        this->Q = Q;
        error = designBiquad();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        	return -1;
		}
		return 0;
    }

    inline int setType(eqType type) {
    	int error;
        this->type = type;
        error = designBiquad();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        	return -1;
		}
		return 0;
    }

    double getGain() const { return this->gain; }
    double getFreq() const { return this->freq; }
    double getQFact() const { return this->Q; }
    eqType getType() const { return this->type; }

    static std::string getTypeName(eqType type);

    void process(std::vector<double> &data);

    int addTransferFunction(std::vector<double> &tf, uint32_t nfft);

protected:
    int designBiquad();

    inline void reset() {
        for (int32_t i = 0 ; i < NUM_STATES_PER_BIQUAD ; i++) {
            states[i] = 0.0;
        }
    }

    inline void setCoeffs(double b0, double b1, double b2, double a1, double a2) {
        coeffs[0] = b0;
        coeffs[1] = b1;
        coeffs[2] = b2;
        coeffs[0] = 1.0;
        coeffs[1] = a1;
        coeffs[2] = a2;
    }

private:
    std::vector<double> states;
    std::vector<double> coeffs;
    double gain, freq, fs, Q;
    eqType type;
};

class CppLimiter {

public:
    CppLimiter(void);

    CppLimiter(double sampleRate, double thres, double makeup, double secRel);

    ~CppLimiter(void);

    inline int setSampleRate(double fs) {
    	double secRel = 0.0;
        if (fs<0) {
            return -1;
        } else {
        	secRel = 1.0/((1.0-aRel)*this->fs);
            this->fs = fs;
            aRel = 1.0-1.0/(secRel*fs);
            holdSamps = uint32_t(0.01*fs);
            lookaheadSamps = uint32_t(0.002*fs);
            mem.resize(lookaheadSamps, 0.0);
            return 0;
        }
    }

    inline int setThreshold(double thres) {
        if (thres<-90.0) {
            return -1;
        } else {
            this->thres = thres;
			return 0;
        }
    }

    inline int setMakeupGain(double makeupGainLog)  {
        if (makeupGainLog < -50.0 || makeupGainLog > 50.0) {
            return -1;
        } else {
            this->makeup = pow(10.0, makeupGainLog*0.05);
			return 0;
        }
    }

    inline int setReleaseTime(double secRel) {
        if (secRel>10.0 || secRel < 0.01) {
            return -1;
        } else {
            aRel = 1.0-1.0/(secRel*fs);
			return 0;
        }
    }

    double getThres() const { return thres; }
    double getMakeup() const { return 20.0*log10(makeup); }
    double getReleaseTime() const { return 1.0/((1.0-aRel)*fs); }

    void process(std::vector<double> &data);

private:
    std::vector<double> mem;
    double fs, thres, makeup, aRel, logAbsSigRel, logAbsSigSmooth, compGainLog;
	uint32_t lookaheadSamps, holdSamps, memCnt;
	int32_t holdCnt;
};

#endif // end of include guard
