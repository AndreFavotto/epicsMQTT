#include "drvMqtt.h"
#include "mqttClient.h"

// Supported type definitions

#define FLAT_FUNC_PREFIX "FLAT"
#define JSON_FUNC_PREFIX "JSON"
#define FLAT_INT_FUNC_STR         FLAT_FUNC_PREFIX ":INT"
#define FLAT_FLOAT_FUNC_STR       FLAT_FUNC_PREFIX ":FLOAT"
#define FLAT_DIGITAL_FUNC_STR     FLAT_FUNC_PREFIX ":DIGITAL"
#define FLAT_STRING_FUNC_STR      FLAT_FUNC_PREFIX ":STRING"
#define FLAT_INTARRAY_FUNC_STR    FLAT_FUNC_PREFIX ":INTARRAY"
#define FLAT_FLOATARRAY_FUNC_STR  FLAT_FUNC_PREFIX ":FLOATARRAY"
#define JSON_INT_FUNC_STR         JSON_FUNC_PREFIX ":INT"
#define JSON_FLOAT_FUNC_STR       JSON_FUNC_PREFIX ":FLOAT"
#define JSON_DIGITAL_FUNC_STR     JSON_FUNC_PREFIX ":DIGITAL"
#define JSON_STRING_FUNC_STR      JSON_FUNC_PREFIX ":STRING"
#define JSON_INTARRAY_FUNC_STR    JSON_FUNC_PREFIX ":INTARRAY"
#define JSON_FLOATARRAY_FUNC_STR  JSON_FUNC_PREFIX ":FLOATARRAY"
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
    // Split function into prefix and type by ':'
    auto pos = function.find(':');
    std::string prefix = (pos == std::string::npos) ? function : function.substr(0, pos);
    std::string type = (pos == std::string::npos) ? "" : function.substr(pos + 1);
    //TODO: add error handling for malformed arguments
    if (prefix == FLAT_FUNC_PREFIX) {
      addr->format = MqttTopicAddr::Flat;
      is >> addr->topicName;
    } 
    else if (prefix == JSON_FUNC_PREFIX) {
      addr->format = MqttTopicAddr::Json;
      is >> addr->topicName;
      is >> addr->jsonField;
    }
    else {
      // unknown prefix
      delete addr;
      return nullptr;
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
                  .setInitHook(initHook)),
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
    registerHandlers<epicsInt32>(FLAT_INT_FUNC_STR, integerRead, integerWrite, NULL);
    registerHandlers<epicsFloat64>(FLAT_FLOAT_FUNC_STR, floatRead, floatWrite, NULL);
    registerHandlers<epicsUInt32>(FLAT_DIGITAL_FUNC_STR, digitalRead, digitalWrite, NULL);
    registerHandlers<Octet>(FLAT_STRING_FUNC_STR, stringRead, stringWrite, NULL);
    registerHandlers<Array<epicsInt32>>(FLAT_INTARRAY_FUNC_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsFloat64>>(FLAT_FLOATARRAY_FUNC_STR, arrayRead, arrayWrite, NULL);

    // json topic support
    registerHandlers<epicsInt32>(JSON_INT_FUNC_STR, integerRead, integerWrite, NULL);
    registerHandlers<epicsFloat64>(JSON_FLOAT_FUNC_STR, floatRead, floatWrite, NULL);
    registerHandlers<epicsUInt32>(JSON_DIGITAL_FUNC_STR, digitalRead, digitalWrite, NULL);
    registerHandlers<Octet>(JSON_STRING_FUNC_STR, stringRead, stringWrite, NULL);
    registerHandlers<Array<epicsInt32>>(JSON_INTARRAY_FUNC_STR, arrayRead, arrayWrite, NULL);
    registerHandlers<Array<epicsFloat64>>(JSON_FLOATARRAY_FUNC_STR, arrayRead, arrayWrite, NULL);

    //TODO: add support for JSON structured messages
    std::this_thread::sleep_for(std::chrono::seconds(2));
  }
/* Class destructor
   - Disconnects from the broker and cleans session
*/ 
MqttDriver::~MqttDriver() {
    mqttClient.disconnect();
}

void MqttDriver::initHook(Autoparam::Driver *driver) {
  auto *pself = static_cast<MqttDriver *>(driver);
  auto vars = pself->getInterruptVariables();
  // Subscribe to all I/O Intr topics
  for (auto itr = vars.begin(); itr != vars.end(); itr++) {
    auto &deviceVar = *static_cast<MqttTopicVariable *>(*itr);
    MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
    pself->mqttClient.subscribe(addr.topicName);
  }
  pself->mqttClient.setMessageCallback([pself](const std::string& topic, const std::string& payload) {
    pself->handleMqttMessage(pself, topic, payload);
  });
}

