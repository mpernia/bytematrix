#ifndef BYTEMATRIX_SEQUENCEFILTER_HPP
#define BYTEMATRIX_SEQUENCEFILTER_HPP

#include <atomic>

#include "QueuedModule.hpp"
#include "bytematrix.h"

namespace bytematrix {
    class SequenceFilter final : public QueuedModule
    {
    public:
        explicit SequenceFilter(ByteArray sequence);

        ~SequenceFilter() override
        {
            stop();
        }

        void setNext(IModule* next) override;

    protected:
        void process(ByteArray data) override;

    private:
        std::atomic<IModule*> next_{nullptr};
        const ByteArray sequence_;
    };
}

#endif // BYTEMATRIX_SEQUENCEFILTER_HPP
