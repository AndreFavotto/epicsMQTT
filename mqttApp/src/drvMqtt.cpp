#include "drvMqtt.h"
#include "mqttClient.h"

// Supported type definitions

#define FLAT_FUNC_PREFIX          "FLAT"
#define JSON_FUNC_PREFIX          "JSON"
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
  const MqttTopicAddr& cmp = static_cast<const MqttTopicAddr&>(comparedAddr);
  if (format != cmp.format) return false;
  switch (format) {
    case Flat:
      return topicName == cmp.topicName;
    case Json:
      return topicName == cmp.topicName && jsonField == cmp.jsonField;
  }
  return false;
}

DeviceAddress* MqttDriver::parseDeviceAddress(std::string const& function, std::string const& arguments) {
  MqttTopicAddr* addr = new MqttTopicAddr;
  std::istringstream argsStream(arguments);
  auto colonPos = function.find(':');
  std::string prefix = (colonPos == std::string::npos) ? function : function.substr(0, colonPos);
  std::string type = (colonPos == std::string::npos) ? "" : function.substr(colonPos + 1);
  //TODO: add error handling for malformed arguments
  if (prefix == FLAT_FUNC_PREFIX) {
    addr->format = MqttTopicAddr::Flat;
    argsStream >> addr->topicName;
  }
  else if (prefix == JSON_FUNC_PREFIX) {
    addr->format = MqttTopicAddr::Json;
    argsStream >> addr->topicName;
    argsStream >> addr->jsonField;
  }
  else {
    delete addr;
    return nullptr;
  }

  return addr;
}

