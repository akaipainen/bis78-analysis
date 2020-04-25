#include "Application.h"

#include <TStyle.h>
#include <TError.h>

namespace Bisa {
    
    Application::Application()
    {
        hits_ = CreateScope<HitCollection>();

        auto props = StreamProps("/Users/alex/workspace/cern/Bisa/data/ModA04_BIS7_IntMDT_autotrigger_5700V_040320201053.dat");
        dataStream_ = CreateScope<DataStream>(props);
        dataStream_->set_new_data_callback([&](HitCollection& hits) {
            // BA_CORE_INFO("Added {0} hits to event", hits.size());
            *hits_ = hits;
        });

        gStyle->SetHistMinimumZero();
        gErrorIgnoreLevel = kWarning;
    }

    Application::~Application()
    {
    }

    void Application::Run()
    {
        for (int i = 0; i < 5; i++)
        {
            auto ok = dataStream_->fill_next_event();
            BA_CORE_INFO("Data found: {}", ok);
            for (auto &&h : *hits_)
            {
                BA_CORE_INFO("{}", h.to_string());
            }

            Step();
        }
    }

    void Application::Step()
    {

    }

}
