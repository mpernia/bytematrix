#include "QueuedModule.hpp"
#include "../include/bytematrix.h"

#include <utility>

namespace bytematrix {
    QueuedModule::~QueuedModule()
    {
        QueuedModule::stop();
    }

    void QueuedModule::start()
    {
        if (worker_.joinable() || started_.exchange(true))
        {
            return;
        }

        worker_ = std::thread(&QueuedModule::run, this);
    }

    void QueuedModule::stop()
    {
        input_.close();

        if (worker_.joinable())
        {
            worker_.join();
        }
    }

    void QueuedModule::run()
    {
        ByteArray data;

        while (input_.pop(data))
        {
            process(std::move(data));
        }

        onStop();
    }
}
