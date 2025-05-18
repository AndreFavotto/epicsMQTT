#pragma once

#include <mqtt/async_client.h>
#include <string>
#include <functional>
#include <memory>

class MqttClient : public virtual mqtt::callback, public virtual mqtt::iaction_listener {
public:
    struct Config {
        std::string brokerUrl = "tcp://localhost:1883";
        std::string clientId = "MqttClient";
        int qos = 1;
        int keepAliveInterval = 20;
        bool cleanStart = true;

        // For future SSL support
        std::string sslCaCert;
        std::string sslCert;
        std::string sslKey;
    };

    using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;

    explicit MqttClient(const Config& cfg);
    ~MqttClient();

    void connect();
    void disconnect();
    void subscribe(const std::string& topic);
    void publish(const std::string& topic, const std::string& payload, int qos = -1, bool retained = false);

    void setMessageCallback(MessageCallback cb);

private:
    mqtt::async_client client_;
    mqtt::connect_options connOpts_;
    Config config_;
    MessageCallback messageCallback_;

    // Callbacks
    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override {}

    // Action listener
    void on_success(const mqtt::token& tok) override;
    void on_failure(const mqtt::token& tok) override;
};
