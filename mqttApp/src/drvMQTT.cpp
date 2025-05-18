#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include <string.h>
#include <map>
#include <devEpicsPvt.h>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "drvMQTT.h"
#include "mqttClient.h"


/* Class constructor
 * @param portName Asyn port name to be used 
 * @param brokerUrl Broker IP or hostname (e.g: mqtt://localhost:1883)
 * @param mqttClientID ClientID to be used - must be unique
 * @param qos Desired quality of service (QoS) for the connection [0|1|2]
 * @param timeout Connection timeout to be used 
 * @param topicsRoot Root path of the topics to be subscribed (e.g: foo/mydevice)
*/
drvMQTT::drvMQTT(const char *portName, const char *brokerUrl, const char *mqttClientID, const int qos, char *topicsRoot)
   : asynPortDriver(portName,
        1, /* maxAddr */
        asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynDrvUserMask | asynOctetMask, /* Interface mask */
        asynInt32Mask | asynFloat64Mask | asynFloat64ArrayMask | asynOctetMask,  /* Interrupt mask */
        0, /* asynFlags.  This driver does not block and it is not multi-device, so flag is 0 */
        1, /* Autoconnect */
        0, /* Default priority */
        0) /* Default stack size*/
{   
    createEPICSParams();
    MqttClient::Config cfg;
    cfg.brokerUrl = brokerUrl;
    cfg.clientId  = mqttClientID;
    cfg.qos       = qos;
    // cfg.sslCert   =;
    // cfg.sslKey    =;
    MqttClient client(cfg);
    client.connect();
    // client.subscribe("test/topic");
    // client.publish("test/topic", "Hello MQTT");

    std::this_thread::sleep_for(std::chrono::seconds(5));
    // subscribeToRoot(client, topicsRoot);
    client.setMessageCallback([](const std::string& topic, const std::string& payload) {
        std::cout << "Received message on topic [" << topic << "]: " << payload << std::endl;
    });
}

void drvMQTT::createEPICSParams() {
    createParam("CONNECTION_STATUS",   asynParamInt32,   &connectionStatus_);   // r
    createParam("BROKER_URL",          asynParamOctet,   &brokerUrl_);         // r
    createParam("CLIENT_ID",           asynParamOctet,   &clientID_);           // r
    createParam("CONNECTION_QOS",      asynParamInt32,   &connectionQos_);      // r
    createParam("CONNECTION_TIMEOUT",  asynParamInt32,   &connectionTimeout_);  // r
    createParam("MQTT_DATA",           asynParamInt32,   &data_);               // rw
    createParam("MQTT_STRUCT_DATA",    asynParamOctet,   &structuredData_);     // rw
}

//--------------------------------------------------------------------------------------------
// Class destructor
drvMQTT::~drvMQTT() {

}

//#############################################################################################
asynStatus drvMQTT::readInt32(asynUser *pasynUser, epicsInt32 *value){
    asynStatus status = asynSuccess;
    int function;
    int addr;
    const char* paramName;
    const char* functionName = __FUNCTION__;
    parseAsynUser(pasynUser, &function, &addr, &paramName);

    return status;
}

asynStatus drvMQTT::writeInt32(asynUser *pasynUser, epicsInt32 value){
  asynStatus status = asynSuccess;
  int function;
  int addr;
  const char* paramName;
  const char* functionName = __FUNCTION__;
  parseAsynUser(pasynUser, &function, &addr, &paramName);

  return status;
}

asynStatus drvMQTT::readFloat64(asynUser *pasynUser, epicsFloat64 *value){
  asynStatus status = asynSuccess;
  int function;
  int addr;
  const char* paramName;
  const char* functionName = __FUNCTION__;
  parseAsynUser(pasynUser, &function, &addr, &paramName);

  return status;
}

asynStatus drvMQTT::writeFloat64(asynUser *pasynUser, epicsFloat64 value){
  asynStatus status = asynSuccess;
  int function;
  int addr;
  const char* paramName;
  const char* functionName = __FUNCTION__;
  parseAsynUser(pasynUser, &function, &addr, &paramName);

  return status;
}

asynStatus drvMQTT::readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason){
  asynStatus status = asynSuccess;
  int function;
  int addr;
  const char* paramName;
  const char* functionName = __FUNCTION__;
  parseAsynUser(pasynUser, &function, &addr, &paramName);

  return status;
}

asynStatus drvMQTT::writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual){
  asynStatus status = asynSuccess;
  int function;
  int addr;
  const char* paramName;
  const char* functionName = __FUNCTION__;
  parseAsynUser(pasynUser, &function, &addr, &paramName);

  return status;
}
//#############################################################################################
std::string getTopicFromRecord(dbCommon* pRecord) {
    const char* topic = asynDbGetInfo(pRecord, "mqtt:TOPIC");
    return topic ? std::string(topic) : "";
}

std::string getStructFieldFromRecord(dbCommon* pRecord) {
    const char* field = asynDbGetInfo(pRecord, "mqtt:FIELD");
    return field ? std::string(field) : "";
}
//#############################################################################################
extern "C" {
    int drvMQTTConfigure(const char *portName, const char *brokerUrl, const char *mqttClientID, const int qos, char *topicsRoot) {
        new drvMQTT(portName, brokerUrl, mqttClientID, qos, topicsRoot);
        return(asynSuccess);
    }

    static const iocshArg initArg0 = { "portName", iocshArgString};
    static const iocshArg initArg1 = { "brokerUrl", iocshArgString};
    static const iocshArg initArg2 = { "mqttClientID", iocshArgString};
    static const iocshArg initArg3 = { "qos", iocshArgInt};
    static const iocshArg initArg4 = { "topicsRoot", iocshArgString};
    static const iocshArg * const initArgs[] = {
        &initArg0,
        &initArg1,
        &initArg2,
        &initArg3,
        &initArg4,
    };

//#############################################################################################
    static const iocshFuncDef initFuncDef = {"drvMQTTConfigure", 5, initArgs};

//#############################################################################################
    static void initCallFunc(const iocshArgBuf *args) {
        drvMQTTConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].ival, args[4].sval);
    }

//#############################################################################################
    void drvMQTTRegister(void) {
        iocshRegister(&initFuncDef, initCallFunc);
    }

    epicsExportRegistrar(drvMQTTRegister);
}
