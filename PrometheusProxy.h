#pragma once

#ifdef _MSC_VER
#	define _USE_DETAILED_FUNCTION_NAME_IN_SOURCE_LOCATION ( 0 )
#endif // _MSC_VER

#include <format>
#include <source_location>

#include "prometheus/counter.h"
#include "prometheus/exposer.h"
#include "prometheus/registry.h"

class PrometheusProxy
{
    template<typename ...TArgs>
    struct f
    {
        f(std::string_view format, TArgs&&... args, const std::source_location& location = std::source_location::current())
            : log_(std::format("[ {}({}) ] ", location.function_name(), location.line())
                .append(std::vformat(format, std::make_format_args(args...))))
        {}

        std::string release() { return std::move(log_); }

    protected:
        std::string log_;
    };

    template<typename ...TArgs>
    f(std::string_view, TArgs&&...) -> f<TArgs...>;

    using fnLog_t = std::function<void(std::string&&)>;
    using fnModify_t = std::function<void(prometheus::Counter*)>;
    using mapCounter_t = std::unordered_map<uint32_t, prometheus::Counter*>;

    class FamilyConfigurer;

public:
    explicit PrometheusProxy(fnLog_t&& fnLog = {})
        : fnLog_(std::move(fnLog))
    {}
    ~PrometheusProxy() { close(); }

public:
    [[nodiscard]] bool open(const std::string& host, uint32_t threadCount = 2);
    void close();

    [[nodiscard]] FamilyConfigurer registerFamily(const std::string& name, const std::string& help = {});
    void increment(uint32_t counterKey, double value = 1.0);
    void reset(uint32_t counterKey);

protected:
    void _modifyCounter(uint32_t counterKey, fnModify_t&& fnModify) const;
    void _onAddCounter(uint32_t counterKey, prometheus::Counter* counter);

    template<typename ...TArgs>
    void _log(f<TArgs...>&& strLog)
    {
        if (fnLog_ != nullptr)
            fnLog_(std::move(strLog.release()));
    }

protected:
    bool isValid_ = true;

    fnLog_t fnLog_ = nullptr;
    std::shared_ptr<prometheus::Registry> registry_ = std::make_shared<prometheus::Registry>();
    std::unique_ptr<prometheus::Exposer> exposer_ = nullptr;
    mapCounter_t mapCounter_;
};

class PrometheusProxy::FamilyConfigurer
{
    using mapLabel_t = std::map<std::string, std::string>;	// key, value

public:
    FamilyConfigurer() = default;
    explicit FamilyConfigurer(PrometheusProxy* owner, prometheus::Family<prometheus::Counter>* counter)
        : owner_(owner)
        , family_(counter)
    {}

    FamilyConfigurer& addCounter(uint32_t counterKey, const mapLabel_t& mapLabel);

protected:
    PrometheusProxy* owner_ = nullptr;
    prometheus::Family<prometheus::Counter>* family_ = nullptr;
};
