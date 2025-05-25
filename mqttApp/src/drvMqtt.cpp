#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include <string.h>
#include <map>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <autoparamDriver.h>
#include <autoparamHandler.h>
#include "drvMqtt.h"
#include "mqttClient.h"

#define FLAT_FUNCTION_STR "FLAT"
#define JSON_FUNCTION_STR "JSON"
//#############################################################################################
// autoParam-specific definitions

// Overloads the operator '==' to compare MqttTopicAddr objects created on record I/O links
bool MqttTopicAddr::operator==(DeviceAddress const& comparedAddr) const {
    const MqttTopicAddr &cmp = static_cast<const MqttTopicAddr&>(comparedAddr);
    if (format != cmp.format) return false;
    switch (format) {
        case Flat:
            return topicName == cmp.topicName;
        case Json:
            return topicName == cmp.topicName && jsonField == cmp.jsonField;
    }
    return false;
}

DeviceAddress *MqttDriver::parseDeviceAddress(std::string const &function, std::string const &arguments) {
    MqttTopicAddr *addr = new MqttTopicAddr;
    std::istringstream is(arguments);
    is >> std::setbase(0);
    //TODO: add error handling for malformed arguments
    if (function == FLAT_FUNCTION_STR) {
        addr->format = MqttTopicAddr::Flat;
        is >> addr->topicName;
        //subscribe to topic here?
    } else if (function == JSON_FUNCTION_STR) {
        addr->format = MqttTopicAddr::Json;
        is >> addr->topicName;
        is >> addr->jsonField;
        //subscribe to topic and link to json parser
    } else {
        delete addr;
        return NULL;
    }

    return addr;
}

DeviceVariable *MqttDriver::createDeviceVariable(DeviceVariable *baseVar){
  return new MqttTopicVariable(this, baseVar);
}
//#############################################################################################

