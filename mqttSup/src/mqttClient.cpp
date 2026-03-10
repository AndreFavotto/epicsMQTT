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

/* Reason string returned if a connection event comes from an auto-reconnect */
const char* MqttClient::AUTO_RECONNECT_REASON = "automatic reconnect";

void MqttClient::connect() {
  client_.connect(connOpts_, nullptr, *this);
}

void MqttClient::disconnect() {
  if (client_.is_connected()) {
    client_.disconnect()->wait();
  }
}

void MqttClient::reconnect() {
  if (!client_.is_connected()) {
    client_.reconnect();
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

void MqttClient::setConnectionCb(ConnectionCallback cb) {
  connectionCb_ = std::move(cb);
}

/* Set callback for "connection_lost" event.

  NOTE: reconnect() is called by default if no disconnection callback is set.
  If a disconnection callback is set, is up to the user to call the reconnect
  routine as needed.
*/
void MqttClient::setDisconnectionCb(DisconnectionCallback cb) {
  disconnectionCb_ = std::move(cb);
}

void MqttClient::setMessageCb(MessageCallback cb) {
  messageCb_ = std::move(cb);
}

void MqttClient::setPublishCb(PublishCallback cb) {
  publishCb_ = std::move(cb);
}

void MqttClient::setOpFailCb(OpFailCallback cb) {
  opFailCb_ = std::move(cb);
}

void MqttClient::setSubscriptionCb(SubscriptionCallback cb) {
  subscriptionCb_ = std::move(cb);
}

// --- mqtt::callback implementations ---

void MqttClient::connected(const std::string& reason) {
  if (connectionCb_) {
    connectionCb_(reason);
  }
  else {
    fprintf(stdout, "%s: %s. Connected to broker.\n", moduleName, reason.c_str());
    fprintf(stdout, "%s: No connection callback configured\n", moduleName);
  }
}

void MqttClient::connection_lost(const std::string& reason) {
  if (disconnectionCb_) {
    disconnectionCb_(reason);
  }
  else {
    fprintf(stderr, "%s: Connection lost: %s\n", moduleName, reason.c_str());
    fprintf(stderr, "%s: Reconnecting... \n", moduleName);
    reconnect();
  }
}

void MqttClient::message_arrived(mqtt::const_message_ptr msg) {
  if (messageCb_) {
    messageCb_(msg->get_topic(), msg->to_string());
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
  if (tok.get_type() == mqtt::token::Type::SUBSCRIBE) {
    auto topic = (*tok.get_topics())[0];
    if (subscriptionCb_) {
      subscriptionCb_(topic);
    }
    else {
      fprintf(stdout, "%s: Subscribed to '%s'\n", moduleName, topic.c_str());
    }
  }
  else if (tok.get_type() == mqtt::token::Type::PUBLISH) {
    auto topic = (*tok.get_topics())[0];
    if (publishCb_) {
      publishCb_(topic);
    }
    else {
      fprintf(stdout, "%s: Message published to '%s'\n", moduleName, topic.c_str());
    }
  }
}

void MqttClient::on_failure(const mqtt::token& tok) {
  std::string errorMsg = "Error: " + tok.get_error_message() + '\n';
  if (opFailCb_) {
    opFailCb_(errorMsg);
  }
  else {
    fprintf(stderr, "%s: %s", moduleName, errorMsg.c_str());
  }
}
