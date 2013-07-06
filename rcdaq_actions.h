#ifndef __RCDAQ_ACTIONS_H
#define __RCDAQ_ACTIONS_H

#define DAQ_BEGIN             101
#define DAQ_END               102
#define DAQ_OPEN              103
#define DAQ_CLOSE             104
#define DAQ_SETFILERULE       105
#define DAQ_SETRUNTYPE        106
#define DAQ_DEFINERUNTYPE     107
#define DAQ_LISTRUNTYPES      108

#define DAQ_FAKETRIGGER       109
#define DAQ_SETMAXEVENTS      110
#define DAQ_SETMAXVOLUME      111
#define DAQ_SETMAXBUFFERSIZE  112

#define DAQ_DEVICE        113
#define DAQ_LISTREADLIST  114
#define DAQ_CLEARREADLIST 115
#define DAQ_STATUS        116
#define DAQ_FULLSTATUS    117

#define DAQ_RUNNUMBERFILE    118
#define DAQ_SETADAPTIVEBUFFER    119

#define DAQ_ELOG          120
#define DAQ_LOAD          121

#define DAQ_DEVICE_RANDOM         1001
#define DAQ_DEVICE_FILE           1002
#define DAQ_DEVICE_TSPMPROTO      1003
#define DAQ_DEVICE_TSPMPARAMS     1004
#define DAQ_DEVICE_FILENUMBERS    1005


#endif