void MqttDriver::handleMqttMessage(Autoparam::Driver *driver, const std::string& topic, const std::string& payload) {
    auto *pself = static_cast<MqttDriver *>(driver);
    pself->lock();
    auto vars = pself->getInterruptVariables();
    // Subscribe to all I/O Intr topics
    for (auto itr = vars.begin(); itr != vars.end(); itr++) {
      auto &deviceVar = *static_cast<MqttTopicVariable *>(*itr);
      MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
        if (addr.topicName != topic)
          continue;
          int index = deviceVar.asynIndex();
          switch (deviceVar.asynType()) {
            case asynParamInt32:
                pself->setParam(deviceVar, std::stoi(payload), asynSuccess);
                break;
            case asynParamFloat64:
                pself->setParam(deviceVar, std::stod(payload), asynSuccess);
                break;
            case asynParamUInt32Digital:
                pself->setParam(deviceVar, static_cast<epicsUInt32>(std::stoul(payload)), 0xFFFFFFFF, asynSuccess);
                break;
            case asynParamOctet:
                // use asyn directly - setParam octet overload is not defined
                pself->setStringParam(index, payload.c_str());
                break;
            case asynParamInt32Array:
                // not implemented
                break;
            case asynParamFloat64Array:
                // not implemented
                break;
            default:
                break;
        }
    }
    pself->callParamCallbacks();
    pself->unlock();
}
//#############################################################################################
// IO function definitions
template <typename epicsDataType>
Result<epicsDataType> MqttDriver::integerRead(DeviceVariable &deviceVar){
  Result<epicsDataType> result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
        static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;
  /*
  since we cannot actively read MQTT topics due to the publish/subscribe
  nature of MQTT - handled as I/O Intr - if a read is requested, we just 
  return what is currently stored in the asyn parameter memory
  */
  epicsInt32 value;
  status = driver->getIntegerParam(deviceVar.asynIndex(), &value);

  if (status != asynSuccess) {
    result.status = asynError;
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s: Failed to get value for param index %d (topic: '%s')\n",
            driverName, functionName, deviceVar.asynIndex(), topicName.c_str());
  }
  result.value = value;
  result.status = status;
  return result;
}

template <typename epicsDataType>
WriteResult MqttDriver::integerWrite(DeviceVariable &deviceVar, epicsDataType value){
  WriteResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  if(addr.format == MqttTopicAddr::TopicFormat::Flat){
    try {
      driver->mqttClient.publish(addr.topicName, std::to_string(value));
      status = asynSuccess;
    }
    catch (const std::exception &exc) {
      status = asynError;
    }
  }

  else if(addr.format == MqttTopicAddr::TopicFormat::Json){
    //Not implemented
    status = asynError;
  }

  if (status != asynSuccess) {
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
            "%s::%s: Failed to get value for param index %d (topic: '%s')\n",
            driverName, functionName, deviceVar.asynIndex(), topicName.c_str());
  }
  result.status = status;
  return result;
}
UInt32ReadResult MqttDriver::digitalRead(DeviceVariable &deviceVar, epicsUInt32 const mask){
  UInt32ReadResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}
WriteResult MqttDriver::digitalWrite(DeviceVariable &deviceVar, epicsUInt32 const value, epicsUInt32 const mask){
  WriteResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}

Float64ReadResult MqttDriver::floatRead(DeviceVariable &deviceVar){
  Float64ReadResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}
WriteResult MqttDriver::floatWrite(DeviceVariable &deviceVar, epicsFloat64 value){
  WriteResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}

// read/write for arrays
template <typename epicsDataType>
ArrayReadResult MqttDriver::arrayRead(DeviceVariable &deviceVar, Array<epicsDataType> &value){
  ArrayReadResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}
template <typename epicsDataType>
WriteResult MqttDriver::arrayWrite(DeviceVariable &deviceVar, Array<epicsDataType> const &value){
  WriteResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}

// strings
OctetReadResult MqttDriver::stringRead(DeviceVariable &deviceVar, Octet &value){
  OctetReadResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}
WriteResult MqttDriver::stringWrite(DeviceVariable &deviceVar, Octet const &value){
  WriteResult result;
  asynStatus status = asynError;
  const char * functionName = __FUNCTION__;
  MqttTopicAddr const &addr =
      static_cast<MqttTopicAddr const &>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver *driver = static_cast<MqttTopicVariable &>(deviceVar).driver;

  result.status = status;
  return result;
}
  //#############################################################################################
  //EPICS shell script function definition
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
