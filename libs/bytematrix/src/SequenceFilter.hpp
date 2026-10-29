#ifndef BYTEMATRIX_SEQUENCEFILTER_HPP
#define BYTEMATRIX_SEQUENCEFILTER_HPP

#include "../include/bytematrix.h"

#include <atomic>

#include "QueuedModule.hpp"

namespace bytematrix {
    class SequenceFilter final : public QueuedModule
    {
    public:
        SequenceFilter();

        ~SequenceFilter() override
        {
            stop();
        }

        void setNext(IModule* next) override;

    protected:
        void process(ByteArray data) override;

    private:
        std::atomic<IModule*> next_{nullptr};
    };
}

#endif // BYTEMATRIX_SEQUENCEFILTER_HPP
