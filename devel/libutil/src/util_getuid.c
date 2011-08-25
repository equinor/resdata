uid_t util_get_entry_uid( const char * file ) {
  struct stat buffer;
  stat( file , &buffer);
  return buffer.st_uid;
}



bool util_chmod_if_owner( const char * filename , mode_t new_mode) {
  struct stat buffer;
  uid_t  exec_uid = getuid(); 
  stat( filename , &buffer );
  
  if (exec_uid == buffer.st_uid) {  /* OKAY - the current running uid is also the owner of the file. */
    mode_t current_mode = buffer.st_mode & ( S_IRWXU + S_IRWXG + S_IRWXO );
    if (current_mode != new_mode) {
      chmod( filename , new_mode); /* No error check ... */
      return true;
    }
  }
  
  return false; /* No update performed. */
}





/*
  IFF the current uid is also the owner of the file the current
  function will add the permissions specified in the add_mode variable
  to the file.

  The function simulates the "chmod +???" behaviour of the shell
  command. If the mode of the file is changed the function will return
  true, otherwise it will return false.
*/



bool util_addmode_if_owner( const char * filename , mode_t add_mode) {
  struct stat buffer;
  stat( filename , &buffer );
  
  {
    mode_t current_mode = buffer.st_mode & ( S_IRWXU + S_IRWXG + S_IRWXO );
    mode_t target_mode  = (current_mode | add_mode);

    return util_chmod_if_owner( filename , target_mode );
  }
}



/**
   Implements shell chmod -??? behaviour.
*/
bool util_delmode_if_owner( const char * filename , mode_t del_mode) {
  struct stat buffer;
  stat( filename , &buffer );
  
  {
    mode_t current_mode = buffer.st_mode & ( S_IRWXU + S_IRWXG + S_IRWXO );
    mode_t target_mode  = (current_mode -  (current_mode & del_mode));
    
    return util_chmod_if_owner( filename , target_mode );
  }
}
  
