#ifndef ECL3_KEYWORD_H
#define ECL3_KEYWORD_H

#include <ecl3/common.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/*
 * `Keywords` in ecl3 is the conceptual structure:
 *
 * struct {
 *     str  name;
 *     tag  type;
 *     int  len;
 *     byte data[];
 * };
 *
 * Or, a more visual example, a tagged column vector:
 *
 * +------------+
 * | 'KEYWORDS' |
 * | 'CHAR'     | HEADER
 * | 5          |
 * +------------+
 * | 'TIME    ' |
 * | 'FOPR    ' |
 * | 'GOPR    ' | BODY
 * | 'GOPR    ' |
 * | 'GOPR    ' |
 * +------------+
 *
 * The header and body are written separately, which means they both come with
 * the Fortran block length metadata, handled by ecl3/f77. Furthermore, large
 * array bodies are split up into 105-element (for strings) or 1000 element
 * chunks.
 *
 * This module provide the functions guide I/O and parse these structures once
 * read from disk
 */

/*
 * Convert from in-file string representation to ecl3_keyword_types value.
 *
 * This function maps the input string, as found in the file, to the
 * corresponding enum value in ecl3_keyword_types. The enum value is a lot more
 * practical to work with in C programs.
 *
 * Returns ECL3_OK if str was any of the values in ecl3_keyword_types, and
 * ECL3_INVALID_ARGS otherwise.
 *
 * Examples
 * --------
 *  int type;
 *  ecl3_keyword_type("INTE", &type);
 *  if (type == ECL3_INTE) puts("type was INTE");
 */
ECL3_API
int ecl3_keyword_type(const char* str, int* type);

/*
 * Get the size, in bytes, of a keyword type
 *
 * Takes a keyword type, as returned by ecl3_keyword_type, and yields the size
 * in bytes. Returns ECL3_OK on success, and ECL3_INVALID_ARGS if the type
 * argument is not an ecl3_keyword_type.
 *
 * If given a valid, but unsupported type, this function returns
 * ECL3_UNSUPPORTED.
 */
ECL3_API
int ecl3_keyword_size(int type, int* size);

/*
 * Get the size of an array header
 *
 * The array header is the record:
 *
 * STRUCTURE /KEYWORD/:
 *      CHARACTER (LEN=8) name
 *      INTEGER           len
 *      CHARACTER (LEN=4) type
 *
 */
ECL3_API
int ecl3_array_header_size();

/*
 * Arrays in the file format are written as two records, a header and a body.
 *
 * The header contains metadata about the following array: its name/keyword,
 * its type, and its length (in elements, not bytes!). This function parses the
 * array header.
 *
 * On disk, an array is typically laid out as such:
 *
 * |head| KEYWORD COUNT TYPE |tail| |head| VALUE1 VALUE2 .. VALUEN |tail|
 *      + ------------------ +           + ----------------------- +
 *      | array header       |           | array body              |
 *
 *
 * where |head| and |tail| are record length markers. This function is unaware
 * of the record markers, and assumes they have been dealt with. That
 * functionality is in ecl3/f77.h
 *
 * This function faithfully outputs what's actually on disk. To obtain a more
 * practical representation for the array type, use the output type from this
 * function as input to ecl3_keyword_type.
 */
ECL3_API
int ecl3_array_header(const void* src, char* keyword, char* type, int* count);

/*
 * The array data types in the manual. In the file format, these are specified
 * as 4-character strings, but it's useful to have a numerical representation
 * for C programs.
 *
 * Encode the type string itself (which occupies four bytes) as an int [1].
 * Users should not care about the numerical value, but it make some internal
 * operations easier. This also means the *length* of each value is contained
 * in the type tag itself.
 *
 * [1] This assumes 4-byte integers and will not work on other platforms.
 */
#define ECL3_MAKE_KWENUM(word) \
    ((int)((word)[0]) << 24 | \
     (int)((word)[1]) << 16 | \
     (int)((word)[2]) << 8  | \
     (int)((word)[3]))

