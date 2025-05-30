#ifndef DRVMQTT_H
#define DRVMQTT_H
#include <unistd.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <autoparamDriver.h>
#include <autoparamHandler.h>
#include <asynPortDriver.h>
#include <sstream>
#include "mqttClient.h"

using namespace Autoparam::Convenience;

static const char* driverName = "MqttDriver";

class MqttDriver : public Autoparam::Driver {
public:
  /* Constructor */
  MqttDriver(const char* portName, const char* mqttBrokerAddr, const char* mqttClientID, const int qos);
  /* Destructor */
  ~MqttDriver();

protected:
  static void initHook(Autoparam::Driver* driver);
  // read/write for scalars
  static WriteResult integerWrite(DeviceVariable& deviceVar, epicsInt32 value);
  static WriteResult digitalWrite(DeviceVariable& deviceVar, epicsUInt32 const value, epicsUInt32 const mask = 0xffff);
  static WriteResult floatWrite(DeviceVariable& deviceVar, epicsFloat64 value);
  // read/write for arrays
  template <typename epicsDataType>
  static WriteResult arrayWrite(DeviceVariable& deviceVar, Array<epicsDataType> const& value);
  // strings
  static WriteResult stringWrite(DeviceVariable& deviceVar, Octet const& value);
  // Interrupt callback
  static void handleMqttMessage(Autoparam::Driver* driver, const std::string& topic, const std::string& payload);

private:
  MqttClient mqttClient;
  /* autoParam specific methods */
  DeviceAddress* parseDeviceAddress(std::string const& function, std::string const& arguments);
  DeviceVariable* createDeviceVariable(DeviceVariable* baseVar);
  /* helper methods */
  static bool isInteger(const std::string& s);
  static bool isFloat(const std::string& s);
  static bool isSign(char character);
  static asynStatus checkAndParseIntArray(const std::string& s, std::vector<epicsInt32>& out);
};

class MqttTopicAddr : public DeviceAddress {
public:
  enum TopicFormat { Flat, Json };

  TopicFormat format;
  std::string topicName;
  std::string jsonField;
  bool operator==(DeviceAddress const& comparedAddr) const;
};

class MqttTopicVariable : public DeviceVariable {
public:
  MqttTopicVariable(MqttDriver* driver, DeviceVariable* baseVar)
    : DeviceVariable(baseVar), driver(driver) {
  }
  MqttDriver* driver;
};

#endif /* DRVMQTT_H */
