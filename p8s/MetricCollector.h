#pragma once

#ifdef _MSC_VER
#	define _USE_DETAILED_FUNCTION_NAME_IN_SOURCE_LOCATION ( 0 )
#endif // _MSC_VER

#include <format>
#include <source_location>
#include <functional>

#include "prometheus/counter.h"
#include "prometheus/registry.h"
#include "CivetServer.h"

// 라이브러리에서 이미 prometheus 를 쓰고 있어 부득이하게 p8s 로 네이밍..
namespace p8s::detail
{
    using mapLabel_t = std::map<std::string, std::string>;	// key, value
}

namespace p8s
{
    /// <summary>
    /// 프로메테우스에서 지원하는 방식으로 등록된 카운터를 보유하여 지표를 내보낼 수 있게한다.
    /// </summary>
    class MetricCollector
    {
    protected:
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

        using mapGauge_t = std::unordered_map<uint32_t, prometheus::Gauge*>;

        using fnLog_t = std::function<void(std::string&&)>;
        using fnModify_t = std::function<void(prometheus::Gauge*)>;

        class FamilyConfigurer;

    protected:
        explicit MetricCollector(fnLog_t&& fnLog)
            : fnLog_(std::move(fnLog))
        {}
        virtual ~MetricCollector() = default;

    public:
        bool isClosed() const { return cancellationSource_.stop_requested() == true; }
        void close();

        [[nodiscard]] FamilyConfigurer registerFamily(const std::string& name, const std::string& help = {});
        void increment(uint32_t key, double value = 1.0);
        void decrement(uint32_t key, double value = 1.0);
        void change(uint32_t key, double value);
        void reset(uint32_t key);

    protected:
        virtual void _close() = 0;

        void _modifyGauge(uint32_t counterKey, fnModify_t&& fnModify) const;
        void _onAddGauge(uint32_t counterKey, prometheus::Gauge* gauge);

        template<typename ...TArgs>
        void _log(f<TArgs...>&& strLog) const;

    protected:
        bool isValid_ = true;
        std::stop_source cancellationSource_;

        fnLog_t fnLog_ = nullptr;
        mapGauge_t mapGauge_;
        std::shared_ptr<prometheus::Registry> registry_ = std::make_shared<prometheus::Registry>();
    };
}

namespace p8s
{
    /// <summary>
    /// 메트릭을 정의하여 패밀리를 구성한다.
    /// 성능의 이유로 서비스 중에 구성 변경은 지양한다.
    /// </summary>
    class MetricCollector::FamilyConfigurer
    {
    public:
        FamilyConfigurer() = default;
        explicit FamilyConfigurer(MetricCollector* owner, prometheus::Family<prometheus::Gauge>* family)
            : owner_(owner)
            , family_(family)
        {}

        FamilyConfigurer& addGauge(uint32_t counterKey, const detail::mapLabel_t& mapLabel);

    protected:
        MetricCollector* owner_ = nullptr;
        prometheus::Family<prometheus::Gauge>* family_ = nullptr;
    };
}

#include "MetricCollector.hpp"
