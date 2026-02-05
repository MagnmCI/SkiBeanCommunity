/*
 * Copyright (c) 2026 Chris Ice
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
