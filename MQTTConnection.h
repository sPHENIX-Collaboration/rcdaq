#ifndef __MQTTCONNECTION_H__
#define __MQTTCONNECTION_H__

#include <string>
#include <mosquitto.h>


class MQTTConnection {

 public:

  MQTTConnection ( const std::string hostname, const std::string topic, const int port=1883);
  virtual ~MQTTConnection();

  virtual int Status() const { return _status;};
  virtual std::string GetHostName() const { return _hostname;};
  virtual int GetPort() const { return _port;};
  
  virtual int send(const std::string message); 

 protected:

  int OpenConnection();
  int CloseConnection();

  std::string _hostname;
  std::string _topic;
  int _status;
  int _port;

  struct mosquitto *mosq;

};

#endif