DeviceVariable* MqttDriver::createDeviceVariable(DeviceVariable* baseVar) {
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
MqttDriver::MqttDriver(const char* portName, const char* brokerUrl, const char* mqttClientID, const int qos)
  : Autoparam::Driver(
    portName,
    Autoparam::DriverOpts()
    .setBlocking(false)
    .setAutoInterrupts(false)
    .setInitHook(initHook)),
  mqttClient([&] {
  MqttClient::Config cfg;
  cfg.brokerUrl = brokerUrl;
  cfg.clientId = mqttClientID;
  cfg.qos = qos;
  return cfg;
    }())
{
  mqttClient.connect();
  /*
    since we cannot actively read MQTT topics due to the publish/subscribe
    nature of MQTT - handled as I/O Intr - we won't register any custom read functions,
    and leave it to the default asynPortDriver implementation.
  */

  // flat topic support
  registerHandlers<epicsInt32>(FLAT_INT_FUNC_STR, NULL, integerWrite, NULL);
  registerHandlers<epicsFloat64>(FLAT_FLOAT_FUNC_STR, NULL, floatWrite, NULL);
  registerHandlers<epicsUInt32>(FLAT_DIGITAL_FUNC_STR, NULL, digitalWrite, NULL);
  registerHandlers<Octet>(FLAT_STRING_FUNC_STR, NULL, stringWrite, NULL);
  registerHandlers<Array<epicsInt32>>(FLAT_INTARRAY_FUNC_STR, NULL, arrayWrite, NULL);
  registerHandlers<Array<epicsFloat64>>(FLAT_FLOATARRAY_FUNC_STR, NULL, arrayWrite, NULL);

  // json topic support
  registerHandlers<epicsInt32>(JSON_INT_FUNC_STR, NULL, integerWrite, NULL);
  registerHandlers<epicsFloat64>(JSON_FLOAT_FUNC_STR, NULL, floatWrite, NULL);
  registerHandlers<epicsUInt32>(JSON_DIGITAL_FUNC_STR, NULL, digitalWrite, NULL);
  registerHandlers<Octet>(JSON_STRING_FUNC_STR, NULL, stringWrite, NULL);
  registerHandlers<Array<epicsInt32>>(JSON_INTARRAY_FUNC_STR, NULL, arrayWrite, NULL);
  registerHandlers<Array<epicsFloat64>>(JSON_FLOATARRAY_FUNC_STR, NULL, arrayWrite, NULL);

  // TODO: instead of sleeping, we should wait for connection callback
  std::this_thread::sleep_for(std::chrono::seconds(2));
}
/* Class destructor
   - Disconnects from the broker and cleans session
*/
MqttDriver::~MqttDriver() {
  mqttClient.disconnect();
}

void MqttDriver::initHook(Autoparam::Driver* driver) {
  // TODO: this procedure should also be done as a callback to the "on_reconnect" event for the MQTT Client
  auto* pself = static_cast<MqttDriver*>(driver);
  auto vars = pself->getInterruptVariables();
  for (auto itr = vars.begin(); itr != vars.end(); itr++) {
    auto& deviceVar = *static_cast<MqttTopicVariable*>(*itr);
    MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
    pself->mqttClient.subscribe(addr.topicName);
  }
  pself->mqttClient.setMessageCallback([pself](const std::string& topic, const std::string& payload) {
    pself->handleMqttMessage(pself, topic, payload);
    });
}

void MqttDriver::handleMqttMessage(Autoparam::Driver* driver, const std::string& topic, const std::string& payload) {
  const char* functionName = __FUNCTION__;
  auto* pself = static_cast<MqttDriver*>(driver);
  pself->lock();
  auto vars = pself->getInterruptVariables();
  for (auto itr = vars.begin(); itr != vars.end(); itr++) {
    auto& deviceVar = *static_cast<MqttTopicVariable*>(*itr);
    MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
    if (addr.topicName != topic)
      continue;
    int index = deviceVar.asynIndex();
    try {
      switch (deviceVar.asynType()) {
        case asynParamInt32:
          if (!isInteger(payload)) throw std::invalid_argument("Invalid integer");
          pself->setParam(deviceVar, std::stoi(payload), asynSuccess);
          break;
        case asynParamFloat64:
          if (!isFloat(payload)) throw std::invalid_argument("Invalid float");
          pself->setParam(deviceVar, std::stod(payload), asynSuccess);
          break;
        case asynParamUInt32Digital:
          if (!isInteger(payload, false)) throw std::invalid_argument("Invalid unsigned integer");
          pself->setParam(deviceVar, static_cast<epicsUInt32>(std::stoul(payload)), asynSuccess);
          break;
        case asynParamOctet:
          // use asyn directly - setParam octet overload is not defined in runtime. TODO: investigate
          pself->setStringParam(index, payload.c_str());
          break;
        case asynParamInt32Array:
        {
          std::vector<epicsInt32> auxArray;
          asynStatus parseStatus = checkAndParseIntArray(payload, auxArray);
          Autoparam::Array<epicsInt32> dataArray(auxArray.data(), auxArray.size());
          if (parseStatus == asynSuccess) {
            pself->doCallbacksArray(deviceVar, dataArray, asynSuccess);
          }
          else throw std::invalid_argument("Failed parsing integer array");
          break;
        }
        case asynParamFloat64Array:
        {
          std::vector<epicsFloat64> auxArray;
          asynStatus parseStatus = checkAndParseFloatArray(payload, auxArray);
          Autoparam::Array<epicsFloat64> dataArray(auxArray.data(), auxArray.size());
          if (parseStatus == asynSuccess) {
            pself->doCallbacksArray(deviceVar, dataArray, asynSuccess);
          }
          else throw std::invalid_argument("Failed parsing float array");
          break;
        }
        default:
          break;
      }
    }
    catch (const std::exception& e) {
      asynPrint(pself->pasynUserSelf, ASYN_TRACE_ERROR,
        "%s::%s:%s: Unexpected value received for topic: '%s': %s)\n",
        driverName, functionName, e.what(), addr.topicName.c_str(), payload.c_str());
    }
  }
  pself->callParamCallbacks();
  pself->unlock();
}
//#############################################################################################
// Helper methods

/* Checks if a string represents a signed or unsigned integer */
bool MqttDriver::isInteger(const std::string& s, bool isSigned) {
  if (s.empty()) return false;
  size_t i = 0;
  if (isSigned && isSign(s[i])) ++i;
  if (i == s.size()) return false; // message has only sign and/or no digits
  for (; i < s.size(); ++i) {
    if (!std::isdigit(static_cast<unsigned char>(s[i]))) return false;
  }
  return true;
}

/* Checks if a string represents a float */
bool MqttDriver::isFloat(const std::string& s) {
  if (s.empty()) return false;
  size_t i = 0;
  if (isSign(s[i])) ++i;
  if (i == s.size()) return false;
  char* end = nullptr;
  std::strtof(s.c_str(), &end);
  return end == s.c_str() + s.size();
}

/* Checks if a character is a + or - sign */
bool MqttDriver::isSign(char character) {
  return character == '-' || character == '+';
}
/*
  Checks the validity and parses the int array represented by a string into an epicsInt32 vector.
  Handles strings with:

  - Optional wrapping brackets ([ ]) - if one bracket is present, both must be;

  - Comma separators with or without spaces (',' or ', ');

  - Single space separators (' ');

  - Trailing spaces (in case of comma separators);

  - Signed digits.

  @param s: string to be parsed
  @param out: pointer to epicsInt32 vector to be filled with data
  @return asynStatus

*/
asynStatus MqttDriver::checkAndParseIntArray(const std::string& s, std::vector<epicsInt32>& out) {
  out.clear();
  if (s.empty()) return asynError;

  size_t i = 0;
  size_t end = s.size() - 1;
  const char comma = ',';
  const char space = ' ';
  const char openBracket = '[';
  const char closeBracket = ']';
  bool separatorIsKnown = false;
  char separator;

  if (s[i] == openBracket) {
    if (s[end] != closeBracket) return asynError;
    end--;
    i++;
  }
  if (i > end) return asynError; // empty content

  while (i <= end) {
    while (i <= end && std::isspace(s[i])) i++;
    if (i > end) return asynError;

    int sign = 1;
    if (isSign(s[i])) {
      if (s[i] == '-') sign = -1;
      i++;
      if (i > end) return asynError;
    }

    int val = 0;
    bool hasDigit = false;
    while (i <= end && std::isdigit(s[i])) {
      val = val * 10 + (s[i] - '0');
      i++;
      hasDigit = true;
    }
    if (!hasDigit) return asynError;
    out.push_back(sign * val);

    if (i > end) break;

    if (!separatorIsKnown) {
      if (std::isspace(s[i])) {
        separator = space;
      }
      else if (s[i] == comma) {
        separator = comma;
      }
      else return asynError;
      separatorIsKnown = true;
    }
    if (s[i] != separator) return asynError;
    i++;
    if (separator == comma) {
      while (i <= end && std::isspace(s[i])) i++;
    }
    if (i > end) return asynError;
  }

  return asynSuccess;
}

/*
  Checks the validity and parses the float array represented by a string into an epicsFloat64 vector.
  Handles strings with:

  - Optional wrapping brackets ([ ]) - if one bracket is present, both must be;

  - Comma separators with or without spaces (',' or ', ');

  - Single space separators (' ');

  - Trailing spaces (in case of comma separators);

  - Signed digits.

  @param s: string to be parsed
  @param out: pointer to epicsFloat64 vector to be filled with data
  @return asynStatus

*/
asynStatus MqttDriver::checkAndParseFloatArray(const std::string& s, std::vector<epicsFloat64>& out) {
  out.clear();
  if (s.empty()) return asynError;

  size_t i = 0;
  size_t end = s.size() - 1;
  const char comma = ',';
  const char space = ' ';
  const char openBracket = '[';
  const char closeBracket = ']';
  bool separatorIsKnown = false;
  char separator;

  if (s[i] == openBracket) {
    if (s[end] != closeBracket) return asynError;
    end--;
    i++;
  }
  if (i > end) return asynError;

  const char* strStart = s.c_str();

  while (i <= end) {
    while (i <= end && std::isspace(s[i])) i++;
    if (i > end) return asynError;

    char* endPtr = nullptr;
    const char* startPtr = strStart + i;
    // strod parses the first valid float value found and puts position of last digit in endPtr
    double val = std::strtod(startPtr, &endPtr);
    if (startPtr == endPtr) return asynError;
    size_t parsed = static_cast<size_t>(endPtr - startPtr);
    i += parsed;
    out.push_back(val);

    if (i > end) break;

    if (!separatorIsKnown) {
      if (std::isspace(s[i])) {
        separator = space;
      }
      else if (s[i] == comma) {
        separator = comma;
      }
      else {
        return asynError;
      }
      separatorIsKnown = true;
    }

    if (s[i] != separator) return asynError;
    i++;
    if (separator == comma) {
      while (i <= end && std::isspace(s[i])) i++;
    }
    if (i > end) return asynError;
  }

  return asynSuccess;
}

//#############################################################################################
// IO function definitions

WriteResult MqttDriver::integerWrite(DeviceVariable& deviceVar, epicsInt32 value) {
  WriteResult result;
  asynStatus status = asynError;
  const char* functionName = __FUNCTION__;
  MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver* driver = static_cast<MqttTopicVariable&>(deviceVar).driver;
  try {
    if (addr.format == MqttTopicAddr::TopicFormat::Flat) {
      driver->mqttClient.publish(addr.topicName, std::to_string(value));
      status = asynSuccess;
    }
    else if (addr.format == MqttTopicAddr::TopicFormat::Json) {
      // TODO: implement JSON support for integer values
      throw std::logic_error("JSON support not implemented");
    }
  }
  catch (const std::exception& exc) {
    status = asynError;
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s: Failed to set value for topic '%s': %s. asynStatus: %d)\n",
      driverName, functionName, topicName.c_str(), exc.what(), status);
  }
  result.status = status;
  return result;
}

