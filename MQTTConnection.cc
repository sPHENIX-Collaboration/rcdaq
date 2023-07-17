
#include <iostream>
#include <sstream>
#include "MQTTConnection.h"

#include <unistd.h>

using namespace std;

MQTTConnection::MQTTConnection( const std::string hostname, const std::string topic, const int port)
{
  _hostname = hostname;
  _topic = topic;
  _port=port;
  _status = 0;
  
  mosquitto_lib_init();
  
  mosq = mosquitto_new(NULL, true, NULL);
  if (mosq == NULL)
    {
      cerr << "Failed to create Mosquitto object" << endl;
      _status =-3;
    }

  if ( OpenConnection())
    {
      cerr << "Failed to connect to server" << endl;
    }
  CloseConnection();
  
  
}

int MQTTConnection::send(const std::string message)
{
  //cout << __FILE__ << " " << __LINE__ << " sending message " << message; 
  if ( OpenConnection())
    {
      return _status;
    }
  int s = mosquitto_publish(mosq, NULL, _topic.c_str(), message.size(), message.c_str(), 0, false);
  if (s != MOSQ_ERR_SUCCESS)
    {
      cerr << "Failed to publish message: " <<  mosquitto_strerror(s) << endl;
      mosquitto_destroy(mosq);
      _status = -2;
      return _status;
      }
  CloseConnection();
  return 0;
}
				
MQTTConnection::~MQTTConnection()
{
  CloseConnection();
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

}

int MQTTConnection::OpenConnection()
{

  int rc = mosquitto_connect(mosq, _hostname.c_str(), _port, 60);
  if (rc != MOSQ_ERR_SUCCESS)
    {
      cerr << "Failed to connect to MQTT broker: " << mosquitto_strerror(rc) << " on port " << _port << endl;
      mosquitto_destroy(mosq);
      _status = -2;
      return _status;
    }
  _status = 0;
  return 0;

}

int MQTTConnection::CloseConnection()
{
  mosquitto_disconnect(mosq);
  //  mosquitto_destroy(mosq);
  return 0;
}

