#ifndef ERT__VECTOR_H
#define ERT__VECTOR_H

#include <stdio.h>
#include <stdbool.h>

#include <ert/util/type_macros.h>
#include <ert/util/perm_vector.h>

#define ERT_DECLARE_LEGACY_VECTOR(_Prefix, _Type)                              \
    typedef struct _Prefix##_vector_struct _Prefix##_vector_type;              \
    typedef _Type (*_Prefix##_ftype)(_Type);                                   \
    int _Prefix##_vector_lookup_bin(const _Prefix##_vector_type *limits,       \
                                    _Type value, int guess);                   \
    int _Prefix##_vector_lookup_bin__(const _Prefix##_vector_type *limits,     \
                                      _Type value, int guess);                 \
    void _Prefix##_vector_inplace_div(                                         \
        _Prefix##_vector_type *vector,                                         \
        const _Prefix##_vector_type *inv_factor);                              \
    void _Prefix##_vector_inplace_mul(_Prefix##_vector_type *vector,           \
                                      const _Prefix##_vector_type *factor);    \
    void _Prefix##_vector_inplace_add(_Prefix##_vector_type *vector,           \
                                      const _Prefix##_vector_type *delta);     \
    void _Prefix##_vector_inplace_sub(_Prefix##_vector_type *vector,           \
                                      const _Prefix##_vector_type *delta);     \
    void _Prefix##_vector_set_read_only(_Prefix##_vector_type *vector,         \
                                        bool read_only);                       \
    bool _Prefix##_vector_get_read_only(const _Prefix##_vector_type *vector);  \
    void _Prefix##_vector_memcpy_data(_Type *target,                           \
                                      const _Prefix##_vector_type *src);       \
    void _Prefix##_vector_memcpy_from_data(_Prefix##_vector_type *target,      \
                                           const _Type *src, int src_size);    \
    void _Prefix##_vector_memcpy(_Prefix##_vector_type *target,                \
                                 const _Prefix##_vector_type *src);            \
    void _Prefix##_vector_memcpy_data_block(                                   \
        _Prefix##_vector_type *target, const _Prefix##_vector_type *src,       \
        int target_offset, int src_offset, int len);                           \
    bool _Prefix##_vector_growable(const _Prefix##_vector_type *vector);       \
    void _Prefix##_vector_select_unique(_Prefix##_vector_type *vector);        \
    _Prefix##_vector_type *_Prefix##_vector_alloc(int init_size, _Type);       \
    _Prefix##_vector_type *_Prefix##_vector_alloc_private_wrapper(             \
        int init_size, _Type default_value, _Type *data, int alloc_size);      \
    _Prefix##_vector_type *_Prefix##_vector_alloc_shared_wrapper(              \
        int init_size, _Type default_value, _Type *data, int alloc_size);      \
    _Prefix##_vector_type *_Prefix##_vector_alloc_strided_copy(                \
        const _Prefix##_vector_type *src, int start, int stop, int stride);    \
    _Prefix##_vector_type *_Prefix##_vector_alloc_copy(                        \
        const _Prefix##_vector_type *src);                                     \
    void _Prefix##_vector_imul(_Prefix##_vector_type *vector, int index,       \
                               _Type factor);                                  \
    void _Prefix##_vector_scale(_Prefix##_vector_type *vector, _Type factor);  \
    void _Prefix##_vector_div(_Prefix##_vector_type *vector, _Type divisor);   \
    _Type _Prefix##_vector_reverse_iget(const _Prefix##_vector_type *vector,   \
                                        int index);                            \
    _Type _Prefix##_vector_iget(const _Prefix##_vector_type *, int);           \
    _Type _Prefix##_vector_safe_iget(const _Prefix##_vector_type *, int);      \
    _Type _Prefix##_vector_get_min(const _Prefix##_vector_type *vector);       \
    _Type _Prefix##_vector_get_max(const _Prefix##_vector_type *vector);       \
    int _Prefix##_vector_get_min_index(const _Prefix##_vector_type *vector,    \
                                       bool reverse);                          \
    int _Prefix##_vector_get_max_index(const _Prefix##_vector_type *vector,    \
                                       bool reverse);                          \
    _Type _Prefix##_vector_iadd(_Prefix##_vector_type *vector, int index,      \
                                _Type delta);                                  \
    void _Prefix##_vector_resize(_Prefix##_vector_type *vector, int new_size,  \
                                 _Type default_value);                         \
    void _Prefix##_vector_iset(_Prefix##_vector_type *, int, _Type);           \
    void _Prefix##_vector_iset_block(_Prefix##_vector_type *vector, int index, \
                                     int block_size, _Type value);             \
    void _Prefix##_vector_idel_block(_Prefix##_vector_type *vector, int index, \
                                     int block_size);                          \
    _Type _Prefix##_vector_idel(_Prefix##_vector_type *vector, int index);     \
    _Type _Prefix##_vector_del_value(_Prefix##_vector_type *vector,            \
                                     _Type del_value);                         \
    void _Prefix##_vector_insert(_Prefix##_vector_type *vector, int index,     \
                                 _Type value);                                 \
    void _Prefix##_vector_append(_Prefix##_vector_type *, _Type);              \
    void _Prefix##_vector_free_container(_Prefix##_vector_type *vector);       \
    void _Prefix##_vector_free(_Prefix##_vector_type *);                       \
    void _Prefix##_vector_free__(void *);                                      \
    void _Prefix##_vector_free_data(_Prefix##_vector_type *);                  \
    void _Prefix##_vector_reset(_Prefix##_vector_type *);                      \
    void _Prefix##_vector_reset__(void *__vector);                             \
    int _Prefix##_vector_size(const _Prefix##_vector_type *);                  \
    void _Prefix##_vector_lshift(_Prefix##_vector_type *vector, int shift);    \
    void _Prefix##_vector_rshift(_Prefix##_vector_type *vector, int shift);    \
    _Type _Prefix##_vector_pop(_Prefix##_vector_type *vector);                 \
    _Type _Prefix##_vector_get_first(const _Prefix##_vector_type *vector);     \
    _Type _Prefix##_vector_get_last(const _Prefix##_vector_type *);            \
    _Type *_Prefix##_vector_get_ptr(const _Prefix##_vector_type *);            \
    _Type *_Prefix##_vector_alloc_data_copy(                                   \
        const _Prefix##_vector_type *vector);                                  \
    const _Type *_Prefix##_vector_get_const_ptr(                               \
        const _Prefix##_vector_type *);                                        \
    bool _Prefix##_vector_init_linear(_Prefix##_vector_type *vector,           \
                                      _Type start_value, _Type end_value,      \
                                      int num_values);                         \
    void _Prefix##_vector_init_range(_Prefix##_vector_type *vector,            \
                                     _Type value1, _Type value2, _Type delta); \
    void _Prefix##_vector_set_many(_Prefix##_vector_type *, int,               \
                                   const _Type *, int);                        \
    void _Prefix##_vector_set_all(_Prefix##_vector_type *vector, _Type value); \
    void _Prefix##_vector_append_many(_Prefix##_vector_type *vector,           \
                                      const _Type *data, int length);          \
    void _Prefix##_vector_append_vector(_Prefix##_vector_type *vector,         \
                                        const _Prefix##_vector_type *other);   \
    void _Prefix##_vector_shrink(_Prefix##_vector_type *);                     \
    _Type _Prefix##_vector_sum(const _Prefix##_vector_type *);                 \
    _Type _Prefix##_vector_get_default(const _Prefix##_vector_type *);         \
    void _Prefix##_vector_set_default(_Prefix##_vector_type *vector,           \
                                      _Type default_value);                    \
    void _Prefix##_vector_append_default(_Prefix##_vector_type *vector,        \
                                         _Type default_value);                 \
    void _Prefix##_vector_iset_default(_Prefix##_vector_type *vector,          \
                                       int index, _Type default_value);        \
    bool _Prefix##_vector_is_sorted(const _Prefix##_vector_type *vector,       \
                                    bool reverse);                             \
    bool _Prefix##_vector_contains(const _Prefix##_vector_type *vector,        \
                                   _Type value);                               \
    bool _Prefix##_vector_contains_sorted(const _Prefix##_vector_type *vector, \
                                          _Type value);                        \
    int _Prefix##_vector_index(const _Prefix##_vector_type *vector,            \
                               _Type value);                                   \
    int _Prefix##_vector_index_sorted(const _Prefix##_vector_type *vector,     \
                                      _Type value);                            \
    void _Prefix##_vector_sort(_Prefix##_vector_type *vector);                 \
    void _Prefix##_vector_rsort(_Prefix##_vector_type *vector);                \
    void _Prefix##_vector_permute(_Prefix##_vector_type *vector,               \
                                  const perm_vector_type *perm);               \
    perm_vector_type *_Prefix##_vector_alloc_sort_perm(                        \
        const _Prefix##_vector_type *vector);                                  \
    perm_vector_type *_Prefix##_vector_alloc_rsort_perm(                       \
        const _Prefix##_vector_type *vector);                                  \
    void _Prefix##_vector_fprintf(const _Prefix##_vector_type *vector,         \
                                  FILE *stream, const char *name,              \
                                  const char *fmt);                            \
    void _Prefix##_vector_fwrite(const _Prefix##_vector_type *vector,          \
                                 FILE *stream);                                \
    _Prefix##_vector_type *_Prefix##_vector_fread_alloc(FILE *stream);         \
    void _Prefix##_vector_fread(_Prefix##_vector_type *vector, FILE *stream);  \
    void _Prefix##_vector_fwrite_data(const _Prefix##_vector_type *vector,     \
                                      FILE *stream);                           \
    void _Prefix##_vector_fread_data(_Prefix##_vector_type *vector, int size,  \
                                     FILE *stream);                            \
    bool _Prefix##_vector_equal(const _Prefix##_vector_type *vector1,          \
                                const _Prefix##_vector_type *vector2);         \
    int _Prefix##_vector_first_equal(const _Prefix##_vector_type *vector1,     \
                                     const _Prefix##_vector_type *vector2,     \
                                     int offset);                              \
    int _Prefix##_vector_first_not_equal(const _Prefix##_vector_type *vector1, \
                                         const _Prefix##_vector_type *vector2, \
                                         int offset);                          \
    void _Prefix##_vector_apply(_Prefix##_vector_type *vector,                 \
                                _Prefix##_ftype *func);                        \
    int _Prefix##_vector_count_equal(const _Prefix##_vector_type *vector,      \
                                     _Type cmp_value);                         \
    int _Prefix##_vector_element_size(const _Prefix##_vector_type *vector);    \
    void _Prefix##_vector_range_fill(_Prefix##_vector_type *vector,            \
                                     _Type limit1, _Type delta, _Type limit2); \
    void _Prefix##_vector_shift(_Prefix##_vector_type *vector, _Type delta);   \
    bool _Prefix##_vector_is_instance(void *__ptr);                            \
    _Prefix##_vector_type *_Prefix##_vector_safe_cast(void *__ptr);

#define ERT_DECLARE_LEGACY_VECTOR2(_Type)                                      \
    ERT_DECLARE_LEGACY_VECTOR(_Type, _Type)

#endif //ERT__VECTOR_H