WriteResult MqttDriver::digitalWrite(DeviceVariable& deviceVar, epicsUInt32 const value, epicsUInt32 const mask) {
  WriteResult result;
  asynStatus status = asynError;
  const char* functionName = __FUNCTION__;
  MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver* driver = static_cast<MqttTopicVariable&>(deviceVar).driver;
  epicsUInt32 outVal = value;
  try {
    if (addr.format == MqttTopicAddr::TopicFormat::Flat) {
      if (mask != 0xFFFFFFFF) {
        // read current value to avoid overwriting other bits when applying mask
        epicsUInt32 auxVal;
        status = driver->getUIntDigitalParam(deviceVar.asynIndex(), &auxVal, 0xFFFFFFFF);
        if (status == asynParamUndefined) {
          throw std::logic_error("Masked write attempted on uninitialized value (current topic value is unknown)");
        }
        else if (status != asynSuccess) {
          throw std::logic_error("Error reading current param value");
        }
        auxVal |= (value & mask);
        auxVal &= (value | ~mask);
        outVal = auxVal;
      }
      driver->mqttClient.publish(addr.topicName, std::to_string(outVal));
      status = asynSuccess;
    }
    else if (addr.format == MqttTopicAddr::TopicFormat::Json) {
      // TODO: implement JSON support for digital values
      throw std::logic_error("JSON support not implemented");
    }
  }
  catch (const std::exception& exc) {
    status = asynError;
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s: Failed to set value for topic '%s': %s. asynStatus: %d)\n",
      driverName, functionName, topicName.c_str(), exc.what(), status);
  }
  result.status = status;
  return result;
}

