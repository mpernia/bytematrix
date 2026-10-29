#ifndef BYTEMATRIX_BYTEARRAYRECORDER_HPP
#define BYTEMATRIX_BYTEARRAYRECORDER_HPP

#include <chrono>
#include <vector>

#include "QueuedModule.hpp"
#include "bytematrix.h"

namespace bytematrix {
    struct Record
    {
        std::chrono::system_clock::time_point timestamp;
        ByteArray data;
    };

    class ByteArrayRecorder final : public QueuedModule
    {
    public:
        ByteArrayRecorder();

        ~ByteArrayRecorder() override
        {
            stop();
        }

        void setNext(IModule* next) override;

    protected:
        void process(ByteArray data) override;

    private:
        void printRecords() const;

        std::vector<Record> records_;
    };
}

#endif // BYTEMATRIX_BYTEARRAYRECORDER_HPP
