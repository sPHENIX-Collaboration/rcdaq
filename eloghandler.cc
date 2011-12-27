#include <eloghandler.h>
#include <iostream>
#include <stdlib.h>
#include <sstream>

using namespace std;

  ElogHandler::ElogHandler (const std::string h, const int p, const std::string name)
    {
      hostname=h;
      port=p;
      logbookname = name;
    }

int ElogHandler::BegrunLog ( const int run, std::string who, std::string filename)
{
 
  ostringstream command;

  command << "elog -h " <<  hostname <<  " -p " << port << " -l " << logbookname <<
    " -a \"Author=" <<  who << "\"" <<
    " -a \"Subject=Run " << run << " started\" " << 
    "\"Run " << run <<  " started with file " <<  filename << "\"" <<
    " >/dev/null 2<&1" << endl;


  //  cout << command.str() << endl;
  system (command.str().c_str() );
  return 0;
}

int ElogHandler::EndrunLog ( const int run, std::string who, const int events, const double volume, time_t starttime)
{


  ostringstream command;

  command << "elog -h " <<  hostname <<  " -p " << port 
	  << " -l " << logbookname<<    " -a \"Author=" <<  who << "\"" 
	  << " -a \"Subject=Run " << run << " ended\" " 
	  << "\"Run " << run 
	  << " ended with  " <<  events 
	  << " events, size is " << volume << " MB"
	  << " duration " << time(0) - starttime << " s\" "
	  << " >/dev/null 2<&1" << endl;

  //  cout << command.str() << endl;
  system (command.str().c_str() );


  return 0;
}
