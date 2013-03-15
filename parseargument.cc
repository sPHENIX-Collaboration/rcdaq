

#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int get_value(const char *arg, int *status)
{

  istringstream sarg ( arg);
  int value;
  string s = arg;
  
  int x =  s.find("0x",0);


  if (x == -1 )
    {
      sarg >> value;
    }
  else
    {
      sarg >> hex >> value;
    }

  return value;
}

unsigned int get_uvalue(const char *arg, int *status)
{

  istringstream sarg ( arg);
  unsigned int value;
  string s = arg;
  
  int x =  s.find("0x",0);


  if (x == -1 )
    {
      sarg >> value;
    }
  else
    {
      sarg >> hex >> value;
    }

  return value;
}