WriteResult MqttDriver::floatWrite(DeviceVariable& deviceVar, epicsFloat64 value) {
  WriteResult result;
  asynStatus status = asynError;
  const char* functionName = __FUNCTION__;
  MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver* driver = static_cast<MqttTopicVariable&>(deviceVar).driver;
  try {
    if (addr.format == MqttTopicAddr::TopicFormat::Flat) {
      driver->mqttClient.publish(addr.topicName, std::to_string(value));
      status = asynSuccess;
    }
    else if (addr.format == MqttTopicAddr::TopicFormat::Json) {
      // TODO: implement JSON support for float values
      throw std::logic_error("JSON support not implemented");
    }
  }
  catch (const std::exception& exc) {
    status = asynError;
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s: Failed to set value for topic '%s': %s. asynStatus: %d)\n",
      driverName, functionName, topicName.c_str(), exc.what(), status);
  }
  result.status = status;
  return result;
}

template <typename epicsDataType>
WriteResult MqttDriver::arrayWrite(DeviceVariable& deviceVar, Array<epicsDataType> const& value) {
  WriteResult result;
  asynStatus status = asynError;
  const char* functionName = __FUNCTION__;
  MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver* driver = static_cast<MqttTopicVariable&>(deviceVar).driver;
  try {
    if (addr.format == MqttTopicAddr::TopicFormat::Flat) {
      const epicsDataType* arrayData = reinterpret_cast<const epicsDataType*>(value.data());
      size_t count = value.size();

      std::ostringstream oss;
      for (size_t i = 0; i < count; ++i) {
        if (i > 0) oss << ",";
        oss << arrayData[i];
      }
      driver->mqttClient.publish(topicName, oss.str());
      status = asynSuccess;
    }
    else if (addr.format == MqttTopicAddr::TopicFormat::Json) {
      // TODO: implement JSON support for array values
      throw std::logic_error("JSON support not implemented");
    }
  }
  catch (const std::exception& exc) {
    status = asynError;
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s: Failed to set value for topic '%s': %s. asynStatus: %d)\n",
      driverName, functionName, topicName.c_str(), exc.what(), status);
  }
  result.status = status;
  return result;
}

