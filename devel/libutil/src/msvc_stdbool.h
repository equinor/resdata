/*
  When building with MSVC this file is renamed to stdbool.h and copied into the
  list of headers. The situation is as follows:

  - The ERT code makes use of stdbool in many places.  The msvc C

  - compiler does not have a stdbool header, i.e. the #include <stdbool.h>
    statements fail when compiling. However, the compiler still has a valid
    definition of the bool type. Hence all we have to do is to satisfy the
    #include statement with an empty file.
*/

typedef int bool;
#define true  1
#define false 0

#define __func__ "????"
