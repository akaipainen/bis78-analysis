#pragma once

#include <Bisa.h>

#include "HitDistribution.h"

class FirstHitsDistribution : public HitDistribution
{
public:
    FirstHitsDistribution(const char* name, double run_duration)
     : HitDistribution(name)
     , run_duration_(run_duration)
    {
    }

    ~FirstHitsDistribution()
    {
        extra_configure();
    }

    void extra_configure()
    {
        strip_rate_.for_each([&] (TH1F& hist) {
            hist.Scale(1/(run_duration_));
        });
        channel_rate_.for_each([&] (TH1F& hist) {
            hist.Scale(1/(run_duration_));
        });
    }

private:
    double run_duration_;
};