WriteResult MqttDriver::stringWrite(DeviceVariable& deviceVar, Octet const& value) {
  WriteResult result;
  asynStatus status = asynError;
  const char* functionName = __FUNCTION__;
  MqttTopicAddr const& addr = static_cast<MqttTopicAddr const&>(deviceVar.address());
  std::string topicName = addr.topicName;
  MqttDriver* driver = static_cast<MqttTopicVariable&>(deviceVar).driver;
  try {
    if (addr.format == MqttTopicAddr::TopicFormat::Flat) {
      char* stringData;
      value.writeTo(stringData, value.maxSize());
      driver->mqttClient.publish(addr.topicName, stringData);
      status = asynSuccess;
    }
    else if (addr.format == MqttTopicAddr::TopicFormat::Json) {
      // TODO: implement JSON support for string values
      throw std::logic_error("JSON support not implemented");
    }
  }
  catch (const std::exception& exc) {
    status = asynError;
    asynPrint(driver->pasynUserSelf, ASYN_TRACE_ERROR,
      "%s::%s: Failed to set value for topic '%s': %s. asynStatus: %d)\n",
      driverName, functionName, topicName.c_str(), exc.what(), status);
  }
  result.status = status;
  return result;
}
//#############################################################################################
//EPICS shell script function definition
extern "C" {
  int mqttDriverConfigure(const char* portName, const char* brokerUrl, const char* mqttClientID, const int qos) {
    new MqttDriver(portName, brokerUrl, mqttClientID, qos);
    return(asynSuccess);
  }
  static const int numArgs = 4;
  static const iocshArg initArg0 = { "portName", iocshArgString };
  static const iocshArg initArg1 = { "brokerUrl", iocshArgString };
  static const iocshArg initArg2 = { "mqttClientID", iocshArgString };
  static const iocshArg initArg3 = { "qos", iocshArgInt };
  static const iocshArg* const initArgs[] = {
      &initArg0,
      &initArg1,
      &initArg2,
      &initArg3
  };
  static const char* usage =
    "MqttDriverConfigure(portName, brokerUrl, mqttClientID, qos)\n"
    "  portName: Asyn port name to be used\n"
    "  brokerUrl: Broker IP or hostname (e.g: mqtt://localhost:1883)\n"
    "  mqttClientID: ClientID to be used - must be unique\n"
    "  qos: Desired quality of service (QoS) for the connection [0|1|2]\n";

  //#############################################################################################
  static const iocshFuncDef initFuncDef = { "mqttDriverConfigure", numArgs, initArgs, usage };

  //#############################################################################################
  static void initCallFunc(const iocshArgBuf* args) {
    mqttDriverConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].ival);
  }

  //#############################################################################################
  void mqttDriverRegister(void) {
    iocshRegister(&initFuncDef, initCallFunc);
  }

  epicsExportRegistrar(mqttDriverRegister);
}
