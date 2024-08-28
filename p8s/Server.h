#pragma once

#include "MetricCollector.h"

#include "prometheus/exposer.h"

namespace p8s
{
    /// <summary>
    /// pull 방식
    /// 웹서버를 띄우며, 프로메테우스 측에서 요청하면 수집 데이터를 반환해준다.
    /// </summary>
    class Server : public MetricCollector
    {
    public:
        Server(fnLog_t&& fnLog = nullptr)
            : MetricCollector(std::move(fnLog))
        {}

    public:
        [[nodiscard]] bool open(const std::string& host, uint32_t threadCount = 2);

    protected:
        virtual void _close() override;

    protected:
        std::unique_ptr<prometheus::Exposer> exposer_ = nullptr;
    };
}

#include "Server.hpp"
