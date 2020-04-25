#pragma once

#include <Bisa.h>

#include "HitDistribution.h"

class RandomNoiseDistribution : public HitDistribution
{
public:
    RandomNoiseDistribution(const char* name)
     : HitDistribution(name)
    {
    }

    ~RandomNoiseDistribution()
    {
        extra_configure();
    }

    void addHits(Bisa::HitCollection hits) override
    {
        for (auto &&hit : hits)
        {
            strip_rate_[hit.second->tdc].Fill(strip(*hit.second));
            channel_rate_[hit.second->tdc].Fill(hit.second->channel);
            strip_count_[hit.second->tdc].Fill(strip(*hit.second));
            channel_count_[hit.second->tdc].Fill(hit.second->channel);
        }
        event_counter_++;
    }

    void extra_configure()
    {
        strip_rate_.for_each([&] (TH1F& hist) {
            hist.Scale(1/(0.0000016*event_counter_));
        });
        channel_rate_.for_each([&] (TH1F& hist) {
            hist.Scale(1/(0.0000016*event_counter_));
        });
    }

private:
    int event_counter_ = 0;
};