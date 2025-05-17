#include <epicsTimer.h>
#include <epicsMutex.h>
#include <epicsEvent.h>

#include <string.h>
#include <map>
#include "devEpicsPvt.h"
#include "drvMQTT.h"

#include "mqtt/async_client.h"


// Class constructor
drvMQTT::drvMQTT(const char *portName, const char *mqttBrokerAddr, const char *mqttClientID, const int qos, const int timeout)
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
}

void drvMQTT::createEPICSParams() {
    // createParam("CONTF_INIT_STATE",        asynParamInt32,      &contFlowInitState_);      // rw
    // createParam("FLOW_RATE_MON",           asynParamFloat64,    &flowRateMon_);            // r
    // createParam("POS_INIT_STATE",          asynParamInt32,      &posInitState_);           // rw
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

//#############################################################################################
std::string getTopicFromRecord(dbCommon* pRecord) {
    const char* topic = asynDbGetInfo(pRecord, "mqtt:TOPIC");
    return topic ? std::string(topic) : "";
}
//#############################################################################################
extern "C" {

    int drvMQTTConfigure(const char *portName, const char *mqttBrokerAddr, const char *mqttClientID, const int qos, const int timeout) {
        new drvMQTT(portName, mqttBrokerAddr, mqttClientID, qos, timeout);
        return(asynSuccess);
    }

    static const iocshArg initArg0 = { "portName", iocshArgString};
    static const iocshArg initArg1 = { "mqttBrokerAddr", iocshArgString};
    static const iocshArg initArg2 = { "mqttClientID", iocshArgString};
    static const iocshArg initArg3 = { "qos", iocshArgInt};
    static const iocshArg initArg4 = { "timeout", iocshArgInt};
    static const iocshArg * const initArgs[] = {
        &initArg0,
        &initArg1,
        &initArg2,
        &initArg3,
        &initArg4,
    };

//#############################################################################################
    static const iocshFuncDef initFuncDef = {"drvMQTTConfigure",5,initArgs};

//#############################################################################################
    static void initCallFunc(const iocshArgBuf *args) {
        drvMQTTConfigure(args[0].sval, args[1].sval, args[2].sval, args[3].ival, args[4].ival);
    }

//#############################################################################################
    void drvMQTTRegister(void) {
        iocshRegister(&initFuncDef, initCallFunc);
    }

    epicsExportRegistrar(drvMQTTRegister);
}
