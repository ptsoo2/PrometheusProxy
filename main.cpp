#include <array>

#include "PrometheusProxy.h"

void example()
{
    // ��Ʈ 8080���� ����Ǵ� HTTP ������ ����ϴ�.
    prometheus::Exposer exposer{ "0.0.0.0:9090" };

    // create a metrics registry
    // ��Ʈ�� ������Ʈ���� ����ϴ�.
    // @note ����ڰ� ��ü�� �����ϴ� ���� å���Դϴ�.
    std::shared_ptr<prometheus::Registry> registry = std::make_shared<prometheus::Registry>();

    // ������Ʈ���� �� ī���� �йи��� �߰��մϴ�(�йи��� ������ �̸��� ���� ���� �ٸ� ���̺� ġ���� �����մϴ�).
    // ��Ʈ�� �̸� ���� best-practice �� �����ʽÿ�: https://prometheus.io/docs/practices/naming/
    prometheus::Family<prometheus::Counter>& packetCounter = prometheus::BuildCounter()
        .Name("observed_packets_total")
        .Help("Number of observed packets")
        .Register(*registry);

    // ġ�� �����͸� �߰��ϰ� ����ϰ�, �װ͵��� ������Ű�� ���� �ſ� �����մϴ�.
    prometheus::Counter& tcp_rx_counter = packetCounter.Add({ {"protocol", "tcp"}, {"direction", "rx"} });
    prometheus::Counter& tcp_tx_counter = packetCounter.Add({ {"protocol", "tcp"}, {"direction", "tx"} });
    prometheus::Counter& udp_rx_counter = packetCounter.Add({ {"protocol", "udp"}, {"direction", "rx"} });
    prometheus::Counter& udp_tx_counter = packetCounter.Add({ {"protocol", "udp"}, {"direction", "tx"} });

    // �����Ͻ� ġ�� �����Ϳ� �˼� ���� ī���͸� �߰������� ġ�� ���� ���� ī��θ�Ƽ������ �߻��ؾ� �մϴ�: https://prometheus.io/docs/practices/naming/#labels
    prometheus::Family<prometheus::Counter>& httpRequestCounter = prometheus::BuildCounter()
        .Name("http_requests_total")
        .Help("Number of HTTP requests")
        .Register(*registry);

    // exposer ���� HTTP ��û�� ���� �� ������Ʈ���� ��ũ�����ϵ��� ��û�մϴ�.
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

            // Family<T>.Add()�� �������� ȣ���ϴ� ���� ���������� �����Ƿ� ���ؾ� �մϴ�.
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
