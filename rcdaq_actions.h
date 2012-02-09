#ifndef __RCDAQ_ACTIONS_H
#define __RCDAQ_ACTIONS_H

#define DAQ_BEGIN             101
#define DAQ_END               102
#define DAQ_OPEN              103
#define DAQ_CLOSE             104
#define DAQ_SETFILERULE       105
#define DAQ_FAKETRIGGER       106
#define DAQ_SETMAXEVENTS      107
#define DAQ_SETMAXVOLUME      108
#define DAQ_SETMAXBUFFERSIZE  109

#define DAQ_DEVICE        110
#define DAQ_LISTREADLIST  111
#define DAQ_CLEARREADLIST 112
#define DAQ_STATUS        113
#define DAQ_FULLSTATUS    114

#define DAQ_ELOG          120
#define DAQ_LOAD          121

#define DAQ_DEVICE_RANDOM         1001
#define DAQ_DEVICE_FILE           1002
#define DAQ_DEVICE_TSPMPROTO      1003
#define DAQ_DEVICE_TSPMPARAMS     1004
#define DAQ_DEVICE_FILENUMBERS    1005


#endif

