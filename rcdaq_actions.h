#ifndef __RCDAQ_ACTIONS_H
#define __RCDAQ_ACTIONS_H

#define DAQ_BEGIN             101
#define DAQ_END               102
#define DAQ_OPEN              103
#define DAQ_CLOSE             104
#define DAQ_SETFILERULE       105
#define DAQ_SETRUNTYPE        106
#define DAQ_GETRUNTYPE        107
#define DAQ_DEFINERUNTYPE     108
#define DAQ_LISTRUNTYPES      109

#define DAQ_FAKETRIGGER       110
#define DAQ_SETMAXEVENTS      111
#define DAQ_SETMAXVOLUME      112
#define DAQ_SETMAXBUFFERSIZE  113

#define DAQ_DEVICE        114
#define DAQ_LISTREADLIST  115
#define DAQ_CLEARREADLIST 116
#define DAQ_STATUS        117
#define DAQ_FULLSTATUS    118

#define DAQ_RUNNUMBERFILE    119
#define DAQ_SETADAPTIVEBUFFER    120

#define DAQ_ELOG          121
#define DAQ_LOAD          122

#define DAQ_WEBCONTROL    123
#define DAQ_SETNAME       124
#define DAQ_GETNAME       125

#define DAQ_GETLASTFILENAME    126

#define DAQ_END_IMMEDIATE      127

#define DAQ_SET_SERVER        128

#define DAQ_BEGIN_IMMEDIATE   129

#define DAQ_SYNC              130

#define DAQ_ROLLOVERLIMIT     131

#define DAQ_SETEVENTFORMAT     132

//#define DAQ_OPENSQLSTREAM         133
//#define DAQ_CLOSESQLSTREAM        134

#define DAQ_SET_RUNCONTROLMODE       135
#define DAQ_GET_RUNCONTROLMODE       136

#define DAQ_SET_MQTT_HOST      137
#define DAQ_GET_MQTT_HOST      138

#define DAQ_GETLASTEVENTNUMBER  139

#define DAQ_SETRUNNUMBERAPP    140

#define DAQ_SET_COMPRESSION        141
#define DAQ_SET_MD5ENABLE          142
#define DAQ_SET_MD5ALLOWTURNOFF    143



#define DAQ_DEVICE_RANDOM         1001
#define DAQ_DEVICE_FILE           1002
#define DAQ_DEVICE_TSPMPROTO      1003
#define DAQ_DEVICE_TSPMPARAMS     1004
#define DAQ_DEVICE_FILENUMBERS    1005


#endif

