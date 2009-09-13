#ifndef __ELOGHANDLER_H
#define __ELOGHANDLER_H


#include <string>



class ElogHandler {

public:

  //** Constructors

  ElogHandler (const std::string h, const int p, const std::string name)
    {
      hostname=h;
      port=p;
      logbookname = name;
    }

  virtual ~ElogHandler() {}

  virtual int BegrunLog ( const int run, std::string who, std::string filename);
  virtual int EndrunLog ( const int run, std::string who, const int events);


protected:

  std::string hostname;
  std::string logbookname;
  int port;


};

#endif