enum ecl3_keyword_types {
    ECL3_INTE = ECL3_MAKE_KWENUM("INTE"),
    ECL3_REAL = ECL3_MAKE_KWENUM("REAL"),
    ECL3_DOUB = ECL3_MAKE_KWENUM("DOUB"),
    ECL3_CHAR = ECL3_MAKE_KWENUM("CHAR"),
    ECL3_MESS = ECL3_MAKE_KWENUM("MESS"),
    ECL3_LOGI = ECL3_MAKE_KWENUM("LOGI"),
    ECL3_X231 = ECL3_MAKE_KWENUM("X231"),

    ECL3_C001 = ECL3_MAKE_KWENUM("C001"),
    ECL3_C002 = ECL3_MAKE_KWENUM("C002"),
    ECL3_C003 = ECL3_MAKE_KWENUM("C003"),
    ECL3_C004 = ECL3_MAKE_KWENUM("C004"),
    ECL3_C005 = ECL3_MAKE_KWENUM("C005"),
    ECL3_C006 = ECL3_MAKE_KWENUM("C006"),
    ECL3_C007 = ECL3_MAKE_KWENUM("C007"),
    ECL3_C008 = ECL3_MAKE_KWENUM("C008"),
    ECL3_C009 = ECL3_MAKE_KWENUM("C009"),
    ECL3_C010 = ECL3_MAKE_KWENUM("C010"),
    ECL3_C011 = ECL3_MAKE_KWENUM("C011"),
    ECL3_C012 = ECL3_MAKE_KWENUM("C012"),
    ECL3_C013 = ECL3_MAKE_KWENUM("C013"),
    ECL3_C014 = ECL3_MAKE_KWENUM("C014"),
    ECL3_C015 = ECL3_MAKE_KWENUM("C015"),
    ECL3_C016 = ECL3_MAKE_KWENUM("C016"),
    ECL3_C017 = ECL3_MAKE_KWENUM("C017"),
    ECL3_C018 = ECL3_MAKE_KWENUM("C018"),
    ECL3_C019 = ECL3_MAKE_KWENUM("C019"),
    ECL3_C020 = ECL3_MAKE_KWENUM("C020"),
    ECL3_C021 = ECL3_MAKE_KWENUM("C021"),
    ECL3_C022 = ECL3_MAKE_KWENUM("C022"),
    ECL3_C023 = ECL3_MAKE_KWENUM("C023"),
    ECL3_C024 = ECL3_MAKE_KWENUM("C024"),
    ECL3_C025 = ECL3_MAKE_KWENUM("C025"),
    ECL3_C026 = ECL3_MAKE_KWENUM("C026"),
    ECL3_C027 = ECL3_MAKE_KWENUM("C027"),
    ECL3_C028 = ECL3_MAKE_KWENUM("C028"),
    ECL3_C029 = ECL3_MAKE_KWENUM("C029"),
    ECL3_C030 = ECL3_MAKE_KWENUM("C030"),
    ECL3_C031 = ECL3_MAKE_KWENUM("C031"),
    ECL3_C032 = ECL3_MAKE_KWENUM("C032"),
    ECL3_C033 = ECL3_MAKE_KWENUM("C033"),
    ECL3_C034 = ECL3_MAKE_KWENUM("C034"),
    ECL3_C035 = ECL3_MAKE_KWENUM("C035"),
    ECL3_C036 = ECL3_MAKE_KWENUM("C036"),
    ECL3_C037 = ECL3_MAKE_KWENUM("C037"),
    ECL3_C038 = ECL3_MAKE_KWENUM("C038"),
    ECL3_C039 = ECL3_MAKE_KWENUM("C039"),
    ECL3_C040 = ECL3_MAKE_KWENUM("C040"),
    ECL3_C041 = ECL3_MAKE_KWENUM("C041"),
    ECL3_C042 = ECL3_MAKE_KWENUM("C042"),
    ECL3_C043 = ECL3_MAKE_KWENUM("C043"),
    ECL3_C044 = ECL3_MAKE_KWENUM("C044"),
    ECL3_C045 = ECL3_MAKE_KWENUM("C045"),
    ECL3_C046 = ECL3_MAKE_KWENUM("C046"),
    ECL3_C047 = ECL3_MAKE_KWENUM("C047"),
    ECL3_C048 = ECL3_MAKE_KWENUM("C048"),
    ECL3_C049 = ECL3_MAKE_KWENUM("C049"),
    ECL3_C050 = ECL3_MAKE_KWENUM("C050"),
    ECL3_C051 = ECL3_MAKE_KWENUM("C051"),
    ECL3_C052 = ECL3_MAKE_KWENUM("C052"),
    ECL3_C053 = ECL3_MAKE_KWENUM("C053"),
    ECL3_C054 = ECL3_MAKE_KWENUM("C054"),
    ECL3_C055 = ECL3_MAKE_KWENUM("C055"),
    ECL3_C056 = ECL3_MAKE_KWENUM("C056"),
    ECL3_C057 = ECL3_MAKE_KWENUM("C057"),
    ECL3_C058 = ECL3_MAKE_KWENUM("C058"),
    ECL3_C059 = ECL3_MAKE_KWENUM("C059"),
    ECL3_C060 = ECL3_MAKE_KWENUM("C060"),
    ECL3_C061 = ECL3_MAKE_KWENUM("C061"),
    ECL3_C062 = ECL3_MAKE_KWENUM("C062"),
    ECL3_C063 = ECL3_MAKE_KWENUM("C063"),
    ECL3_C064 = ECL3_MAKE_KWENUM("C064"),
    ECL3_C065 = ECL3_MAKE_KWENUM("C065"),
    ECL3_C066 = ECL3_MAKE_KWENUM("C066"),
    ECL3_C067 = ECL3_MAKE_KWENUM("C067"),
    ECL3_C068 = ECL3_MAKE_KWENUM("C068"),
    ECL3_C069 = ECL3_MAKE_KWENUM("C069"),
    ECL3_C070 = ECL3_MAKE_KWENUM("C070"),
    ECL3_C071 = ECL3_MAKE_KWENUM("C071"),
    ECL3_C072 = ECL3_MAKE_KWENUM("C072"),
    ECL3_C073 = ECL3_MAKE_KWENUM("C073"),
    ECL3_C074 = ECL3_MAKE_KWENUM("C074"),
    ECL3_C075 = ECL3_MAKE_KWENUM("C075"),
    ECL3_C076 = ECL3_MAKE_KWENUM("C076"),
    ECL3_C077 = ECL3_MAKE_KWENUM("C077"),
    ECL3_C078 = ECL3_MAKE_KWENUM("C078"),
    ECL3_C079 = ECL3_MAKE_KWENUM("C079"),
    ECL3_C080 = ECL3_MAKE_KWENUM("C080"),
    ECL3_C081 = ECL3_MAKE_KWENUM("C081"),
    ECL3_C082 = ECL3_MAKE_KWENUM("C082"),
    ECL3_C083 = ECL3_MAKE_KWENUM("C083"),
    ECL3_C084 = ECL3_MAKE_KWENUM("C084"),
    ECL3_C085 = ECL3_MAKE_KWENUM("C085"),
    ECL3_C086 = ECL3_MAKE_KWENUM("C086"),
    ECL3_C087 = ECL3_MAKE_KWENUM("C087"),
    ECL3_C088 = ECL3_MAKE_KWENUM("C088"),
    ECL3_C089 = ECL3_MAKE_KWENUM("C089"),
    ECL3_C090 = ECL3_MAKE_KWENUM("C090"),
    ECL3_C091 = ECL3_MAKE_KWENUM("C091"),
    ECL3_C092 = ECL3_MAKE_KWENUM("C092"),
    ECL3_C093 = ECL3_MAKE_KWENUM("C093"),
    ECL3_C094 = ECL3_MAKE_KWENUM("C094"),
    ECL3_C095 = ECL3_MAKE_KWENUM("C095"),
    ECL3_C096 = ECL3_MAKE_KWENUM("C096"),
    ECL3_C097 = ECL3_MAKE_KWENUM("C097"),
    ECL3_C098 = ECL3_MAKE_KWENUM("C098"),
    ECL3_C099 = ECL3_MAKE_KWENUM("C099"),
};

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // ECL3_KEYWORD_H
