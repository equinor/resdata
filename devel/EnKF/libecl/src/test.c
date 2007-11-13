#include <stdlib.h>
#include <stdio.h>
#include <time.h>



int main (int argc, char **argv) {
  char buffer[64];
  char *end_ptr;
  double power;
  double arg   = strtod("1.0000E+01" , &end_ptr);
  if (end_ptr[0] == 'D')
    power = strtod(end_ptr + 1 , NULL);
  


  printf("Hei %lg  %lg \n",arg, power);

  return 0;
}




