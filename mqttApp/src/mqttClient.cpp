/*
  Code based on examples provided on the paho.cpp lib repository
  https://github.com/eclipse-paho/paho.mqtt.cpp/tree/master/examples
*/

#include <cstdio>
#include <stdexcept>
#include "mqttClient.h"

// TODO: improve overall error handling

MqttClient::MqttClient(const Config& cfg)
  : client_(cfg.brokerUrl, cfg.clientId), config_(cfg)
{
  client_.set_callback(*this);

  auto builder = mqtt::connect_options_builder::v5()
    .clean_start(cfg.cleanStart)
    .keep_alive_interval(std::chrono::seconds(cfg.keepAliveInterval))
    .automatic_reconnect(true);

  // TODO: integrate secure connection
  // if (!cfg.sslCaCert.empty()) {
  //     mqtt::ssl_options sslOpts;
  //     sslOpts.set_trust_store(cfg.sslCaCert);
  //     if (!cfg.sslCert.empty()) sslOpts.set_key_store(cfg.sslCert);
  //     if (!cfg.sslKey.empty()) sslOpts.set_private_key(cfg.sslKey);
  //     builder.ssl(sslOpts);
  // }

  connOpts_ = builder.finalize();
}

MqttClient::~MqttClient() {
  try {
    disconnect();
  }
  catch (...) {}
}

void MqttClient::connect() {
  client_.connect(connOpts_, nullptr, *this);
}

void MqttClient::disconnect() {
  if (client_.is_connected()) {
    client_.disconnect()->wait();
  }
}

void MqttClient::subscribe(const std::string& topic) {
  if (client_.is_connected())
    client_.subscribe(topic, config_.qos, nullptr, *this);
  else
    throw std::runtime_error("MQTT client not connected");
}

void MqttClient::publish(const std::string& topic, const std::string& payload, int qos, bool retained) {
  if (!client_.is_connected())
    throw std::runtime_error("MQTT client not connected");

  int q = qos >= 0 ? qos : config_.qos;
  client_.publish(topic, payload.c_str(), payload.size(), q, retained, nullptr, *this);
}

void MqttClient::setMessageCallback(MessageCallback cb) {
  messageCallback_ = std::move(cb);
}

void MqttClient::setConnectionCallback(ConnectionCallback cb) {
  connectionCallback_ = std::move(cb);
}

void MqttClient::setOpFailCallback(OpFailCallback cb) {
  opFailCallback_ = std::move(cb);
}

// --- mqtt::callback implementations ---

void MqttClient::connected(const std::string& cause) {
  if (connectionCallback_) {
    connectionCallback_();
  }
  else {
    fprintf(stdout, "%s: Connected: %s\n", moduleName, cause.c_str());
    fprintf(stdout, "%s: No connection callback configured\n", moduleName);
  }
}

void MqttClient::connection_lost(const std::string& cause) {
  fprintf(stderr, "%s: Connection lost: %s\n", moduleName, cause.c_str());
  fprintf(stderr, "%s: Reconnecting... \n", moduleName);
  client_.reconnect();
}

void MqttClient::message_arrived(mqtt::const_message_ptr msg) {
  if (messageCallback_) {
    messageCallback_(msg->get_topic(), msg->to_string());
  }
  else {
    fprintf(stdout, "%s: Message arrived\n", moduleName);
    fprintf(stdout, "%s: \ttopic: '%s'\n", moduleName, msg->get_topic().c_str());
    fprintf(stdout, "%s: \tpayload: '%s'\n", moduleName, msg->to_string().c_str());
    fprintf(stdout, "%s: No message callback configured\n", moduleName);
  }
}

// --- mqtt::iaction_listener implementations ---

void MqttClient::on_success(const mqtt::token& tok) {
  if (tok.get_type() == mqtt::token::Type::CONNECT) {
    fprintf(stdout, "%s: Connection successful\n", moduleName);
  }
  else if (tok.get_type() == mqtt::token::Type::SUBSCRIBE) {
    auto topic = (*tok.get_topics())[0];
    fprintf(stdout, "%s: Subscribed successfully to topic: %s", topic.c_str(), moduleName);
  }
  else if (tok.get_type() == mqtt::token::Type::PUBLISH) {
    fprintf(stdout, "%s: Message published\n", moduleName);
  }
}

void MqttClient::on_failure(const mqtt::token& tok) {
  if (opFailCallback_) {
    opFailCallback_("Operation failed. Token type: " + std::to_string(int(tok.get_type())) + "ErrorMsg: " + tok.get_error_message());
  }
  else {
    fprintf(stderr, "%s: Operation failed. Token type: %d\n", moduleName, int(tok.get_type()));
    fprintf(stderr, "%s: Operation failure callback undefined.\n", moduleName);
  }
}
