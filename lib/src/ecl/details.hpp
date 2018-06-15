/*
  Internal header ecl3 library.
*/

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif HAVE_ARPA_INET_H
#include <arpa/inet.h>
#elif HAVE_WINSOCK2_H
#include <winsock2.h>
#endif

