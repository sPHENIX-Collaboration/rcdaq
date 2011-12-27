
#include <iostream>

#include <daq_device_tspmproto.h>
#include <string.h>

#define PORT 32003

#define MAX_PACKETS 1

using namespace std;

daq_device_tspmproto::daq_device_tspmproto(const int eventtype
					   , const int subeventid
					   , const char *ipaddr)
{

  m_eventType  = eventtype;
  m_subeventid = subeventid;
  strcpy ( _ip, ipaddr);
  _s = 0;
  _th = 0;

  //  cout << __LINE__ << "  " << __FILE__ << " in init" << endl;
  if ( _s ) 
    {
      close (_s);
      _s = 0;
    }

  _broken = 0;


  if ((_s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
      _broken =1;
      cout << __LINE__ << "  " << __FILE__ << " broken 1" << endl;
      perror("xxx");
    }

  memset((char *) &_si_me, 0, sizeof(_si_me));
  _si_me.sin_family = AF_INET;
  _si_me.sin_port = htons(PORT);
  _si_me.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(_s, (sockaddr *)&_si_me, sizeof(_si_me))==-1)
   {
      _broken =1;
      cout << __LINE__ << "  " << __FILE__ << " broken 1" << endl;
      perror("bind");

    }

  FD_ZERO(&read_flag);
  FD_SET(_s, &read_flag);

  //  cout << __LINE__ << "  " << __FILE__ << " registering triggerhandler " << endl;
  _th  = new tspmTriggerHandler ( &read_flag, _s);
  registerTriggerHandler ( _th);





}

daq_device_tspmproto::~daq_device_tspmproto()
{
  close( _s );
}



// the put_data function

int daq_device_tspmproto::put_data(const int etype, int * adr, const int length )
{

  if ( _broken ) 
    {
      //      cout << __LINE__ << "  " << __FILE__ << " broken ";
      //      identify();
      return 0; //  we had a catastrophic failure
    }

  int len = 0;

  if (etype != m_eventType )  // not our id
    {
      return 0;
    }

  sevt =  (subevtdata_ptr) adr;
  // set the initial subevent length
  sevt->sub_length =  SEVTHEADERLENGTH;

  // update id's etc
  sevt->sub_id =  m_subeventid;
  sevt->sub_type=4;
  sevt->sub_decoding = 62;
  sevt->reserved[0] = 0;
  sevt->reserved[1] = 0;

  if ( length < max_length(etype) ) 
    {
      cout << __LINE__ << "  " << __FILE__ << " length " << length <<endl;
      return 0;
    }

  unsigned int data;

  char *d = (char *) &sevt->data;

  struct timeval tv;



  struct sockaddr_in si_src;
  socklen_t src_len;
  int ia;
  int received_len; 

  for ( ia = 0; ia < MAX_PACKETS; ia++)  //receive 
    {

      tv.tv_sec = 60;
      tv.tv_usec = 0;
      int retval = select(_s+1, &read_flag, NULL, NULL, &tv);
      if ( retval >0) 
      	{

	  if ( ( received_len = recvfrom(_s, d, 4*256, 0, (sockaddr *) &si_src, &src_len) ) ==-1)
	    {
	      cout << __LINE__ << "  " << __FILE__ << " error " << received_len <<endl;
	      return 0;
	    }
	  len += received_len;   // we are still counting bytes here
	  d+= received_len;     // and here - update the write pointer
	}
      else
	{

	  // cout << __LINE__ << "  " << __FILE__ << " timeout in ";
	  //identify();
	  return 0;
	}

    }

  len /= 4;  // now we have ints

  sevt->sub_padding = len%2;
  len = len + (len%2);
  sevt->sub_length += len;
  //  cout << __LINE__ << "  " << __FILE__ << " returning "  << sevt->sub_length << endl;

  return  sevt->sub_length;
}


void daq_device_tspmproto::identify(std::ostream& os) const
{
  
  os  << "Tspmproto Device  Event Type: " << m_eventType << " Subevent id: " << m_subeventid 
      << " IP: " << _ip;
  if ( _broken) os << " ** not functional ** ";
  os << endl;

}

int daq_device_tspmproto::max_length(const int etype) const
{
  if (etype != m_eventType) return 0;
  return  (MAX_PACKETS*1024 + SEVTHEADERLENGTH);
}

int  daq_device_tspmproto::init()
{

  return 0;
  //  return start_tspm();

}

// the rearm() function
int  daq_device_tspmproto::rearm(const int etype)
{
  if (etype != m_eventType) return 0;
  return 0;
}

int daq_device_tspmproto::start_tspm()
{

#define SETPORT 32000
#define READREQPORT 32001

#define RECPORT 32003

  struct sockaddr_in server;

  int i;
  int sockfd;
  socklen_t slen=sizeof(server);

  int retval;
  

  if ((sockfd=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1)
    {
      _broken = 2;
      return 0;
    }


  memset((char *) &server, 0, sizeof(server));
  server.sin_family = AF_INET;
  if (inet_aton(_ip, &server.sin_addr)==0) 
    {
      _broken = 2;
      return 0;

    }


  short reg;
  int val = 0;

  set_value( 3, 1, sockfd, &server);

  return 0;
}


int daq_device_tspmproto::set_value( const short reg, const int value, int s, struct sockaddr_in *server)
{

  typedef struct {
    unsigned int deadbeef;
    unsigned short payload[1+2+1];
  }  requeststructure;
  
  typedef struct {
    short reg;
    short val[23];
  }  returnstructure;
  


  int slen;
  requeststructure request;

  slen = sizeof(*server);
  
  server->sin_port = htons(SETPORT);
  request.deadbeef = htonl(0xdeadbeef);
  request.payload[0] = htons(reg);
  request.payload[1] = htons((value >> 16));
  request.payload[2] = htons((value & 0xffff));
  request.payload[3] = 0xffff;
  
  if (sendto(s, &request, sizeof(request), 0, (sockaddr *) server, slen)==-1)
    {
      perror("sendto 1 " );
      return -1;
    }

  return 0;
}
