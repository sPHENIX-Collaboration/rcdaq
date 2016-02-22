
// device_i.cc
/* This defines the device class.  
 * This module inherits
 * its fsm from the activecomponent class
 * Its static implementation is inherited from the component class
 *
 * created June 9,1997
 *
 * author:
 *
 * module = %M% date = %D% Version = %I%
 * modifications: 
 *	none
 */


#include <iostream>
#include "daq_device.h"


daq_device::daq_device()
{

}



daq_device::~daq_device() 
{
}


int daq_device::formatPacketHdr(int * adr)
{
  packetdata_ptr sevt =  (packetdata_ptr) adr;

  sevt->sub_length = 6;
  
  // sevt->hdrversion = 1;
  // sevt->hdrlength = 6;
  // sevt->status = 0;
  
  sevt->hdrinfo = 0x01060000;

  sevt->sub_id =0;
  
  sevt->debug_length =0;
  sevt->error_length =0;
  
  // sevt->structure =0;
  // sevt->numDescWds =1;
  // sevt->endianism=2;
  // sevt->sub_padding=0;

  sevt->structureinfo = 0x00010200;

  sevt->sub_type =4;
  sevt->sub_decoding =0;
  return 0;
}

  
