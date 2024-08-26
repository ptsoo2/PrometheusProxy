#include <array>

#include "PrometheusProxy.h"

void example()
{
    // 포트 8080에서 실행되는 HTTP 서버를 만듭니다.
    prometheus::Exposer exposer{ "0.0.0.0:9090" };

    // create a metrics registry
    // 메트릭 레지스트리를 만듭니다.
    // @note 사용자가 객체를 유지하는 것이 책임입니다.
    std::shared_ptr<prometheus::Registry> registry = std::make_shared<prometheus::Registry>();

    // 레지스트리에 새 카운터 패밀리를 추가합니다(패밀리는 동일한 이름을 가진 값과 다른 레이블 치수를 결합합니다).
    // 메트릭 이름 짓는 best-practice 를 따르십시오: https://prometheus.io/docs/practices/naming/
    prometheus::Family<prometheus::Counter>& packetCounter = prometheus::BuildCounter()
        .Name("observed_packets_total")
        .Help("Number of observed packets")
        .Register(*registry);

    // 치수 데이터를 추가하고 기억하고, 그것들을 증가시키는 것은 매우 저렴합니다.
    prometheus::Counter& tcp_rx_counter = packetCounter.Add({ {"protocol", "tcp"}, {"direction", "rx"} });
    prometheus::Counter& tcp_tx_counter = packetCounter.Add({ {"protocol", "tcp"}, {"direction", "tx"} });
    prometheus::Counter& udp_rx_counter = packetCounter.Add({ {"protocol", "udp"}, {"direction", "rx"} });
    prometheus::Counter& udp_tx_counter = packetCounter.Add({ {"protocol", "udp"}, {"direction", "tx"} });

    // 컴파일시 치수 데이터에 알수 없는 카운터를 추가하지만 치수 값은 낮은 카디널리티에서만 발생해야 합니다: https://prometheus.io/docs/practices/naming/#labels
    prometheus::Family<prometheus::Counter>& httpRequestCounter = prometheus::BuildCounter()
        .Name("http_requests_total")
        .Help("Number of HTTP requests")
        .Register(*registry);

    // exposer 에게 HTTP 요청이 들어올 때 레지스트리를 스크래핑하도록 요청합니다.
    exposer.RegisterCollectable(registry);

    printf("boot complete \n");

    const std::array<std::string, 4> methods{ "GET", "PUT", "POST", "HEAD" };

    for (;;)
    {
        const int randomValue = std::rand();

        {
            if (randomValue & 1) { tcp_rx_counter.Increment(); }
            if (randomValue & 2) { tcp_tx_counter.Increment(); }
            if (randomValue & 4) { udp_rx_counter.Increment(); }
            if (randomValue & 8) { udp_tx_counter.Increment(); }
        }

        {
            const std::string& method = methods.at(randomValue % methods.size());

            // Family<T>.Add()를 동적으로 호출하는 것은 가능하지만 느리므로 피해야 합니다.
            httpRequestCounter
                .Add({ {"method", method} })
                .Increment();
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

enum ENUM_NETWORK_METRIC : uint32_t
{
    METRIC_1 = 0,
    METRIC_2,
    METRIC_3,
    METRIC_4,
    METRIC_5,
    _METRIC_MAX_,
};

int main()
{
    auto cbLog = [](std::string&& str)
        {
            printf("%s \n", str.c_str());
        };

    PrometheusProxy proxy{ std::move(cbLog) };

    proxy
        .registerFamily("observed_packets_total", "Number of observed packets")
        .addCounter(METRIC_1, { {"protocol", "tcp"}, { "direction", "rx" } })
        .addCounter(METRIC_2, { {"protocol", "tcp"}, { "direction", "tx" } })
        .addCounter(METRIC_3, { {"protocol", "udp"}, { "direction", "rx" } })
        .addCounter(METRIC_4, { {"protocol", "udp"}, { "direction", "tx" } })
        ;

    proxy
        .registerFamily("http_requests_total", "Number of HTTP requests")
        .addCounter(METRIC_5, { {"method", "GET"} })
        ;

    if (proxy.open("172.30.1.62:9090") == false)
    {
        printf("Failed to do open \n");
        return -1;
    }

    proxy.increment(METRIC_1, 1.0);
    proxy.increment(METRIC_2, 1.0);
    proxy.increment(METRIC_3, 1.0);
    proxy.increment(METRIC_4, 1.0);
    proxy.increment(METRIC_5, 1.0);

    proxy.reset(METRIC_1);
    proxy.reset(METRIC_2);
    proxy.reset(METRIC_3);
    proxy.reset(METRIC_4);
    proxy.reset(METRIC_5);

    size_t counter = 10;
    while (counter--)
    {
        auto randVal = rand() % 3;
        auto randVal2 = (ENUM_NETWORK_METRIC)(rand() % _METRIC_MAX_);
        switch (randVal)
        {
        case 0:
        {
            proxy.increment(randVal2);
            printf("increment (%d, 1) \n", randVal2);
        }
        break;
        case 1:
        {
            const double value = static_cast<double>(rand() % 2147483);
            proxy.increment(randVal2, value);
            printf("increment (%d, %f) \n", randVal2, value);
        }
        break;
        case 2:
        {
            proxy.reset(randVal2);
            printf("reset (%d) \n", randVal2);
        }
        break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    proxy.close();

    return 0;
}
