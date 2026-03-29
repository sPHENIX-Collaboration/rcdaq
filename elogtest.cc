
#include <iostream>
#include <eloghandler.h>

int main()
{

  ElogHandler *x = new ElogHandler("10.211.55.2", 666, "Main");

  x->BegrunLog(104,"mmm", "run_xxx");
  x->EndrunLog(104,"mmm", 24567);

  delete x;
}
