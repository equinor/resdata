#include <cstdio>
#include <cstring>
#include <cstdint>

#include <ecl/fortio.h>
#include <ecl/ecl_kw.h>

#include "ecl/details.hpp"


int ecl_kw_get_header(FILE*fp, const char * opts, char * header, char * data_type, int32_t * nmemb) {
    /*
      The header is stored on file as something resmebling the struct:

      {
          char[8] header;
          int32   nmember;
          char[4] data_type;
      }

      Here it is read as one 16 byte buffer:
    */
    char buffer[16 * sizeof(char)];
    int get = eclfio_get(fp, opts, nullptr, buffer);
    if (get != ECL_OK)
        return get;

    if (header)
        std::memcpy(header, buffer, 8 * sizeof(char));

    if (data_type)
        std::memcpy(data_type, &buffer[12], 4 * sizeof(char));

    if (nmemb) {
        std::memcpy(nmemb, &buffer[8], sizeof nmemb);
        // Need to parse the opts string to determine whether endian flipping
        // should be performed; would like to use the anonymous implementation
        // in fortio
        if (true)
            *nmemb = (std::int32_t) htonl( (std::uint32_t) *nmemb);
    }
    return ECL_OK;
}
