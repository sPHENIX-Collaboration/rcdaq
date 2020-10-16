

#include <stdlib.h>


int get_value(const char *arg, int *status)
{

  int value;
  value = strtol(arg, 0,0);

  return value;
}

unsigned int get_uvalue(const char *arg, int *status)
{

  unsigned int value;
  value = strtoul(arg, 0,0);

  return value;
}