/* Class constructor
 * @param portName Asyn port name to be used 
 * @param brokerUrl Broker IP or hostname (e.g: mqtt://localhost:1883)
 * @param mqttClientID ClientID to be used - must be unique
 * @param qos Desired quality of service (QoS) for the connection [0|1|2]
 * @param timeout Connection timeout to be used 
*/
MqttDriver::MqttDriver(const char *portName, const char *brokerUrl, const char *mqttClientID, const int qos)
   : Autoparam::Driver(
        portName,
        Autoparam::DriverOpts()
                  .setBlocking(false)
                  .setAutoInterrupts(false)
                  .setInitHook(NULL)),
        mqttClient([&]{
         MqttClient::Config cfg;
         cfg.brokerUrl = brokerUrl;
         cfg.clientId  = mqttClientID;
         cfg.qos       = qos;
         return cfg;
     }())
  {
    mqttClient.connect();
    // epics -> Asyn DTYP param mapping:
    // epicsInt32 → asynParamInt32
    // epicsInt64 → asynParamInt64
    // epicsFloat64 → asynParamFloat64
    // epicsUint32 → asynParamUint32Digital
    // Octet → asynParamOctet
    // Array<epicsInt8> → asynParamInt8Array
    // Array<epicsInt16> → asynParamInt16Array
    // Array<epicsInt32> → asynParamInt32Array
    // Array<epicsInt64> → asynParamInt64Array
    // Array<epicsFloat32> → asynParamFloat32Array
    // Array<epicsFloat64> → asynParamFloat64Array

    // flat topic support
    registerHandlers<epicsInt32>(FLAT_FUNCTION_STR, integerRead, integerWrite, NULL);
    registerHandlers<epicsInt64>(FLAT_FUNCTION_STR, integerRead, integerWrite, NULL);
    registerHandlers<epicsFloat64>(FLAT_FUNCTION_STR, floatRead, floatWrite, NULL);
    registerHandlers<epicsUInt32>(FLAT_FUNCTION_STR, digitalRead, digitalWrite, NULL);
    registerHandlers<Octet>(FLAT_FUNCTION_STR, stringRead, stringWrite, NULL);
    registerHandlers<Array<epicsInt8>>(FLAT_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsInt16>>(FLAT_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsInt32>>(FLAT_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsFloat32>>(FLAT_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsFloat64>>(FLAT_FUNCTION_STR, arrayRead, arrayWrite, NULL);

    // json topic support
    //TODO: add support for JSON structured messages
    registerHandlers<epicsInt32>(JSON_FUNCTION_STR, integerRead, integerWrite, NULL);
    registerHandlers<epicsInt64>(JSON_FUNCTION_STR, integerRead, integerWrite, NULL);
    registerHandlers<epicsFloat64>(JSON_FUNCTION_STR, floatRead, floatWrite, NULL);
    registerHandlers<epicsUInt32>(JSON_FUNCTION_STR, digitalRead, digitalWrite, NULL);
    registerHandlers<Octet>(JSON_FUNCTION_STR, stringRead, stringWrite, NULL);
    registerHandlers<Array<epicsInt8>>(JSON_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsInt16>>(JSON_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsInt32>>(JSON_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsFloat32>>(JSON_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsFloat64>>(JSON_FUNCTION_STR, arrayRead, arrayWrite, NULL);
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
/* Class destructor
   - Disconnects from the broker and cleans session
*/ 
MqttDriver::~MqttDriver() {
    mqttClient.disconnect();
}

//#############################################################################################
// IO function definitions
template <typename epicsDataType>
Result<epicsDataType> MqttDriver::integerRead(DeviceVariable &deviceVar){
  Result<epicsDataType> result;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &topicName =
        static_cast<MqttTopicAddr const &>(deviceVar.address());
 std::string format = deviceVar.function();
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;
  epicsDataType value;
  result.status = asynError;
  result.value = static_cast<epicsInt32>(value);
  return result;
}

template <typename epicsDataType>
WriteResult MqttDriver::integerWrite(DeviceVariable &deviceVar, epicsDataType value){
  WriteResult result;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &topicName =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;
  std::string format = deviceVar.function();
  result.status = asynError;
  return result;
}
UInt32ReadResult MqttDriver::digitalRead(DeviceVariable &deviceVar, epicsUInt32 const mask){
  UInt32ReadResult result;
  result.status = asynError;
  return result;
}
WriteResult MqttDriver::digitalWrite(DeviceVariable &deviceVar, epicsUInt32 const value, epicsUInt32 const mask){
  WriteResult result;
  result.status = asynError;
  return result;
}

Float64ReadResult MqttDriver::floatRead(DeviceVariable &deviceVar){
  Float64ReadResult result;
  result.status = asynError;
  return result;
}
WriteResult MqttDriver::floatWrite(DeviceVariable &deviceVar, epicsFloat64 value){
  WriteResult result;
  result.status = asynError;
  return result;
}

// read/write for arrays
template <typename epicsDataType>
ArrayReadResult MqttDriver::arrayRead(DeviceVariable &deviceVar, Array<epicsDataType> &value){
  ArrayReadResult result;
  result.status = asynError;
  return result;
}
template <typename epicsDataType>
WriteResult MqttDriver::arrayWrite(DeviceVariable &deviceVar, Array<epicsDataType> const &value){
  WriteResult result;
  result.status = asynError;
  return result;
}

// strings
OctetReadResult MqttDriver::stringRead(DeviceVariable &deviceVar, Octet &value){
  OctetReadResult result;
  result.status = asynError;
  return result;
}
WriteResult MqttDriver::stringWrite(DeviceVariable &deviceVar, Octet const &value){
  WriteResult result;
  result.status = asynError;
  return result;
}
  //#############################################################################################
extern "C" {
    int mqttDriverConfigure(const char *portName, const char *brokerUrl, const char *mqttClientID, const int qos) {
        new MqttDriver(portName, brokerUrl, mqttClientID, qos);
        return(asynSuccess);
    }
    static const int numArgs = 4;
    static const iocshArg initArg0 = { "portName", iocshArgString};
    static const iocshArg initArg1 = { "brokerUrl", iocshArgString};
    static const iocshArg initArg2 = { "mqttClientID", iocshArgString};
    static const iocshArg initArg3 = { "qos", iocshArgInt};
    static const iocshArg * const initArgs[] = {
        &initArg0,
        &initArg1,
        &initArg2,
        &initArg3
    };
    static const char *usage =
    "MqttDriverConfigure(portName, brokerUrl, mqttClientID, qos)\n"
    "  portName: Asyn port name to be used\n"
    "  brokerUrl: Broker IP or hostname (e.g: mqtt://localhost:1883)\n"
    "  mqttClientID: ClientID to be used - must be unique\n"
    "  qos: Desired quality of service (QoS) for the connection [0|1|2]\n";

//#############################################################################################
    static const iocshFuncDef initFuncDef = {"mqttDriverConfigure", numArgs, initArgs, usage};

//#############################################################################################
    static void initCallFunc(const iocshArgBuf *args) {
        mqttDriverConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].ival);
    }

//#############################################################################################
    void mqttDriverRegister(void) {
        iocshRegister(&initFuncDef, initCallFunc);
    }

    epicsExportRegistrar(mqttDriverRegister);
}
