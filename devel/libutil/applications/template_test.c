/*
   Copyright (C) 2012  Statoil ASA, Norway. 
    
   The file 'template_test.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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
#include <stdbool.h>
#include <errno.h>

#include <util.h>
#include <template.h>


int main( int argc , char ** argv) {
  template_type * template = template_alloc( "/tmp/latex-Nuk2LY/report1.tex" , true , NULL );
  template_instantiate( template , "/tmp/target.txt" , NULL , true );
  template_free( template );
}
