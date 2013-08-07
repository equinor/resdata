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

off_t util_ftell(FILE * stream) {
#ifdef WINDOWS_LFS_SUPPORT
  return _ftelli64(stream);
#else
  return ftello(stream);
#endif
} 


int util_fseek(FILE * stream, off_t offset, int whence) {
#ifdef WINDOWS_LFS_SUPPORT
  return _fseeki64(stream , offset , whence);
#else
  return fseeko( stream , offset , whence );
#endif
} 



void util_rewind(FILE * stream) {
#ifdef WINDOWS_LFS_SUPPORT
  _fseeki64(stream , 0L , SEEK_SET);
#else
  rewind( stream );
#endif
}





int util_stat(const char * filename , stat_type * stat_info) {
#ifdef WINDOWS_LFS_SUPPORT
  return _stat64(filename , stat_info);
#else
  return stat(filename , stat_info);
#endif
}


int util_fstat(int fileno, stat_type * stat_info) {
#ifdef WINDOWS_LFS_SUPPORT
  return _fstat64(fileno , stat_info);
#else
  return fstat(fileno , stat_info);
#endif
}
