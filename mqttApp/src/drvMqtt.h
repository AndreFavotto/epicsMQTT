#ifndef MQTT_H
#define MQTT_H
#include <unistd.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <autoparamDriver.h>
#include <autoparamHandler.h>
#include <asynPortDriver.h>
#include <sstream>
#include <iomanip>
#include "mqttClient.h"

using namespace Autoparam::Convenience;

static const char *driverName = "MqttDriver";

class MqttDriver : public Autoparam::Driver {
  public:
  /* Constructor */
    MqttDriver(const char *portName, const char *mqttBrokerAddr, const char *mqttClientID, const int qos);
  /* Destructor */ 
    ~MqttDriver(); 

  protected:
    // read/write for scalars
    template <typename epicsDataType>
    static Result<epicsDataType> integerRead(DeviceVariable &deviceVar);
    template <typename epicsDataType>
    static WriteResult integerWrite(DeviceVariable &deviceVar, epicsDataType value);
    static UInt32ReadResult digitalRead(DeviceVariable &deviceVar, epicsUInt32 const mask = 0xffff);
    static WriteResult digitalWrite(DeviceVariable &deviceVar, epicsUInt32 const value, epicsUInt32 const mask = 0xffff);

    static Float64ReadResult floatRead(DeviceVariable &deviceVar);
    static WriteResult floatWrite(DeviceVariable &deviceVar, epicsFloat64 value);

    // read/write for arrays
    template <typename epicsDataType>
    static ArrayReadResult arrayRead(DeviceVariable &deviceVar, Array<epicsDataType> &value);
    template <typename epicsDataType>
    static WriteResult arrayWrite(DeviceVariable &deviceVar, Array<epicsDataType> const &value);

    // strings
    static OctetReadResult stringRead(DeviceVariable &deviceVar, Octet &value);
    static WriteResult stringWrite(DeviceVariable &deviceVar, Octet const &value);

  private:
    MqttClient mqttClient;
    /* autoParam specific methods */
    DeviceAddress *parseDeviceAddress(std::string const &function, std::string const &arguments) ;
    DeviceVariable *createDeviceVariable(DeviceVariable *baseVar);
};

class MqttTopicAddr : public DeviceAddress {
  public:
    enum TopicFormat {Flat, Json};

    TopicFormat format;
    std::string topicName;
    std::string jsonField;
    bool operator==(DeviceAddress const& comparedAddr) const;
};

class MqttTopicVariable : public DeviceVariable {
  public:
    MqttTopicVariable(MqttDriver *driver, DeviceVariable *baseVar)
        : DeviceVariable(baseVar), driver(driver) {}
    MqttDriver *driver;
};

#endif /* MQTT_H */
