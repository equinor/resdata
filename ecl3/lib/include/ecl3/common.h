#ifndef ECL3_COMMON_H
#define ECL3_COMMON_H

/*
 * symbol visibility (export and import)
 *
 * The ECL3_EXPORT symbol is set by cmake when building shared libraries.
 *
 * If linking to a shared build on Windows the ECL3_SHARED symbol must be
 * defined too
 */

/* make sure the symbol always exists */
#if defined(ECL3_API)
    #error ECL3_API is already defined
#endif

#define ECL3_API

#if defined(ECL3_EXPORT)
    #if defined(_MSC_VER)
        #undef ECL3_API
        #define ECL3_API __declspec(dllexport)
    #endif

    /*
     * nothing in particular is needed for symbols to be visible on non-MSVC
     * compilers
     */
#endif

#if !defined(ECL3_EXPORT)
    #if defined(_MSC_VER) && defined(ECL3_SHARED)
        /*
         * TODO: maybe this could be addressed by checking #ifdef _DLL, rather
         * than relying on ECL3_SHARED being set.
         */
        #undef ECL3_API
        #define ECL3_API __declspec(dllimport)
    #endif

    /* assume gcc style exports if gcc or clang */
    #if defined(__GNUC__) || defined(__clang__)
        #undef ECL3_API
        #define ECL3_API __attribute__ ((visibility ("default")))
    #endif
#endif

enum ecl3_errno {
    ECL3_OK = 0,
    ECL3_INVALID_ARGS,
};

#endif //ECL3_COMMON_H
