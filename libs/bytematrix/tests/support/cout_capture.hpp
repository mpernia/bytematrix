#ifndef BYTEMATRIX_TESTS_SUPPORT_COUT_CAPTURE_HPP
#define BYTEMATRIX_TESTS_SUPPORT_COUT_CAPTURE_HPP

#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>

namespace bytematrix::test {

    class CoutCapture
    {
    public:
        CoutCapture() : previous_(std::cout.rdbuf(buffer_.rdbuf())) {}

        ~CoutCapture()
        {
            std::cout.rdbuf(previous_);
        }

        CoutCapture(const CoutCapture&) = delete;
        CoutCapture& operator=(const CoutCapture&) = delete;
        CoutCapture(CoutCapture&&) = delete;
        CoutCapture& operator=(CoutCapture&&) = delete;

        [[nodiscard]] std::string str() const
        {
            return buffer_.str();
        }

    private:
        std::ostringstream buffer_;
        std::streambuf* previous_;
    };
}

#endif // BYTEMATRIX_TESTS_SUPPORT_COUT_CAPTURE_HPP
