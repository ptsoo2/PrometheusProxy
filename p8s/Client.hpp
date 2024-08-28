#include "Client.h"

namespace p8s
{
    bool Client::open(ClientOption&& option)
    {
        if ((isClosed() == true) || (gateway_ != nullptr))
            throw std::runtime_error("Duplicate try open");

        if (isValid_ == false)
            return false;

        option_ = std::forward<ClientOption>(option);

        try
        {
            gateway_ = std::make_unique<prometheus::Gateway>(
                option_.ipAddress_,
                std::to_string(option_.port_),
                option_.jobName_,
                option_.mapLabel_,
                option_.userName_,
                option_.password_,
                option_.timeout_
            );
        }
        catch (const CivetException& e)
        {
            _log(f{ "Failed to open gateway(option: {}, error: {})", option_.toString(), e.what() });
            return false;
        }

        gateway_->RegisterCollectable(registry_);

        // ���ý� �ѹ� push �� ���������� �Ǵ� ���� Ȯ���ϰ� �Ѿ��.
        if (_flush() == false)
            return false;

        thread_ = std::make_unique<std::thread>([this]() { this->_run(); });

        _log(f{ "Success to open gateway(option: {})", option_.toString() });
        return true;
    }

    void Client::_close()
    {
        {
            cond_.notify_all();
            if (thread_->joinable() == true)
                thread_->join();
            thread_.reset();
        }

        // ����Ʈ���̿��� ������ ���� �����ϰ� �����Ƿ� ����ÿ��� ���� 0���� �����Ѵ�.
        for (auto& iter : mapGauge_)
        {
            prometheus::Gauge* gauge = iter.second;
            gauge->Set(0.0);
        }
        _flush();

        gateway_.reset();
        MetricCollector::close();
    }

    bool Client::_flush() const
    {
        if (gateway_ == nullptr)
            throw std::runtime_error("Not opened");

        const int status = gateway_->Push();
        if (status != 200)
        {
            _log(f{ "Failed to push(status: {})", status });
            return false;
        }

        return true;
    }

    void Client::_run()
    {
        std::mutex lock;
        std::stop_token token = cancellationSource_.get_token();

        while (token.stop_requested() == false)
        {
            _flush();

            {
                std::unique_lock grab(lock);
                cond_.wait_for(grab, option_.flushInterval_);
            }
        }
    }
}
