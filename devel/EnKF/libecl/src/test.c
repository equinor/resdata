#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main (int argc, char **argv) {
  char buffer[64];

  sprintf(buffer,"Joakim");
  printf("Navn: %s",buffer);
  scanf("%s" , &buffer[6] );
  printf("Ny: %s \n",buffer);

  return 0;
}




