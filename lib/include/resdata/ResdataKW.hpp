#ifndef OPM_ERT_RD_KW
#define OPM_ERT_RD_KW

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include <type_traits>

#include <ert/util/ert_unique_ptr.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_type.hpp>
#include <resdata/FortIO.hpp>

namespace ERT {
template <typename> struct rd_type {};

template <> struct rd_type<float> {
    static const rd_type_enum type{RD_FLOAT_TYPE};
};

template <> struct rd_type<double> {
    static const rd_type_enum type{RD_DOUBLE_TYPE};
};

template <> struct rd_type<int> {
    static const rd_type_enum type{RD_INT_TYPE};
};

template <> struct rd_type<bool> {
    static const rd_type_enum type{RD_BOOL_TYPE};
};

template <> struct rd_type<char *> {
    static const rd_type_enum type{RD_CHAR_TYPE};
};

template <> struct rd_type<const char *> {
    static const rd_type_enum type{RD_CHAR_TYPE};
};

/*
      Both std::string and char* are mapped to the eight character string type
      RD_CHAR_TYPE. That implies that the variable length string type
      RD_STRING is invisible from this API.
    */

template <> struct rd_type<std::string> {
    static const rd_type_enum type{RD_CHAR_TYPE};
};

template <> struct rd_type<const std::string> {
    static const rd_type_enum type{RD_CHAR_TYPE};
};

template <typename T> class ResdataKW_ref {
public:
    explicit ResdataKW_ref(rd_kw_type *kw) : m_kw(kw) {
        if (rd_type_get_type(rd_kw_get_data_type(kw)) != rd_type<T>::type)
            throw std::invalid_argument("Type error");
    }

    ResdataKW_ref() noexcept = default;

    const char *name() const { return rd_kw_get_header(this->m_kw); }

    size_t size() const { return size_t(rd_kw_get_size(this->m_kw)); }

    void fwrite(FortIO &fortio) const {
        rd_kw_fwrite(this->m_kw, fortio.get());
    }

    T at(size_t i) const {
        return *static_cast<T *>(rd_kw_iget_ptr(this->m_kw, i));
    }

    T &operator[](size_t i) {
        return *static_cast<T *>(rd_kw_iget_ptr(this->m_kw, i));
    }

    const typename std::remove_pointer<T>::type *data() const {
        using Tp = const typename std::remove_pointer<T>::type *;
        return static_cast<Tp>(rd_kw_get_ptr(this->m_kw));
    }

    rd_kw_type *get() const { return this->m_kw; }

    void resize(size_t new_size) { rd_kw_resize(this->m_kw, new_size); }

protected:
    rd_kw_type *m_kw = nullptr;
};

template <> inline bool ResdataKW_ref<bool>::at(size_t i) const {
    return rd_kw_iget_bool(this->m_kw, i);
}

template <> inline const char *ResdataKW_ref<const char *>::at(size_t i) const {
    return rd_kw_iget_char_ptr(this->m_kw, i);
}

template <> inline std::string ResdataKW_ref<std::string>::at(size_t i) const {
    return rd_kw_iget_char_ptr(this->m_kw, i);
}

/*
  The current implementation of "string" and "bool" storage in the underlying C
  rd_kw structure does not lend itself to easily implement operator[]. We have
  therefore explicitly deleted them here.
*/

template <>
const char *&ResdataKW_ref<const char *>::operator[](size_t i) = delete;

template <> bool &ResdataKW_ref<bool>::operator[](size_t i) = delete;

template <typename T> class ResdataKW : public ResdataKW_ref<T> {
private:
    using base = ResdataKW_ref<T>;

public:
    using ResdataKW_ref<T>::ResdataKW_ref;

    ResdataKW(const ResdataKW &) = delete;
    ResdataKW(ResdataKW &&rhs) : base(rhs.m_kw) { rhs.m_kw = nullptr; }

    ~ResdataKW() {
        if (this->m_kw)
            rd_kw_free(this->m_kw);
    }

    ResdataKW(const std::string &kw, int size_)
        : base(rd_kw_alloc(kw.c_str(), size_,
                           rd_type_create_from_type(rd_type<T>::type))) {}

    ResdataKW(const std::string &kw, const std::vector<T> &data)
        : ResdataKW(kw, data.size()) {
        rd_kw_set_memcpy_data(this->m_kw, data.data());
    }

    template <typename U>
    ResdataKW(const std::string &kw, const std::vector<U> &data)
        : ResdataKW(kw, data.size()) {
        T *target = static_cast<T *>(rd_kw_get_ptr(this->m_kw));

        for (size_t i = 0; i < data.size(); ++i)
            target[i] = T(data[i]);
    }

    std::vector<T> data() const {
        const T *ptr = static_cast<T *>(rd_kw_get_ptr(this->m_kw));
        std::vector<T> vector;
        vector.assign(ptr, ptr + this->size());
        return vector;
    }

    static ResdataKW load(FortIO &fortio) {
        rd_kw_type *c_ptr = rd_kw_fread_alloc(fortio.get());

        if (!c_ptr)
            throw std::invalid_argument("fread kw failed - EOF?");

        return ResdataKW(c_ptr);
    }
};

template <>
inline ResdataKW<const char *>::ResdataKW(const std::string &kw,
                                          const std::vector<const char *> &data)
    : ResdataKW(kw, data.size()) {
    auto *ptr = this->get();
    for (size_t i = 0; i < data.size(); ++i) {
        if (strlen(data[i]) > 8)
            throw std::range_error("Strings must be maximum 8 characters long");
        rd_kw_iset_string8(ptr, i, data[i]);
    }
}

template <>
inline ResdataKW<std::string>::ResdataKW(const std::string &kw,
                                         const std::vector<std::string> &data)
    : ResdataKW(kw, data.size()) {
    auto *ptr = this->get();
    for (size_t i = 0; i < data.size(); ++i) {
        if (data[i].size() > 8)
            throw std::range_error("Strings must be maximum 8 characters long");
        rd_kw_iset_string8(ptr, i, data[i].c_str());
    }
}

template <>
template <>
inline ResdataKW<std::string>::ResdataKW(const std::string &kw,
                                         const std::vector<const char *> &data)
    : ResdataKW(kw, data.size()) {
    auto *ptr = this->get();
    for (size_t i = 0; i < data.size(); ++i) {
        if (strlen(data[i]) > 8)
            throw std::range_error("Strings must be maximum 8 characters long");
        rd_kw_iset_string8(ptr, i, data[i]);
    }
}

template <>
inline ResdataKW<bool>::ResdataKW(const std::string &kw,
                                  const std::vector<bool> &data)
    : ResdataKW(kw, data.size()) {
    for (size_t i = 0; i < data.size(); i++)
        rd_kw_iset_bool(this->m_kw, i, data[i]);
}

template <>
inline std::vector<std::string> ResdataKW<std::string>::data() const {
    std::vector<std::string> strings;
    auto *ptr = this->get();
    for (size_t i = 0; i < this->size(); ++i) {
        std::string s8 = rd_kw_iget_char_ptr(ptr, i);
        s8.erase(s8.find_last_not_of(' ') + 1);
        strings.push_back(s8);
    }
    return strings;
}

/*
  Will write an rd_kw instance to the open Fortio file.
*/
template <typename T>
void write_kw(FortIO &fortio, const std::string &kw,
              const std::vector<T> &data) {
    ResdataKW<T> rd_kw(kw, data);
    rd_kw_fwrite(rd_kw.get(), fortio.get());
}

/*
  Will write an empty rd_kw instance of type 'MESS' to the Fortio file.
*/
inline void write_mess(FortIO &fortio, const std::string &kw) {
    rd_kw_type *rd_kw = rd_kw_alloc(kw.c_str(), 0, RD_MESS);
    rd_kw_fwrite(rd_kw, fortio.get());
}

} // namespace ERT

#endif
