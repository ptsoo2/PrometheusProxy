#pragma once

#include "MetricCollector.h"

#include "prometheus/exposer.h"

namespace p8s
{
    /// <summary>
    /// pull ���
    /// �������� ����, ���θ��׿콺 ������ ��û�ϸ� ���� �����͸� ��ȯ���ش�.
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
