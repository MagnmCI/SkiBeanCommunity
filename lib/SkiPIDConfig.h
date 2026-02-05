#pragma once

#include <PID_v1.h>   // Arduino PID library

class PIDConfig
{
public:
    PIDConfig()
        : kP_(9.0),
          kI_(0.3),
          kD_(2.5),
          sampleTime_(500L),
          pMode_(P_ON_M),
          maxPower_(100)
    {
    }

    // --- Getters ---
    double getKp() const { return kP_; }
    double getKi() const { return kI_; }
    double getKd() const { return kD_; }
    int   getSampleTime() const { return sampleTime_; }
    int    getPMode() const { return pMode_; }
    int    getMaxPower() const { return maxPower_; }

    // --- Setters ---
    void setKp(double kp) { kP_ = kp; }
    void setKi(double ki) { kI_ = ki; }
    void setKd(double kd) { kD_ = kd; }

    void setSampleTime(int sampleTime)
    {
        if (sampleTime > 0)
            sampleTime_ = sampleTime;
    }

    void setPMode(int mode)
    {
        if (mode == P_ON_E || mode == P_ON_M)
            pMode_ = mode;
    }

    void setMaxPower(int maxPower)
    {
        if (maxPower >= 0 && maxPower <= 100)
            maxPower_ = maxPower;
    }

    // --- Apply to Arduino PID ---
    void apply(PID& pid) const
    {
        pid.SetTunings(kP_, kI_, kD_, pMode_);
        pid.SetSampleTime(sampleTime_);
        pid.SetOutputLimits(0, maxPower_);
    }

private:
    double kP_;
    double kI_;
    double kD_;
    int   sampleTime_;
    int    pMode_;
    int    maxPower_;
};