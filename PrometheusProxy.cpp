#include "PrometheusProxy.h"

#include "CivetServer.h"

bool PrometheusProxy::open(const std::string& host, uint32_t threadCount /*= 2*/)
{
    if (exposer_ != nullptr)
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

void PrometheusProxy::close()
{
    mapCounter_.clear();
    exposer_.reset();
    registry_.reset();
}

auto PrometheusProxy::registerFamily(const std::string& name, const std::string& help /*= {}*/) -> FamilyConfigurer
{
    if (exposer_ != nullptr)
        throw std::runtime_error("Not allowed to register family after open");

    if (isValid_ == false)
        return {};

    try
    {
        prometheus::Family<prometheus::Counter>& family = prometheus::BuildCounter()
            .Name(name)
            .Help(help)
            .Register(*registry_);

        _log(f{ "Success to register family(name: {})", name });

        return FamilyConfigurer{ this, &family };
    }
    catch (const CivetException& e)
    {
        _log(f{ "Failed to register family(name: {}, error: {})", name, e.what() });
        isValid_ = false;

        return {};
    }
}

void PrometheusProxy::increment(uint32_t counterKey, double value /*= 1.0*/)
{
    _modifyCounter(counterKey, [value](auto counter) { counter->Increment(value); });
}

void PrometheusProxy::reset(uint32_t counterKey)
{
    _modifyCounter(counterKey, [](auto counter) { counter->Reset(); });
}

void PrometheusProxy::_modifyCounter(uint32_t counterKey, fnModify_t&& fnModify) const
{
    if (isValid_ == false)
        return;

    prometheus::Counter* counter = [this, counterKey]()
        {
            auto findIter = mapCounter_.find(counterKey);
            return (findIter == mapCounter_.end())
                ? nullptr
                : findIter->second;
        }();

    fnModify(counter);
}

void PrometheusProxy::_onAddCounter(uint32_t counterKey, prometheus::Counter* counter)
{
    if (mapCounter_.find(counterKey) != mapCounter_.end())
    {
        isValid_ = false;
        _log(f{ "Failed to add counter(key: {}, error: already exist)", counterKey });
        return;
    }

    mapCounter_.emplace(counterKey, counter);
}

auto PrometheusProxy::FamilyConfigurer::addCounter(uint32_t counterKey, const mapLabel_t& mapLabel) -> FamilyConfigurer&
{
    prometheus::Counter& counter = family_->Add(mapLabel);
    owner_->_onAddCounter(counterKey, &counter);
    return *this;
}
