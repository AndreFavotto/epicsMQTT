#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H
#include <mqtt/async_client.h>
#include <string>
#include <functional>
#include <memory>

class MqttClient : public virtual mqtt::callback, public virtual mqtt::iaction_listener {
public:
  struct Config {
    std::string brokerUrl = "mqtt://localhost:1883";
    std::string clientId = "MqttClient";
    int qos = 1;
    int keepAliveInterval = 20;
    bool cleanStart = true;

    // For future SSL support
    std::string sslCaCert;
    std::string sslCert;
    std::string sslKey;
  };
  MqttClient(const Config& cfg);
  ~MqttClient();

  using MessageCallback = std::function<void(const std::string& topic, const std::string& payload)>;
  using ConnectionCallback = std::function<void()>;
  using OpFailCallback = std::function<void(const std::string& message)>;

  void connect();
  void disconnect();
  void subscribe(const std::string& topic);
  void publish(const std::string& topic, const std::string& payload, int qos = -1, bool retained = false);

  void setMessageCallback(MessageCallback cb);
  void setConnectionCallback(ConnectionCallback cb);
  void setOpFailCallback(OpFailCallback cb);

private:
  int nretry_;
  const char* moduleName = "pahoMqttClient";
  mqtt::async_client client_;
  mqtt::connect_options connOpts_;
  Config config_;
  MessageCallback messageCallback_;
  ConnectionCallback connectionCallback_;
  OpFailCallback opFailCallback_;

  // Callbacks
  void connected(const std::string& cause) override;
  void connection_lost(const std::string& cause) override;
  void message_arrived(mqtt::const_message_ptr msg) override;
  void delivery_complete(mqtt::delivery_token_ptr tok) override {}

  // Action listener
  void on_success(const mqtt::token& tok) override;
  void on_failure(const mqtt::token& tok) override;
};
#endif