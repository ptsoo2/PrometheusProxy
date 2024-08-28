#include "MetricCollector.h"

namespace p8s
{
    void MetricCollector::close()
    {
        if (cancellationSource_.request_stop() == false)
            return;

        _close();

        mapGauge_.clear();
        registry_.reset();
    }

    auto MetricCollector::registerFamily(const std::string& name, const std::string& help /*= {}*/) -> FamilyConfigurer
    {
        if ((isValid_ == false) || (isClosed() == true))
            return {};

        try
        {
            prometheus::Family<prometheus::Gauge>& family = prometheus::BuildGauge()
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

    void MetricCollector::increment(uint32_t key, double value /*= 1.0*/)
    {
        _modifyGauge(key, [value](auto gauge) { gauge->Increment(value); });
    }

    void MetricCollector::decrement(uint32_t key, double value /*= 1.0*/)
    {
        _modifyGauge(key, [value](auto gauge) { gauge->Decrement(value); });
    }

    void MetricCollector::change(uint32_t key, double value)
    {
        _modifyGauge(key, [value](auto gauge) { gauge->Set(value); });
    }

    void MetricCollector::reset(uint32_t key)
    {
        change(key, 0.0);
    }

    void MetricCollector::_modifyGauge(uint32_t key, fnModify_t&& fnModify) const
    {
        if ((isValid_ == false) || (isClosed() == true))
            return;

        prometheus::Gauge* gauge = [this, key]()
            {
                auto findIter = mapGauge_.find(key);
                return (findIter == mapGauge_.end())
                    ? nullptr
                    : findIter->second;
            }();

        if (gauge == nullptr)
            return;

        fnModify(gauge);
    }

    void MetricCollector::_onAddGauge(uint32_t key, prometheus::Gauge* gauge)
    {
        if (mapGauge_.find(key) != mapGauge_.end())
        {
            isValid_ = false;
            _log(f{ "Failed to add counter(key: {}, error: already exist)", key });
            return;
        }

        mapGauge_.emplace(key, gauge);
    }

    template<typename ...TArgs>
    inline void MetricCollector::_log(f<TArgs...>&& strLog) const
    {
        if (fnLog_ == nullptr)
            return;

        fnLog_(strLog.release());
    }
}

namespace p8s
{
    auto MetricCollector::FamilyConfigurer::addGauge(uint32_t counterKey, const detail::mapLabel_t& mapLabel) -> FamilyConfigurer&
    {
        prometheus::Gauge& counter = family_->Add(mapLabel);
        owner_->_onAddGauge(counterKey, &counter);
        return *this;
    }
}
