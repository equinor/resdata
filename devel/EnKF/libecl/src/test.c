#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main (int argc, char **argv) {
  
  struct tm ts;
  time_t t;
  
  time(&t);
  localtime_r(&t , &ts);
  
  printf("Now: %02d:%02d \n",ts.tm_hour , ts.tm_min);

  return 0;
}




