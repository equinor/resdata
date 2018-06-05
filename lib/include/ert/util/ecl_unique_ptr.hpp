#ifndef ECL_UNIQUE_PTR
#define ECL_UNIQUE_PTR

#include <memory>

namespace ecl {

    template <typename T , void (*F)(T*)>
    struct deleter
    {
        void operator() (T * arg) const {
            F( arg );
        }
    };

    template <typename T , void (*F)(T*)>
    using ecl_unique_ptr = std::unique_ptr<T, deleter<T,F> >;

}

#endif


