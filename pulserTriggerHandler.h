#ifndef __PULSERTRIGGERHANDLER_H__
#define __PULSERTRIGGERHANDLER_H__

#include <unistd.h>

class pulserTriggerHandler : public TriggerHandler {

public:

  pulserTriggerHandler(const int evttype = 1)
    {
      _evttype = evttype;
      _count = 0;
    }

  ~pulserTriggerHandler() {};

  //  virtual void identify(std::ostream& os = std::cout) const = 0;

  // this is the virtual worker routine
  int wait_for_trigger( const int moreinfo=0)
  {
    //std::cout << "trigger " << _count++ << std::endl; 
    return _evttype;
  }
 protected:

  int _count;
  int _evttype;
};

#endif
