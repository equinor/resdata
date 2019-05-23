#ifndef ECL3_F77_H
#define ECL3_F77_H

#include <stddef.h>
#include <stdint.h>

#include <ecl3/common.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
 * As per the gnu fortran manual, the record byte marker is int32. We could
 * support 8-byte markers with either compile-time configuration or a run-time
 * switch
 *
 * A Fortran program writes unformatted data to file in a statemente like:
 *
 *    integer array(100)
 *    write(unit) array
 *
 * it actually writes a head and tail in addition to the actual data. The
 * header and tail is a 4 [1] byte integer, which value is the number of bytes
 * in the immediately following record. I.e. what is actually found on disk
 * after the Fortran code above is:
 *
 *   | 400 | array ...... | 400 |
 *
 * [1] http://gcc.gnu.org/onlinedocs/gfortran/File-format-of-unformatted-sequential-files.html
 */

enum ecl3_format_code {
    /*
     * big-endian format codes
     */
    ECL3_INTE_BE = 'I',
    ECL3_REAL_BE = 'F',
    ECL3_DOUB_BE = 'D',
    ECL3_CHAR_BE = 's',
    ECL3_MESS_BE = 'M',
    ECL3_LOGI_BE = 'L',
    ECL3_C0NN_BE = 'S',
    ECL3_X231_BE = 'X',
    ECL3_BYTE_BE = 'C',

    /*
     * little-endian format codes
     */
    ECL3_INTE_LE = 'i',
    ECL3_REAL_LE = 'f',
    ECL3_DOUB_LE = 'd',
    ECL3_CHAR_LE = 's',
    ECL3_MESS_LE = 'm',
    ECL3_LOGI_LE = 'l',
    ECL3_C0NN_LE = 's',
    ECL3_X231_LE = 'x',
    ECL3_BYTE_LE = 'c',
};

/*
 * Compute the numer of array elements of type fmt
 *
 * Parameters
 * ----------
 * head should be the 4-byte array head, as it was read from disk
 * fmt is an ecl3_format_code matching the type of the array
 * elems is the output number of elements
 *
 * The fmt argument is endian aware, and performs the correct byte swaps if
 * necessary. Use this function to help parse the array head, to decide how
 * many elements to allocate for the array
 *
 * Notes
 * -----
 * If fmt is ECL3_BYTE_*, this function outputs the number of bytes in the
 * array, doing byte-swap if necessary. For any other type, number-of-elems !=
 * size-in-bytes
 */
ECL3_API
int ecl3_elems_in_block(const char* head, int fmt, size_t* elems);


/*
 * Copy elems elements of type fmt from memory area src to memory area dst
 *
 * This is essentially a memcpy that's endian- and type aware, and translates
 * to/from on-disk representation of arrays to CPU-native representations.
 *
 * Examples
 * --------
 * Read an integer array from disk:
 *
 *  char head[4], tail[4];
 *  size_t elems;
 *  int32_t* data;
 *  fread(head, sizeof(head), 1, fp);
 *  ecl3_elems_in_block(head, ECL3_INTE_BE, &elems);
 *  data = malloc(sizeof(int32_t) * elems);
 *  fread(buffer, sizeof(int32_t), elems, fp);
 *  ecl3_get_native(data, buffer, ECL3_INTE_BE, elems);
 *  fread(tail, sizeof(tail), 1, fp);
 */
ECL3_API
int ecl3_get_native(void* dst, const void* src, int fmt, size_t elems);

ECL3_API
int ecl3_put_native(void* dst, const void* src, int fmt, size_t elems);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // ECL3_F77_H
