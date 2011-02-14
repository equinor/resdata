/*
   Copyright (C) 2011  Statoil ASA, Norway. 
    
   The file 'test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
   ERT is free software: you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by 
   the Free Software Foundation, either version 3 of the License, or 
   (at your option) any later version. 
    
   ERT is distributed in the hope that it will be useful, but WITHOUT ANY 
   WARRANTY; without even the implied warranty of MERCHANTABILITY or 
   FITNESS FOR A PARTICULAR PURPOSE.   
    
   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html> 
   for more details. 
*/

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




