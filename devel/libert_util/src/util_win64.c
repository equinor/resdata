/*
   Copyright (C) 2013  Statoil ASA, Norway. 
    
   The file 'util_win64.c' is part of ERT - Ensemble based Reservoir Tool. 
    
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

/*
  The util_win64 function contains superthin wrappers for a couple of
  stdlib functions which require different names on Windows 64 bit,
  the file is included into util.c.
*/

long util_ftell(FILE * stream) {
#ifdef _WIN64
	return _ftelli64(stream);
#else
	return ftell(stream);
#endif
} 


int util_fseek(FILE * stream, long offset, int whence) {
#ifdef _WIN64
	return _fseeki64(stream , offset , whence);
#else
  return fseek( stream , offset , whence );
#endif
} 


void util_rewind(FILE * stream) {
#ifdef _WIN64
	_fseeki64(stream , 0L , SEEK_SET);
#else
	rewind( stream );
#endif
}
