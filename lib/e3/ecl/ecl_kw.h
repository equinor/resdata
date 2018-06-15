#ifndef ECL_KW_H
#define ECL_KW_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <ecl/fortio.h>

/*
 * This function will read the header for one vector of data. The header is a
 * record consisting of a 8 character string, like 'PRESSURE' and 'SWAT ', an
 * integer denoting the number of elements and a four character string denoting
 * the type of the data.
 *
 * The variables header and and data_type must be of sufficient size when
 * calling the function. The string values returned in header and data_type are
 * raw, i.e. they are not \0 terminated and the space padding is not removed.
 *
 * All three pointers header, data_type and nmemb can be nullptr, in which case
 * that particular value is not set. The function will return ECL_OK on success
 * and one of the errors from enum ecl_errno if the read fails; if the read
 * fails the FILE should be rewinded to the position prior to the call.
 *
 * The opts string is documented in the fortio.h header. Observe that the header
 * is read as a set of bytes, i.e. the record data type indicator must be 'c'
 * for character.
*/


int eclkw_get_header(FILE *, const char * opts, char * header, char * data_type, int32_t * nmemb);


#ifdef __cplusplus
}
#endif
#endif
