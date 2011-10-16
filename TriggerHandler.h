#ifndef __TRIGGERHANDLER_H__
#define __TRIGGERHANDLER_H__


class TriggerHandler {

public:

  TriggerHandler() {};


  virtual ~TriggerHandler() {};

  //  virtual void identify(std::ostream& os = std::cout) const = 0;

  // this is the virtual worker routine
  virtual int wait_for_trigger( const int moreinfo=0) =0;

  // functions that we might need 
  virtual int enable(){return 0;};
  virtual int disable(){return 0;};
  virtual int rearm(){return 0;};


};

#endif
