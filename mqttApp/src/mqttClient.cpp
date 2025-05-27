/*
  Code based on examples provided on the paho.cpp lib repository
  https://github.com/eclipse-paho/paho.mqtt.cpp/tree/master/examples
*/

#include <iostream>
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

// --- mqtt::callback implementations ---

void MqttClient::connected(const std::string& cause) {
  std::cout << "Connected: " << cause << std::endl;
}

void MqttClient::connection_lost(const std::string& cause) {
  std::cerr << "Connection lost: " << cause << std::endl;
}

void MqttClient::message_arrived(mqtt::const_message_ptr msg) {
  if (messageCallback_) {
    messageCallback_(msg->get_topic(), msg->to_string());
  }
  else {
    std::cout << "\nMessage arrived" << std::endl;
    std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
    std::cout << "\tpayload: '" << msg->to_string() << std::endl;
    std::cout << "No callback configured";
  }
}

// --- mqtt::iaction_listener implementations ---

void MqttClient::on_success(const mqtt::token& tok) {
  if (tok.get_type() == mqtt::token::Type::CONNECT) {
    std::cout << "Connection successful" << std::endl;
  }
  else if (tok.get_type() == mqtt::token::Type::SUBSCRIBE) {
    std::cout << "Subscribed successfully" << std::endl;
  }
  else if (tok.get_type() == mqtt::token::Type::PUBLISH) {
    std::cout << "Message published" << std::endl;
  }
}

void MqttClient::on_failure(const mqtt::token& tok) {
  std::cerr << "Operation failed. Token type: " << int(tok.get_type()) << std::endl;
}
