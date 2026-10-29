#ifndef PIPELINE_RUNNER_HPP
#define PIPELINE_RUNNER_HPP

#include <bytematrix.h>

#include <chrono>
#include <memory>

class PipelineRunner
{
public:
    PipelineRunner();
    ~PipelineRunner();

    PipelineRunner(const PipelineRunner&) = delete;
    PipelineRunner& operator=(const PipelineRunner&) = delete;

    void run(std::chrono::milliseconds duration);

private:
    void start();
    void stop();
    void print() const;

    std::unique_ptr<bytematrix::IModule> recorder_;
    std::unique_ptr<bytematrix::IModule> filter_;
    std::unique_ptr<bytematrix::IModule> generator_;

    bool interrupted_{false};
    std::chrono::seconds elapsed_{};
};

#endif // PIPELINE_RUNNER_HPP
