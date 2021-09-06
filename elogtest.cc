
#include <iostream>
#include <eloghandler.h>

int main()
{

  ElogHandler *x = new ElogHandler("localhost", 666, "RCDAQLog");

  x->BegrunLog(104,"mmm", "run_xxx");
  x->EndrunLog(104,"mmm", 24567);

  delete x;
}
