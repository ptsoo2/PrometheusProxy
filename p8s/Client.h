#pragma once

#include "MetricCollector.h"
#include "prometheus/gateway.h"

namespace p8s
{
    /// <summary>
    /// client 사용을 위한 옵션
    /// </summary>
    struct ClientOption
    {
        std::string toString() const
        {
            return std::format("ip: {}, port: {}, jobName: {}, userName: {}, password: {}, timeout: {}(sec), flushInterval: {}(sec)",
                ipAddress_, port_, jobName_, userName_, password_, timeout_.count(), flushInterval_.count());
        }

    public:
        std::string ipAddress_;
        uint16_t port_ = 0;

        std::string jobName_;
        detail::mapLabel_t mapLabel_ = {};

        std::string userName_;
        std::string password_;

        std::chrono::seconds timeout_ = std::chrono::seconds(1);
        std::chrono::seconds flushInterval_ = std::chrono::seconds(1);
    };
}

namespace p8s
{
    /// <summary>
    /// push 방식
    /// 프로메테우스 게이트웨이 측으로 수집 데이터를 꽂아넣는다.
    /// </summary>
    class Client : public MetricCollector
    {
        using fnFlush_t = std::function<void(prometheus::Gateway*)>;

    public:
        Client(fnLog_t&& fnLog = nullptr)
            : MetricCollector(std::move(fnLog))
        {}

    public:
        [[nodiscard]] bool open(ClientOption&& option);

    protected:
        virtual void _close() override;
        bool _flush() const;
        void _run();

    protected:
        ClientOption option_;
        std::unique_ptr<prometheus::Gateway> gateway_ = nullptr;
        std::unique_ptr<std::thread> thread_;
        std::condition_variable cond_;
    };
}

#include "Client.hpp"
