#ifndef MQTT_H
#define MQTT_H
#include <unistd.h>
#include <iocsh.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <asynPortDriver.h>
#include "mqttClient.h"
#include "devEpicsPvt.h"

static const char *driverName = "drvMQTT";

class drvMQTT : public asynPortDriver {
  public:
  /* Constructor */
    drvMQTT(const char *portName, const char *mqttBrokerAddr, const char *mqttClientID, const int qos, char *topicsRoot);
  /* Destructor */ 
    ~drvMQTT(); 
  /* Overrides from asynPortDriver */
    virtual asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value);
    virtual asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus readFloat64(asynUser *pasynUser, epicsFloat64 *value);
    virtual asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value);
    virtual asynStatus writeOctet(asynUser *pasynUser, const char *value, size_t maxChars, size_t *nActual);
    virtual asynStatus readOctet(asynUser *pasynUser, char *value, size_t maxChars, size_t *nActual, int *eomReason);

  /* driver-specific methods */
    void createEPICSParams();

  protected:
    /* Values used for pasynUser->reason, and indexes into the parameter library. */
    int connectionStatus_;
    #define  FIRST_MQTT_PARAM connectionStatus_
    int brokerUrl_;
    int clientID_;
    int connectionQos_;
    int connectionTimeout_;
    int data_;
    int structuredData_;
    #define LAST_MQTT_PARAM structuredData_
  private:
    /* */
};

#endif /* MQTT_H */
