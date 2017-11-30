/*------------------------------------------------------------------*\
Interface to a BiQuad Equalizer Class. The design is based on the EQ cookbook
by Robert bristow Johnson.
public domain 

Version 1.0.1 (debuuged and tested, 29.09.2017).
\*------------------------------------------------------------------*/

#ifndef _DSP_H // include guard
#define _DSP_H

#include <vector>
#include <cmath>
#include <cstdint>

class CppEQ {

public:
    CppEQ(void);

    CppEQ(double sampleRate, double gain, double freq, double Q, int32_t type);

    ~CppEQ(void);

    inline void setSampleRate(double fs) {
        m_fs = fs;
        int32_t error = DesignPeakEQ();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        }
    }

    inline void setGain(double gain) {
        m_gain = gain;
        int32_t error = DesignPeakEQ();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        }
    }

    inline void setFreq(double freq) {
        m_freq = freq;
        int32_t error = DesignPeakEQ();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        }
    }

    inline void setQFactor(double Q) {
        m_Q = Q;
        int32_t error = DesignPeakEQ();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        }
    }

    inline void setType(int32_t type) {
        m_type = type;
        int32_t error = DesignPeakEQ();
        if (error <0) {
            setCoeffs(1.,0.,0.,0.,0.);
        }
    }

    void process(std::vector<double> &data);

protected:
    int32_t DesignPeakEQ();

    inline void reset() {
        for (int32_t i = 0 ; i < 2 ; i++) {
            for (int32_t j = 0 ; j < 2; j++) {
                m_states[i][j] = 0.0;
            }
        }
    }

    inline void setCoeffs(double b0, double b1, double b2, double a1, double a2) {
        m_b[0] = b0;
        m_b[1] = b1;
        m_b[2] = b2;
        m_a[0] = 1.0;
        m_a[1] = a1;
        m_a[2] = a2;
    }

private:
    std::vector< std::vector<double> > m_states;
    std::vector<double> m_b, m_a;
    double m_gain, m_freq, m_fs, m_Q;
    uint32_t m_type;
};

class CppLimiter {

public:
    CppLimiter(void);

    CppLimiter(double sampleRate, double thres, double makeup, double secRel);

    ~CppLimiter(void);

    inline int32_t setSampleRate(double fs) {
        if (fs<0) {
            return -1;
        } else {
            m_fs = fs;
            m_holdSamps = uint32_t(0.01*m_fs);
            m_lookaheadSamps = uint32_t(0.002*m_fs);
            m_mem.resize(m_lookaheadSamps, 0.0);
            return 0;
        }
    }

    inline int32_t setThreshold(double thres) {
        if (thres<-90.0) {
            return -1;
        } else {
            m_thres = thres;
        }
    }

    inline int32_t setMakeupGain(double makeupGainLog)  {
        if (makeupGainLog < -50.0 || makeupGainLog > 50.0) {
            return -1;
        } else {
            m_makeup = std::pow(10,makeupGainLog*0.05);
        }
    }

    inline int32_t setReleaseTime(double secRel) {
        if (secRel>10.0 || secRel < 0.01) {
            return -1;
        } else {
            m_aRel = 1.0-1.0/(secRel*m_fs);
        }
    }

    void process(std::vector<double> &data);

private:
    std::vector<double> m_mem;
    double m_fs, m_thres, m_makeup, m_aRel, m_logAbsSigRel, m_logAbsSigSmooth, m_compGainLog;
    uint32_t m_lookaheadSamps, m_holdSamps;
    int32_t m_memCnt, m_holdCnt;
};

#endif // end of include guard
