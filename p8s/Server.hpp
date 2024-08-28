#include "Server.h"

namespace p8s
{
    bool Server::open(const std::string& host, uint32_t threadCount /*= 2*/)
    {
        if ((isClosed() == true) || (exposer_ != nullptr))
            throw std::runtime_error("Duplicate try open");

        if (isValid_ == false)
            return false;

        try
        {
            exposer_ = std::make_unique<prometheus::Exposer>(host, threadCount);
        }
        catch (const CivetException& e)
        {
            _log(f{ "Failed to open exposer(host: {}, error: {})", host, e.what() });
            return false;
        }

        exposer_->RegisterCollectable(registry_);

        _log(f{ "Success to open exposer(host: {}, threadCount: {})", host, threadCount });
        return true;
    }

    inline void Server::_close()
    {
        exposer_.reset();
        MetricCollector::close();
    }
}
