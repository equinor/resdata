/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'vector_template.c' is part of ERT - Ensemble based Reservoir Tool.

   ERT is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   ERT is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.

   See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
   for more details.
*/

/**
   This file implements a very simple vector functionality. The vector
   is charactereized by the following:

    o The content is only fundamental scalar types, like double, int
      or bool. If you want a vector of compositie types use the
      vector_type implementation.

    o The vector will grow as needed. You can safely set any index in
      the vector, and it will automatically grow. However - this might
      lead to 'holes' - these will be filled with the default value.

    o The implementation is in terms of a @TYPE@, the following sed
      one-liners will then produce proper source and header files:

        sed -e's/@TYPE@/int/g' vector_template.c > int_vector.c
        sed -e's/@TYPE@/int/g' vector_template.h > int_vector.h


   Illustration of the interplay of size, alloc_size and default value.


   1. int_vector_type * vector = int_vector_alloc(2 , 77);
   2. int_vector_append(vector , 1);
   3. int_vector_append(vector , 0);
   4. int_vector_append(vector , 12);
   5. int_vector_iset(vector , 6 , 78);
   6. int_vector_set_default( vector , 99 );

   ------------------------------------

    �-----�-----�
1.  | 77  | 77  |                                                                                     size = 0, alloc_size = 2
    �-----�-----�

    �-----�-----�
2.  |  1  | 77  |                                                                                     size = 1, alloc_size = 2
    �-----�-----�

    �-----�-----�
3.  |  1  |  0  |                                                                                     size = 2, alloc_size = 2
    �-----�-----�

    �-----�-----�-----�-----�
4.  |  1  |  0  |  12 | 77  |                                                                         size = 3, alloc_size = 4
    �-----�-----�-----�-----�

    �-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�
5.  |  1  |  0  |  12 |  77 |  77 | 77  |  78 | 77  | 77  |  77 | 77  | 77  | 77  | 77  | 77  | 77  | size = 7, alloc_size = 12, default = 77
    �-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�

    �-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�
6.  |  1  |  0  |  12 |  77 |  77 | 77  |  78 | 99  | 99  |  99 | 99  | 99  | 99  | 99  | 99  | 99  | size = 7, alloc_size = 12, default = 99
    �-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�-----�

       0     1      2    3     4     5     6     7     8     9    10    11    12    13    14   15


    1. In point 4. above - if you ask the vector for it's size you
       will get 3, and int_vector_iget(vector, 3) will fail because
       that is beyound the end of the vector.

    2. The size of the vector is the index (+1) of the last validly
       set element in the vector.

    3. In point 5 above we have grown the vector quite a lot to be
       able to write in index 6, as a results there are now many slots
       in the vector which contain the default value - i.e. 77 in this
       case.

    4. In point 6 we change the default value 77 -> 99, then all the
       77 values from position 7 and onwards are changed to 99; the 77
       values in positions 3,4 & 5 are not touched.


*/

#include <cstring>
#include <cstdbool>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <functional>
#include <vector>

#include <ert/util/type_macros.hpp>
#include <ert/util/util.h>
#include <ert/util/perm_vector.hpp>

/* strict alias version of time_t as it's often aliased to long */
struct strict_time {
    time_t value{};

    strict_time() = default;
    strict_time(time_t value) : value(value) {}
    strict_time(const strict_time &) = default;

    strict_time &operator=(const strict_time &) = default;
    strict_time &operator+=(const strict_time &other) {
        this->value += other.value;
        return *this;
    }
    strict_time &operator-=(const strict_time &other) {
        this->value -= other.value;
        return *this;
    }
    strict_time &operator*=(const strict_time &other) {
        this->value *= other.value;
        return *this;
    }
    strict_time &operator/=(const strict_time &other) {
        this->value /= other.value;
        return *this;
    }
    operator time_t() const { return value; }
};

template <typename T> struct ecl_type_info;
template <> struct ecl_type_info<int> { static constexpr int id = 'i'; };
template <> struct ecl_type_info<long> { static constexpr int id = 'l'; };
template <> struct ecl_type_info<float> { static constexpr int id = 'f'; };
template <> struct ecl_type_info<double> { static constexpr int id = 'd'; };
template <> struct ecl_type_info<bool> { static constexpr int id = 'b'; };
template <> struct ecl_type_info<strict_time> {
    static constexpr int id = 't';
};
template <> struct ecl_type_info<size_t> { static constexpr int id = 's'; };

template <typename T> constexpr int ecl_type_id = ecl_type_info<T>::id;

template <typename T> struct vector {
    using ftype = T(T);
    int __type_id{ecl_type_id<T>};
    int alloc_size{}; /* The allocated size of data. */
    int size{}; /* The index of the last valid - i.e. actively set - element in the vector. */
    T default_value{}; /* The data vector is initialized with this value. */
    T *data{};         /* The actual data. */
    bool data_owner{}; /* Is the vector owner of the the actual storage data?
                             If this is false the vector can not be resized. */
    bool read_only{};

public:
    static vector *safe_cast(void *__arg);
    static bool is_instance(const void *__arg);

    vector(int init_size, T default_value, void *data = nullptr,
           int alloc_size = 0, bool data_owner = true);
    vector(const vector<T> &other);

    void resize(int new_size, T default_value);
    void memcpy_data_block(const vector<T> &src, int target_offset,
                           int src_offset, int len);
    void memcpy_from_data(const void *src, int src_size);
    void memcpy_data(void *target) const;
    void memcpy(const vector<T> &src);
    void set_default(T default_value);
    void append_default(T default_value);
    void iset_default(int index, T default_value);
    T iget(int index) const;
    T safe_iget(int index) const;
    T reverse_iget(int index) const;
    T get_first() const;
    T get_last() const;
    void imul(int index, T factor);
    void iset(int index, T value);
    void iset_block(int index, int block_size, T value);
    T iadd(int index, T value);
    void idel_block(int index, int block_size);
    void insert(int index, T value);
    void append(T value);
    void reset();
    T idel(int index);
    void rshift(int shift);
    void lshift(int shift);
    void *get_ptr();
    const void *get_const_ptr() const;
    T pop();
    T del_value(T del_value);
    void scale(T factor);
    void div(T divisor);
    void shift(T delta);
    void inplace_add(const vector &delta);
    void inplace_sub(const vector &delta);
    void inplace_mul(const vector &factor);
    void inplace_div(const vector &inv_factor);
    void set_many(int index, const void *data, int length);
    void set_all(T value);
    void init_range(T value1, T value2, T delta);
    bool init_linear(T start_value, T end_value, int num_values);
    void append_many(const void *data, int length);
    void append_vector(const vector<T> &other);
    void shrink();
    int get_max_index(bool reverse) const;
    T get_max() const;
    int get_min_index(bool reverse) const;
    T get_min() const;
    T sum() const;
    int index(T value) const;
    bool contains(T value) const;
    bool contains_sorted(T value) const;
    int index_sorted(T value) const;
    void select_unique();
    void sort();
    void rsort();
    void permute(const perm_vector_type *perm);
    bool is_sorted(bool reverse) const;
    int lookup_bin(T value, int guess) const;
    int lookup_bin__(T value, int guess) const;
    void fprintf(FILE *stream, const char *name, const char *fmt) const;
    void fread_data(int size, FILE *stream);
    void fwrite_data(FILE *stream) const;
    void fwrite(FILE *stream) const;
    void fread(FILE *stream);
    bool equal(const vector &other) const;
    int first_equal(const vector &other, int offset) const;
    int first_not_equal(const vector &other, int offset) const;
    void apply(ftype *func);
    int count_equal(T cmp_value) const;
    void range_fill(T limit1, T delta, T limit2);
    perm_vector_type *alloc_sort_perm(bool reverse) const;

    void realloc_data__(int new_alloc_size, T default_value);
    void memmove(int offset, int shift);
    void assert_index(int index) const;
    void assert_writable();
};

template <typename T> vector<T> *vector<T>::safe_cast(void *__arg) {
    if (__arg == NULL) {
        util_abort("%s: runtime cast failed - tried to dereference NULL\n",
                   __func__);
        return NULL;
    }
    auto arg = reinterpret_cast<vector<T> *>(__arg);
    if (arg->__type_id == ecl_type_id<T>)
        return arg;
    else {
        util_abort("%s: runtime cast failed: Got ID:%d  Expected ID:%d \n",
                   __func__, arg->__type_id, ecl_type_id<T>);
        return NULL;
    }
}

template <typename T> bool vector<T>::is_instance(const void *__arg) {
    if (__arg == NULL)
        return false;
    auto arg = reinterpret_cast<const vector<T> *>(__arg);
    return arg->__type_id == ecl_type_id<T>;
}

/**
   This datatype is used when allocating a permutation list
   corresponding to a sorted a xxx_vector instance. This permutation
   list can then be used to reorder several xxx_vector instances.
*/
template <typename T>
void vector<T>::realloc_data__(int new_alloc_size, T default_value) {
    if (new_alloc_size != this->alloc_size) {
        if (this->data_owner) {
            if (new_alloc_size > 0) {
                int i;
                this->data = (T *)util_realloc(
                    this->data, new_alloc_size * sizeof *this->data);
                for (i = this->alloc_size; i < new_alloc_size; i++)
                    this->data[i] = default_value;
            } else {
                if (this->alloc_size > 0) {
                    free(this->data);
                    this->data = NULL;
                }
            }
            this->alloc_size = new_alloc_size;
        } else
            util_abort("%s: tried to change the storage are for a shared data "
                       "segment \n",
                       __func__);
    }
}

template <typename T> void vector<T>::memmove(int offset, int shift) {
    if ((shift + offset) < 0)
        util_abort("%s: offset:%d  left_shift:%d - invalid \n", __func__,
                   offset, -shift);

    if ((shift + this->size > this->alloc_size))
        this->realloc_data__(
            util_int_min(2 * this->alloc_size, shift + this->size),
            this->default_value);

    {
        size_t move_size = (this->size - offset) * sizeof(T);
        T *target = &this->data[offset + shift];
        const T *src = &this->data[offset];
        std::memmove(target, src, move_size);

        this->size += shift;
    }
}

template <typename T> void vector<T>::assert_index(int index) const {
    if ((index < 0) || (index >= this->size))
        util_abort("%s: index:%d invalid. Valid interval: [0,%d>.\n", __func__,
                   index, this->size);
}

template <typename T>
vector<T>::vector(int init_size, T default_value, void *data, int alloc_size,
                  bool data_owner) {
    this->default_value = default_value;

    /**
     Not all combinations of (data, alloc_size, data_owner) are valid:

     1. Creating a new vector instance with fresh storage allocation
        from @TYPE@_vector_alloc():

          data       == NULL
          alloc_size == 0
          data_owner == true


     2. Creating a shared wrapper from the @TYPE@_vector_alloc_shared_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == false


     3. Creating a private wrapper which steals the input data from
        @TYPE@_vector_alloc_private_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == true

  */

    if (data == NULL) { /* Case 1: */
        this->data = NULL;
        this->data_owner = true; /* The input values alloc_size and */
        this->alloc_size = 0;    /* data_owner are not even consulted. */
    } else {                     /* Case 2 & 3 */
        this->data = static_cast<T *>(data);
        this->data_owner = data_owner;
        this->alloc_size = alloc_size;
    }

    if (init_size > 0)
        this->resize(
            init_size,
            default_value); /* Filling up the init size elements with the default value */
}

/**
   new_size < current_size: The trailing elements will be lost

   new_size > current_size: The vector will grow by adding default elements at the end.
*/

template <typename T> void vector<T>::resize(int new_size, T default_value) {
    if (new_size > this->size) {
        if (new_size > this->alloc_size) {
            for (int i = this->size; i < this->alloc_size; i++)
                this->data[i] = default_value;
            this->realloc_data__(2 * new_size, default_value);
        } else
            for (int i = this->size; i < new_size; i++)
                this->data[i] = default_value;
    }
    this->size = new_size;
}

template <typename T> void vector<T>::assert_writable() {
    if (this->read_only)
        util_abort("%s: Sorry - tried to modify a read_only vector instance.\n",
                   __func__);
}

/**
   This function will allocate a shared wrapper around the input
   pointer data. The vector implementation will work transparently
   with the input data, but the data will not be reallocated, and also
   not freed when exiting, this implies that it is safe (memory wise)
   to work with the memory location pointed to by data from somewhere
   else as well.

   Vector wrappers allocated this way can NOT grow, and will fail HARD
   if a resize beyond alloc_size is attempted - check with
   @TYPE@_vector_growable()
*/

// template <typename T>
// vector<T> *vector<T>::alloc_shared_wrapper(int init_size, T default_value,
//                                            T *data, int alloc_size) {
//     return vector<T>::alloc__(init_size, default_value, data, alloc_size,
//                               false);
// }

/**
   This function will allocate a vector wrapper around the input
   pointer data. The input data will be hijacked by the vector
   implementation, and the vector will continue like a 100% normal
   vector instance, that includes the capability to resize the data
   area, and the data are will also be freed when the vector is freed.

   Observe that it is (in general) NOT safe to continue to use the
   data pointer from an external scope.
*/

// template <typename T>
// vector<T> *vector<T>::alloc_private_wrapper(int init_size, T default_value,
//                                             T *data, int alloc_size) {
//     return vector<T>::alloc__(init_size, default_value, data, alloc_size,
//                               false);
// }

/**
   This function will copy a block starting at index @src_offset in
   the src vector to a block starting at @target_offset in the target
   vector. The target vector will be resized and initialized with
   default values as required.

   If len goes beyond the length of the src vector the function will
   fail hard.
*/
template <typename T>
void vector<T>::memcpy_data_block(const vector<T> &src, int target_offset,
                                  int src_offset, int len) {
    if (src_offset + len > src.size)
        util_abort("%s: offset:%d  blocksize:%d  vector_size:%d - invalid \n",
                   __func__, src_offset, len, src.size);

    /* Force a resize + default initialisation of the target. */
    if (this->alloc_size < (target_offset + len))
        this->iset(target_offset + len - 1, this->default_value);

    /* Copy the content. */
    std::memcpy(&this->data[target_offset], &src.data[src_offset],
                len * sizeof *src.data);

    /* Update size of target. */
    if (this->size < (target_offset + len))
        this->size = target_offset + len;
}

template <typename T>
void vector<T>::memcpy_from_data(const void *src, int src_size) {
    this->reset();
    this->resize(src_size, 0);
    std::memcpy(this->data, src, src_size * sizeof *this->data);
}

template <typename T> void vector<T>::memcpy_data(void *target) const {
    std::memcpy(target, this->data, this->size * sizeof *this->data);
}

/**
   This function will copy all the content (both header and data) from
   the src vector to the target vector. If the the current allocation
   size of the target vector is sufficiently large, it will not be
   touched.

   Observe that also the default value will be copied.
*/
template <typename T> void vector<T>::memcpy(const vector<T> &src) {
    this->reset();
    this->default_value = src.default_value;

    this->memcpy_data_block(src, 0, 0, src.size);
}

template <typename T>
vector<T>::vector(const vector<T> &other)
    : vector(other.size, other.default_value) {
    this->realloc_data__(other.alloc_size, other.default_value);
    this->size = other.size;
    std::memcpy(this->data, other.data, other.alloc_size * sizeof *other.data);
}

/**
   This will set the default value. This implies that everything
   following the current length of the vector will be set to the new
   default value, whereas values not explicitly set in the interior of
   the vector will retain the old default value.
*/

template <typename T> void vector<T>::set_default(T default_value) {
    this->assert_writable();

    this->default_value = default_value;
    for (int i = this->size; i < this->alloc_size; i++)
        this->data[i] = default_value;
}

/**
   This function will append the value @default_value to the vector,
   and then subsequently set this value as the new default.
*/

template <typename T> void vector<T>::append_default(T default_value) {
    this->append(default_value);
    this->set_default(default_value);
}

/**
   This function will iset the value @default_value into the vector,
   and then subsequently set this value as the new default.

   1.    V = [1 , 2 , 3 , 4 ], default = 77


   2.  _iset_default(v , 10 , 66)

         V = [1 ,2 , 3 ,  4 , 77, 77, 77, 77, 77, 77, 66]

       I.e. before setting the 66 value all values up to @index are
       filled with the current default.

   3. If @index is inside the current data region, there will be no
      effect of the current default, i.e. _iset_default(v , 2 , 66)
      will just give:

         V = [1 ,2 , 66 ,  4 ]

      and 66 as the new default value.
*/

template <typename T> void vector<T>::iset_default(int index, T default_value) {
    this->iset(index, default_value);
    this->set_default(default_value);
}

template <typename T> T vector<T>::iget(int index) const {
    this->assert_index(index);
    return this->data[index];
}

/* Will start counting from the reverse end, as negative indexing in python:

   vector_reverse_iget( v , -1 ) => The last element
   vector_reverse_iget( v , -2 ) => The second to last element

*/
template <typename T> T vector<T>::reverse_iget(int index) const {
    return this->iget(index + this->size);
}

/**
   This might very well operate on a default value.
*/
template <typename T> void vector<T>::imul(int index, T factor) {
    this->assert_index(index);
    this->data[index] *= factor;
}

/* Vector * scalar */
template <typename T> void vector<T>::scale(T factor) {
    for (int i = 0; i < this->size; i++)
        this->data[i] *= factor;
}

/* Vector / scalar; seperate _div function to ensure correct integer division. */
template <typename T> void vector<T>::div(T divisor) {
    for (int i = 0; i < this->size; i++)
        this->data[i] /= divisor;
}

/* vector + scalar */
template <typename T> void vector<T>::shift(T delta) {
    for (int i = 0; i < this->size; i++)
        this->data[i] += delta;
}

/* vector + vector */
template <typename T> void vector<T>::inplace_add(const vector<T> &delta) {
    if (this->size != delta.size)
        util_abort("%s: combining vectors with different size: %d and %d \n",
                   __func__, this->size, delta.size);
    for (int i = 0; i < this->size; i++)
        this->data[i] += delta.data[i];
}

/* vector - vector */
template <typename T> void vector<T>::inplace_sub(const vector<T> &delta) {
    if (this->size != delta.size)
        util_abort("%s: combining vectors with different size: %d and %d \n",
                   __func__, this->size, delta.size);
    for (int i = 0; i < this->size; i++)
        this->data[i] -= delta.data[i];
}

/* vector * vector (elementwise) */
template <typename T> void vector<T>::inplace_mul(const vector<T> &factor) {
    if (this->size != factor.size)
        util_abort("%s: combining vectors with different size: %d and %d \n",
                   __func__, this->size, factor.size);
    for (int i = 0; i < this->size; i++)
        this->data[i] *= factor.data[i];
}

/* vector / vector (elementwise) */
template <typename T> void vector<T>::inplace_div(const vector<T> &inv_factor) {
    if (this->size != inv_factor.size)
        util_abort("%s: combining vectors with different size: %d and %d \n",
                   __func__, this->size, inv_factor.size);
    for (int i = 0; i < this->size; i++)
        this->data[i] /= inv_factor.data[i];
}

/* Will return default value if index > size. Will fail HARD on negative indices (not that safe) ....*/

template <typename T> T vector<T>::safe_iget(int index) const {
    if (index >= this->size)
        return this->default_value;
    else {
        if (index >= 0)
            return this->data[index];
        else {
            util_abort(
                "%s: index:%d is invalid - only accepts positive indices.\n",
                __func__, index);
            return -1;
        }
    }
}

/** Will abort is size == 0 */
template <typename T> T vector<T>::get_last() const {
    return this->iget(this->size - 1);
}

/** Will abort is size == 0 */
template <typename T> T vector<T>::get_first() const { return this->iget(0); }

/**
   Observe that this function will grow the vector if necessary. If
   index > size - i.e. leaving holes in the vector, these are
   explicitly set to the default value. If a reallocation is needed it
   is done in the realloc routine, otherwise it is done here.
*/

template <typename T> void vector<T>::iset(int index, T value) {
    this->assert_writable();

    if (index < 0)
        util_abort("%s: Sorry - can NOT set negative indices. called with "
                   "index:%d \n",
                   __func__, index);
    if (this->alloc_size <= index)
        this->realloc_data__(
            2 * (index + 1),
            this->default_value); /* Must have ( + 1) here to ensure we are not doing 2*0 */
    this->data[index] = value;
    if (index >= this->size) {
        for (int i = this->size; i < index; i++)
            this->data[i] = this->default_value;
        this->size = index + 1;
    }
}

/*
  The block_size can be negative, in which case the loop will walk to
  the left in the vector.
*/

template <typename T>
void vector<T>::iset_block(int index, int block_size, T value) {
    int sign = (block_size > 0) ? 1 : -1;
    for (int c = 0; c < abs(block_size); c++)
        this->iset(index + c * sign, value);
}

/**
   This function invokes _iset - i.e. growing the vector if needed. If
   the index is not currently set, the default value will be used.
*/

template <typename T> T vector<T>::iadd(int index, T delta) {
    T new_value = this->safe_iget(index) + delta;
    this->iset(index, new_value);
    return new_value;
}

/**
   Will remove a block of length @block_size elements, starting at
   @index from the vector. The @block_size might very well extend beyond
   the length of the vector.

   V = [ 0 , 1 , 2 , 3 , 4 , 5 , 6 , 7 , 8 , 9]

   vector_idel_block( vector , 4 , 2 ) =>  V = [ 0 , 1 , 2 , 3 , 6 , 7 , 8 , 9]

   The function is based on memmove() and probably not a high
   performance player....
*/
template <typename T> void vector<T>::idel_block(int index, int block_size) {
    this->assert_writable();
    if ((index >= 0) && (index < this->size) && (block_size >= 0)) {
        if (index + block_size > this->size)
            block_size = this->size - index;

        index += block_size;
        this->memmove(index, -block_size);
    } else
        util_abort("%s: invalid input \n", __func__);
}

/**
   Removes element @index from the vector, shifting all elements to
   the right of @index one element to the left and shrinking the total
   vector. The return value is the value which is removed.
*/

template <typename T> T vector<T>::idel(int index) {
    T del_value = this->iget(index);
    this->idel_block(index, 1);
    return del_value;
}

/**
   Removes all occurences of @value from the vector, thereby shrinking
   the vector. The return value is the number of elements removed from
   the vector.
*/

template <typename T> T vector<T>::del_value(T del_value) {
    int index = 0;
    int del_count = 0;
    while (true) {
        if (index == this->size)
            break;

        if (this->iget(index) == del_value) {
            this->idel(index);
            del_count++;
        } else
            index++;
    }
    return del_count;
}

template <typename T> void vector<T>::insert(int index, T value) {
    if (index >= this->size)
        this->iset(index, value);
    else {
        this->memmove(index, 1);
        this->iset(index, value);
    }
}

template <typename T> void vector<T>::append(T value) {
    int size = this->size;
    this->resize(size + 1, 0);
    this->data[size] = value;
}

template <typename T> void vector<T>::reset() {
    this->assert_writable();
    this->size = 0;
}

/**
   The pop function will remove the last element from the vector and
   return it. If the vector is empty - it will abort.
*/
template <typename T> T vector<T>::pop() {
    this->assert_writable();

    if (this->size > 0) {
        T value = this->data[this->size - 1];
        this->size--;
        return value;
    } else {
        util_abort("%s: trying to pop from empty vector \n", __func__);
        return -1; /* Compiler shut up. */
    }
}

template <typename T> void vector<T>::rshift(int shift) {
    if (shift < 0)
        this->memmove(-shift, shift);
    else {
        this->memmove(0, shift);
        for (int i = 0; i < shift; i++)
            this->data[i] = this->default_value;
    }
}

template <typename T> void vector<T>::lshift(int shift) {
    this->rshift(-shift);
}

template <typename T> void *vector<T>::get_ptr() {
    this->assert_writable();
    return this->data;
}

template <typename T> const void *vector<T>::get_const_ptr() const {
    return this->data;
}

/**
   Observe that there is a principle difference between the get_ptr()
   functions and alloc_data_copy() when the vector has zero size. The
   former functions will always return valid (@TYPE@ *) pointer,
   altough possibly none of the elements have been set by the vector
   instance, whereas the alloc_data_copy() function will return NULL
   in that case.
*/
// template <typename T> T *vector<T>::alloc_data_copy() {
//     int size = this->size * sizeof(T);
//     vector<T> *copy = (vector<T> *)util_calloc(this->size, sizeof *copy);
//     if (copy != NULL)
//         memcpy(copy, this->data, size);
//     return copy;
// }

template <typename T>
void vector<T>::set_many(int index, const void *data, int length) {
    this->assert_writable();

    int min_size = index + length;
    if (min_size > this->alloc_size)
        this->realloc_data__(2 * min_size, this->default_value);
    std::memcpy(&this->data[index], data, length * sizeof(T));
    if (min_size > this->size)
        this->size = min_size;
}

template <typename T> void vector<T>::set_all(T value) {
    this->assert_writable();
    for (int i = 0; i < this->size; i++)
        this->data[i] = value;
}

/**
   The bahviour of this function should closely follow the semantics
   of the Python range() function.
*/
template <typename T> void vector<T>::init_range(T value1, T value2, T delta) {
    this->reset();
    if (delta != 0) {
        bool valid_range = delta > 0 && value2 > value1;
        if constexpr (std::is_signed_v<T>) {
            valid_range |= delta < 0 && value2 < value1;
        }

        if (valid_range) {
            T current_value = value1;
            while (true) {
                this->append(current_value);
                current_value += delta;

                if (delta > 0 && current_value >= value2)
                    break;

                if constexpr (std::is_signed_v<T>)
                    if (delta < 0 && current_value <= value2)
                        break;
            }
        }
    }
}

template <typename T>
bool vector<T>::init_linear(T start_value, T end_value, int num_values) {
    if (num_values < 2)
        return false;

    this->reset();
    this->iset(0, start_value);
    double slope = (end_value - start_value) / (num_values - 1);

    for (int i = 1; i < num_values - 1; i++) {
        T value = (T)start_value + slope * i;
        this->iset(i, value);
    }
    this->iset(num_values - 1, end_value);
    return true;
}

template <typename T>
void vector<T>::append_many(const void *data, int length) {
    this->set_many(this->size, data, length);
}

template <typename T> void vector<T>::append_vector(const vector<T> &other) {
    this->append_many(other.get_const_ptr(), other.size);
}

/**
   This will realloc the vector so that alloc_size exactly matches
   size.
*/
template <typename T> void vector<T>::shrink() {
    this->realloc_data__(this->size, this->default_value);
}

template <typename T> int vector<T>::get_max_index(bool reverse) const {
    if (this->size == 0)
        util_abort("%s: can not look for max_index in an empty vector \n",
                   __func__);

    int max_index;
    int i;
    if (reverse) {
        T max_value;
        max_index = this->size - 1;
        max_value = this->data[max_index];

        for (i = this->size - 1; i >= 0; i--) {
            if (this->data[i] > max_value) {
                max_value = this->data[i];
                max_index = i;
            }
        }
    } else {
        T max_value;
        max_index = 0;
        max_value = this->data[max_index];

        for (i = 0; i < this->size; i++) {
            if (this->data[i] > max_value) {
                max_value = this->data[i];
                max_index = i;
            }
        }
    }
    return max_index;
}

template <typename T> T vector<T>::get_max() const {
    int max_index = this->get_max_index(false);
    return this->data[max_index];
}

template <typename T> int vector<T>::get_min_index(bool reverse) const {
    if (this->size == 0)
        util_abort("%s: can not look for min_index in an empty vector \n",
                   __func__);

    int min_index;
    int i;
    if (reverse) {
        T min_value;
        min_index = this->size - 1;

        min_value = this->data[min_index];
        for (i = this->size - 1; i >= 0; i--) {
            if (this->data[i] < min_value) {
                min_value = this->data[i];
                min_index = i;
            }
        }
    } else {
        T min_value;
        min_index = 0;

        min_value = this->data[min_index];
        for (i = 0; i < this->size; i++) {
            if (this->data[i] < min_value) {
                min_value = this->data[i];
                min_index = i;
            }
        }
    }
    return min_index;
}

template <typename T> T vector<T>::get_min() const {
    int min_index = this->get_min_index(false);
    return this->data[min_index];
}

template <typename T> T vector<T>::sum() const {
    T sum = 0;
    for (int i = 0; i < this->size; i++)
        sum += this->data[i];
    return sum;
}

/**
   Checks if the vector contains the value @value. The comparison is
   done with ==; i.e. it is only suitable for integer-like types.

   The implementation does a linear search through the vector and
   returns the index of the @value, or -1 if @value is not found. If
   the vector is known to be sorted you should use the
   @TYPE@_vector_index_sorted() instead.
*/
template <typename T> int vector<T>::index(T value) const {
    if (this->size) {
        int index = 0;
        while (true) {
            if (this->data[index] == value)
                break;

            index++;
            if (index == this->size) {
                index = -1; /* Not found */
                break;
            }
        }

        return index;
    } else
        return -1;
}

template <typename T> bool vector<T>::contains(T value) const {
    return this->index(value) >= 0;
}

template <typename T> bool vector<T>::contains_sorted(T value) const {
    return this->index_sorted(value) >= 0;
}

template <typename T> int vector<T>::index_sorted(T value) const {
    if (!this->size)
        return -1;

    if (value < this->data[0])
        return -1;
    if (value == this->data[0])
        return 0;

    int last_index = this->size - 1;
    if (value > this->data[last_index])
        return -1;
    if (value == this->data[last_index])
        return last_index;

    int lower_index = 0;
    int upper_index = this->size - 1;

    while (true) {
        if ((upper_index - lower_index) <= 1)
            /* Not found */
            return -1;

        {
            int center_index = (lower_index + upper_index) / 2;
            T center_value = this->data[center_index];

            if (center_value == value)
                /* Found it */
                return center_index;
            else {
                if (center_value > value)
                    upper_index = center_index;
                else
                    lower_index = center_index;
            }
        }
    }
}

/**
   The input vector will be altered in place, so that the vector only
   contains every numerical value __once__. On exit the values will be
   sorted in increasing order.

   vector = <7 , 0 , 1 , 7 , 1 , 0 , 7 , 1> => <0,1,7>
*/
template <typename T> void vector<T>::select_unique() {
    this->assert_writable();
    if (this->size > 0) {
        vector<T> copy = *this;
        copy.sort();
        this->reset();

        T previous_value = copy.iget(0);
        this->append(previous_value);

        for (int i = 1; i < copy.size; i++) {
            T value = copy.iget(i);
            if (value != previous_value)
                this->append(value);
            previous_value = value;
        }
    }
}

/**
   Inplace numerical sort of the vector; sorted in increasing order.
*/
template <typename T> void vector<T>::sort() {
    this->assert_writable();
    std::sort(this->data, this->data + this->size);
}

template <typename T> void vector<T>::rsort() {
    this->assert_writable();
    std::sort(this->data, this->data + this->size, std::greater{});
}

/**
   This function will allocate a (int *) pointer of indices,
   corresponding to the permutations of the elements in the vector to
   get it into sorted order. This permutation can then be used to sort
   several vectors identically:

   int_vector_type    * v1;
   bool_vector_type   * v2;
   double_vector_type * v2;
   .....
   .....

   {
      int * sort_perm = int_vector_alloc_sort_perm( v1 );
      int_vector_permute( v1 , sort_perm );
      bool_vector_permute( v2 , sort_perm );
      double_vector_permute( v3 , sort_perm );
      free(sort_perm);
   }
*/
template <typename T>
perm_vector_type *vector<T>::alloc_sort_perm(bool reverse) const {
    struct element {
        int index;
        T value;
    };
    std::vector<element> elements;
    elements.resize(this->size);
    int *perm = (int *)util_calloc(
        this->size,
        sizeof *perm); // The perm_vector return value will take ownership of this array.
    for (size_t i = 0; i < this->size; i++) {
        elements[i].index = i;
        elements[i].value = this->data[i];
    }
    if (reverse)
        std::sort(elements.begin(), elements.end(),
                  [](const auto &lhs, const auto &rhs) {
                      return lhs.value > rhs.value;
                  });
    else
        std::sort(elements.begin(), elements.end(),
                  [](const auto &lhs, const auto &rhs) {
                      return lhs.value < rhs.value;
                  });

    for (size_t i = 0; i < this->size; i++)
        perm[i] = elements[i].index;

    return perm_vector_alloc(perm, this->size);
}

template <typename T> void vector<T>::permute(const perm_vector_type *perm) {
    this->assert_writable();

    T *tmp = (T *)util_alloc_copy(this->data, sizeof *tmp * this->size);
    for (int i = 0; i < this->size; i++)
        this->data[i] = tmp[perm_vector_iget(perm, i)];
    free(tmp);
}

/**
   Looks through the vector and checks if it is in sorted form. If the
   @reverse argument is true it will check for descending values,
   otherwise for ascending values.
*/
template <typename T> bool vector<T>::is_sorted(bool reverse) const {
    if (reverse) {
        return std::is_sorted(data, data + size, std::greater{});
    } else {
        return std::is_sorted(data, data + size, std::less{});
    }
}

/**
   This function can be used to implement lookup tables of various
   types. The input vector @limits is supposed to contain container
   limits, e.g.

       limits = [0 , 1 , 2 , 3 , 4]

   The @limits vector must be sorted. The function will then look up
   in which bin @value fits and return the bin index. If the test

         limit[i] <= value < limit[i+1]

   succeeds the function will return i. If value is less than
   limits[0] the function will return -1, if value is greater than
   max( limit ) the function will return -1 * limit->size.

   The parameter @guess can be used to speed up the process, a value
   of @guess < 0 is interpreted as "no guess". When all input has been
   validated the final search will be binary search.
*/
template <typename T> int vector<T>::lookup_bin(T value, int guess) const {
    if (value < this->data[0])
        return -1;

    if (value > this->data[this->size - 1])
        return -1 * this->size;

    if (guess >= this->size)
        guess = -1; /* No guess */

    return this->lookup_bin__(value, guess);
}

/*
  This is the fast path and assumes that @value is within the limits,
  and that guess < limits->size. See @TYPE@_vector_lookup_bin() for
  further documentation.x
*/
template <typename T> int vector<T>::lookup_bin__(T value, int guess) const {
    if (guess >= 0) {
        if ((this->data[guess] <= value) && (this->data[guess + 1] > value))
            return guess; /* The guess was a hit. */
    }
    /* We did not have a guess - or it did not pay off. Start with a
     binary search. */
    int lower_index = 0;
    int upper_index = this->size - 1;
    while (true) {
        if ((upper_index - lower_index) == 1) {
            /* We have found it. */
            return lower_index;
        }

        int central_index = (lower_index + upper_index) / 2;
        T central_value = this->data[central_index];

        if (central_value > value)
            upper_index = central_index;
        else
            lower_index = central_index;
    }
}

template <typename T>
void vector<T>::fprintf(FILE *stream, const char *name, const char *fmt) const {
    if (name != NULL)
        ::fprintf(stream, "%s = [", name);
    else
        ::fprintf(stream, "[");

    for (int i = 0; i < this->size; i++) {
        ::fprintf(stream, fmt, this->data[i]);
        if (i < (this->size - 1))
            ::fprintf(stream, ", ");
    }

    ::fprintf(stream, "]\n");
}

/*
  This function does not consider the default value; it does a
  vector_resize based on the input size.
*/
template <typename T> void vector<T>::fread_data(int size, FILE *stream) {
    this->realloc_data__(size, this->default_value);
    util_fread(this->data, sizeof *this->data, size, stream, __func__);
    this->size = size;
}

template <typename T> void vector<T>::fwrite_data(FILE *stream) const {
    util_fwrite(this->data, sizeof *this->data, this->size, stream, __func__);
}

/**
   Writing:
   1. Size
   2. default value
   3. Values
*/
template <typename T> void vector<T>::fwrite(FILE *stream) const {
    util_fwrite_int(this->size, stream);
    util_fwrite(&this->default_value, sizeof this->default_value, 1, stream,
                __func__);
    this->fwrite_data(stream);
}

/*
  Observe that this function will reset the default value.
*/
template <typename T> void vector<T>::fread(FILE *stream) {
    T default_value;
    int size = util_fread_int(stream);
    util_fread(&default_value, sizeof default_value, 1, stream, __func__);
    this->set_default(default_value);
    this->fread_data(size, stream);
}

template <typename T> bool vector<T>::equal(const vector<T> &other) const {
    if (this->size == other.size) {
        return memcmp(this->data, other.data,
                      sizeof *this->data * this->size) == 0;
    }
    return false;
}

template <typename T>
int vector<T>::first_equal(const vector<T> &other, int offset) const {
    if (offset >= this->size)
        return -2;

    if (offset >= other.size)
        return -2;

    int index = offset;
    while (this->data[index] != other.data[index]) {
        index++;

        if (index == this->size)
            return -1;

        if (index == other.size)
            return -1;
    }

    return index;
}

template <typename T>
int vector<T>::first_not_equal(const vector<T> &other, int offset) const {
    if (offset >= this->size)
        return -2;

    if (offset >= other.size)
        return -2;

    int index = offset;
    while (this->data[index] == other.data[index]) {
        index++;

        if (index == this->size)
            return -1;

        if (index == other.size)
            return -1;
    }

    return index;
}

template <typename T> void vector<T>::apply(vector<T>::ftype *func) {
    this->assert_writable();
    for (int i = 0; i < this->size; i++)
        this->data[i] = func(this->data[i]);
}

template <typename T> int vector<T>::count_equal(T cmp_value) const {
    int count = 0;
    for (int i = 0; i < this->size; i++)
        if (this->data[i] == cmp_value)
            count += 1;
    return count;
}

/*
  The upper limit is inclusive - if it is commensurable with the
  delta.
*/
template <typename T> void vector<T>::range_fill(T limit1, T delta, T limit2) {
    T current_value = limit1;

    if (delta == 0)
        util_abort("%s: sorry can not have delta == 0 \n", __func__);

    this->reset();
    while (true) {
        this->append(current_value);
        current_value += delta;
        if (delta > 0 && current_value > limit2)
            break;

        if constexpr (std::is_signed_v<T>)
            if (delta < 0 && current_value < limit2)
                break;
    }
}

#define DEFINE(_Prefix, _Type, _LocalType)                                     \
    using _Prefix##_vector_type = vector<_LocalType>;                          \
    using _Prefix##_ftype = typename vector<_LocalType>::ftype;                \
    int _Prefix##_vector_lookup_bin(const _Prefix##_vector_type *limits,       \
                                    _Type value, int guess) {                  \
        return limits->lookup_bin(value, guess);                               \
    }                                                                          \
    int _Prefix##_vector_lookup_bin__(const _Prefix##_vector_type *limits,     \
                                      _Type value, int guess) {                \
        return limits->lookup_bin__(value, guess);                             \
    }                                                                          \
    void _Prefix##_vector_inplace_div(                                         \
        _Prefix##_vector_type *vector,                                         \
        const _Prefix##_vector_type *inv_factor) {                             \
        vector->inplace_div(*inv_factor);                                      \
    }                                                                          \
    void _Prefix##_vector_inplace_mul(_Prefix##_vector_type *vector,           \
                                      const _Prefix##_vector_type *factor) {   \
        vector->inplace_mul(*factor);                                          \
    }                                                                          \
    void _Prefix##_vector_inplace_add(_Prefix##_vector_type *vector,           \
                                      const _Prefix##_vector_type *delta) {    \
        vector->inplace_add(*delta);                                           \
    }                                                                          \
    void _Prefix##_vector_inplace_sub(_Prefix##_vector_type *vector,           \
                                      const _Prefix##_vector_type *delta) {    \
        vector->inplace_sub(*delta);                                           \
    }                                                                          \
    void _Prefix##_vector_set_read_only(_Prefix##_vector_type *vector,         \
                                        bool read_only) {                      \
        vector->read_only = read_only;                                         \
    }                                                                          \
    bool _Prefix##_vector_get_read_only(const _Prefix##_vector_type *vector) { \
        return vector->read_only;                                              \
    }                                                                          \
    void _Prefix##_vector_memcpy_data(_Type *target,                           \
                                      const _Prefix##_vector_type *src) {      \
        src->memcpy_data(target);                                              \
    }                                                                          \
    void _Prefix##_vector_memcpy_from_data(_Prefix##_vector_type *target,      \
                                           const _Type *src, int src_size) {   \
        target->memcpy_from_data(src, src_size);                               \
    }                                                                          \
    void _Prefix##_vector_memcpy(_Prefix##_vector_type *target,                \
                                 const _Prefix##_vector_type *src) {           \
        target->memcpy(*src);                                                  \
    }                                                                          \
    void _Prefix##_vector_memcpy_data_block(                                   \
        _Prefix##_vector_type *target, const _Prefix##_vector_type *src,       \
        int target_offset, int src_offset, int len) {                          \
        target->memcpy_data_block(*src, target_offset, src_offset, len);       \
    }                                                                          \
    bool _Prefix##_vector_growable(const _Prefix##_vector_type *vector) {      \
        return vector->data_owner;                                             \
    }                                                                          \
    void _Prefix##_vector_select_unique(_Prefix##_vector_type *vector) {       \
        return vector->select_unique();                                        \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_alloc(int init_size,               \
                                                  _Type default_value) {       \
        return new vector<_LocalType>(init_size, default_value);               \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_alloc_private_wrapper(             \
        int init_size, _Type default_value, _Type *data, int alloc_size) {     \
        return new vector<_LocalType>{init_size, default_value, data,          \
                                      alloc_size, false};                      \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_alloc_shared_wrapper(              \
        int init_size, _Type default_value, _Type *data, int alloc_size) {     \
        return new vector<_LocalType>{init_size, default_value, data,          \
                                      alloc_size, false};                      \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_alloc_strided_copy(                \
        const _Prefix##_vector_type *src, int start, int stop, int stride) {   \
        auto copy = new vector<_LocalType>{0, src->default_value};             \
        if (start < 0)                                                         \
            start = src->size - start;                                         \
        if (stop < 0)                                                          \
            stop = src->size - stop;                                           \
        int src_index = start;                                                 \
        while (src_index < stop) {                                             \
            copy->append(src->iget(src_index));                                \
            src_index += stride;                                               \
        }                                                                      \
        return copy;                                                           \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_alloc_copy(                        \
        const _Prefix##_vector_type *src) {                                    \
        return new vector<_LocalType>{*src};                                   \
    }                                                                          \
    void _Prefix##_vector_imul(_Prefix##_vector_type *vector, int index,       \
                               _Type factor) {                                 \
        vector->imul(index, factor);                                           \
    }                                                                          \
    void _Prefix##_vector_scale(_Prefix##_vector_type *vector, _Type factor) { \
        vector->scale(factor);                                                 \
    }                                                                          \
    void _Prefix##_vector_div(_Prefix##_vector_type *vector, _Type divisor) {  \
        vector->div(divisor);                                                  \
    }                                                                          \
    _Type _Prefix##_vector_reverse_iget(const _Prefix##_vector_type *vector,   \
                                        int index) {                           \
        return vector->reverse_iget(index);                                    \
    }                                                                          \
    _Type _Prefix##_vector_iget(const _Prefix##_vector_type *vector,           \
                                int index) {                                   \
        return vector->iget(index);                                            \
    }                                                                          \
    _Type _Prefix##_vector_safe_iget(const _Prefix##_vector_type *vector,      \
                                     int index) {                              \
        return vector->safe_iget(index);                                       \
    }                                                                          \
    _Type _Prefix##_vector_get_min(const _Prefix##_vector_type *vector) {      \
        return vector->get_min();                                              \
    }                                                                          \
    _Type _Prefix##_vector_get_max(const _Prefix##_vector_type *vector) {      \
        return vector->get_max();                                              \
    }                                                                          \
    int _Prefix##_vector_get_min_index(const _Prefix##_vector_type *vector,    \
                                       bool reverse) {                         \
        return vector->get_min_index(reverse);                                 \
    }                                                                          \
    int _Prefix##_vector_get_max_index(const _Prefix##_vector_type *vector,    \
                                       bool reverse) {                         \
        return vector->get_max_index(reverse);                                 \
    }                                                                          \
    _Type _Prefix##_vector_iadd(_Prefix##_vector_type *vector, int index,      \
                                _Type delta) {                                 \
        return vector->iadd(index, delta);                                     \
    }                                                                          \
    void _Prefix##_vector_resize(_Prefix##_vector_type *vector, int new_size,  \
                                 _Type default_value) {                        \
        vector->resize(new_size, default_value);                               \
    }                                                                          \
    void _Prefix##_vector_iset(_Prefix##_vector_type *vector, int index,       \
                               _Type value) {                                  \
        vector->iset(index, value);                                            \
    }                                                                          \
    void _Prefix##_vector_iset_block(_Prefix##_vector_type *vector, int index, \
                                     int block_size, _Type value) {            \
        vector->iset_block(index, block_size, value);                          \
    }                                                                          \
    void _Prefix##_vector_idel_block(_Prefix##_vector_type *vector, int index, \
                                     int block_size) {                         \
        vector->idel_block(index, block_size);                                 \
    }                                                                          \
    _Type _Prefix##_vector_idel(_Prefix##_vector_type *vector, int index) {    \
        return vector->idel(index);                                            \
    }                                                                          \
    _Type _Prefix##_vector_del_value(_Prefix##_vector_type *vector,            \
                                     _Type del_value) {                        \
        return vector->del_value(del_value);                                   \
    }                                                                          \
    void _Prefix##_vector_insert(_Prefix##_vector_type *vector, int index,     \
                                 _Type value) {                                \
        vector->insert(index, value);                                          \
    }                                                                          \
    void _Prefix##_vector_append(_Prefix##_vector_type *vector, _Type value) { \
        vector->append(value);                                                 \
    }                                                                          \
    void _Prefix##_vector_free_container(_Prefix##_vector_type *vector) {      \
        delete vector;                                                         \
    }                                                                          \
    void _Prefix##_vector_free(_Prefix##_vector_type *vector) {                \
        /*use std::free rather than delete because the */                      \
        /*data could've been provided by the user*/                            \
        if (vector->data_owner)                                                \
            free(vector->data);                                                \
        delete vector;                                                         \
    }                                                                          \
    void _Prefix##_vector_free__(void *__vector) {                             \
        _Prefix##_vector_free(static_cast<vector<_LocalType> *>(__vector));    \
    }                                                                          \
    void _Prefix##_vector_free_data(_Prefix##_vector_type *vector) {           \
        vector->reset();                                                       \
        vector->realloc_data__(0, vector->default_value);                      \
    }                                                                          \
    void _Prefix##_vector_reset(_Prefix##_vector_type *vector) {               \
        vector->reset();                                                       \
    }                                                                          \
    void _Prefix##_vector_reset__(void *__vector) {                            \
        static_cast<_Prefix##_vector_type *>(__vector)->reset();               \
    }                                                                          \
    int _Prefix##_vector_size(const _Prefix##_vector_type *vector) {           \
        return vector->size;                                                   \
    }                                                                          \
    void _Prefix##_vector_lshift(_Prefix##_vector_type *vector, int shift) {   \
        vector->lshift(shift);                                                 \
    }                                                                          \
    void _Prefix##_vector_rshift(_Prefix##_vector_type *vector, int shift) {   \
        vector->rshift(shift);                                                 \
    }                                                                          \
    _Type _Prefix##_vector_pop(_Prefix##_vector_type *vector) {                \
        return vector->pop();                                                  \
    }                                                                          \
    _Type _Prefix##_vector_get_first(const _Prefix##_vector_type *vector) {    \
        return vector->get_first();                                            \
    }                                                                          \
    _Type _Prefix##_vector_get_last(const _Prefix##_vector_type *vector) {     \
        return vector->get_last();                                             \
    }                                                                          \
    _Type *_Prefix##_vector_get_ptr(_Prefix##_vector_type *vector) {           \
        return static_cast<_Type *>(vector->get_ptr());                        \
    }                                                                          \
    _Type *_Prefix##_vector_alloc_data_copy(                                   \
        const _Prefix##_vector_type *vector) {                                 \
        int size = vector->size * sizeof(_Type);                               \
        _Type *copy = (_Type *)util_calloc(vector->size, sizeof *copy);        \
        if (copy != NULL)                                                      \
            memcpy(copy, vector->data, size);                                  \
        return copy;                                                           \
    }                                                                          \
    const _Type *_Prefix##_vector_get_const_ptr(                               \
        const _Prefix##_vector_type *vector) {                                 \
        return static_cast<const _Type *>(vector->get_const_ptr());            \
    }                                                                          \
    bool _Prefix##_vector_init_linear(_Prefix##_vector_type *vector,           \
                                      _Type start_value, _Type end_value,      \
                                      int num_values) {                        \
        return vector->init_linear(start_value, end_value, num_values);        \
    }                                                                          \
    void _Prefix##_vector_init_range(_Prefix##_vector_type *vector,            \
                                     _Type value1, _Type value2,               \
                                     _Type delta) {                            \
        vector->init_range(value1, value2, delta);                             \
    }                                                                          \
    void _Prefix##_vector_set_many(_Prefix##_vector_type *vector, int index,   \
                                   const _Type *data, int length) {            \
        vector->set_many(index, data, length);                                 \
    }                                                                          \
    void _Prefix##_vector_set_all(_Prefix##_vector_type *vector,               \
                                  _Type value) {                               \
        vector->set_all(value);                                                \
    }                                                                          \
    void _Prefix##_vector_append_many(_Prefix##_vector_type *vector,           \
                                      const _Type *data, int length) {         \
        vector->append_many(data, length);                                     \
    }                                                                          \
    void _Prefix##_vector_append_vector(_Prefix##_vector_type *vector,         \
                                        const _Prefix##_vector_type *other) {  \
        vector->append_vector(*other);                                         \
    }                                                                          \
    void _Prefix##_vector_shrink(_Prefix##_vector_type *vector) {              \
        vector->shrink();                                                      \
    }                                                                          \
    _Type _Prefix##_vector_sum(const _Prefix##_vector_type *vector) {          \
        return vector->sum();                                                  \
    }                                                                          \
    _Type _Prefix##_vector_get_default(const _Prefix##_vector_type *vector) {  \
        return vector->default_value;                                          \
    }                                                                          \
    void _Prefix##_vector_set_default(_Prefix##_vector_type *vector,           \
                                      _Type default_value) {                   \
        return vector->set_default(default_value);                             \
    }                                                                          \
    void _Prefix##_vector_append_default(_Prefix##_vector_type *vector,        \
                                         _Type default_value) {                \
        vector->append_default(default_value);                                 \
    }                                                                          \
    void _Prefix##_vector_iset_default(_Prefix##_vector_type *vector,          \
                                       int index, _Type default_value) {       \
        vector->iset_default(index, default_value);                            \
    }                                                                          \
    bool _Prefix##_vector_is_sorted(const _Prefix##_vector_type *vector,       \
                                    bool reverse) {                            \
        return vector->is_sorted(reverse);                                     \
    }                                                                          \
    bool _Prefix##_vector_contains(const _Prefix##_vector_type *vector,        \
                                   _Type value) {                              \
        return vector->contains(value);                                        \
    }                                                                          \
    bool _Prefix##_vector_contains_sorted(const _Prefix##_vector_type *vector, \
                                          _Type value) {                       \
        return vector->contains_sorted(value);                                 \
    }                                                                          \
    int _Prefix##_vector_index(const _Prefix##_vector_type *vector,            \
                               _Type value) {                                  \
        return vector->index(value);                                           \
    }                                                                          \
    int _Prefix##_vector_index_sorted(const _Prefix##_vector_type *vector,     \
                                      _Type value) {                           \
        return vector->index_sorted(value);                                    \
    }                                                                          \
    void _Prefix##_vector_sort(_Prefix##_vector_type *vector) {                \
        return vector->sort();                                                 \
    }                                                                          \
    void _Prefix##_vector_rsort(_Prefix##_vector_type *vector) {               \
        return vector->rsort();                                                \
    }                                                                          \
    void _Prefix##_vector_permute(_Prefix##_vector_type *vector,               \
                                  const perm_vector_type *perm) {              \
        return vector->permute(perm);                                          \
    }                                                                          \
    perm_vector_type *_Prefix##_vector_alloc_sort_perm(                        \
        const _Prefix##_vector_type *vector) {                                 \
        return vector->alloc_sort_perm(false);                                 \
    }                                                                          \
    perm_vector_type *_Prefix##_vector_alloc_rsort_perm(                       \
        const _Prefix##_vector_type *vector) {                                 \
        return vector->alloc_sort_perm(true);                                  \
    }                                                                          \
    void _Prefix##_vector_fprintf(const _Prefix##_vector_type *vector,         \
                                  FILE *stream, const char *name,              \
                                  const char *fmt) {                           \
        vector->fprintf(stream, name, fmt);                                    \
    }                                                                          \
    void _Prefix##_vector_fwrite(const _Prefix##_vector_type *vector,          \
                                 FILE *stream) {                               \
        vector->fwrite(stream);                                                \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_fread_alloc(FILE *stream) {        \
        auto v = new vector<_LocalType>(0, 0);                                 \
        v->fread(stream);                                                      \
        return v;                                                              \
    }                                                                          \
    void _Prefix##_vector_fread(_Prefix##_vector_type *vector, FILE *stream) { \
        vector->fread(stream);                                                 \
    }                                                                          \
    void _Prefix##_vector_fwrite_data(const _Prefix##_vector_type *vector,     \
                                      FILE *stream) {                          \
        vector->fwrite_data(stream);                                           \
    }                                                                          \
    void _Prefix##_vector_fread_data(_Prefix##_vector_type *vector, int size,  \
                                     FILE *stream) {                           \
        vector->fread_data(size, stream);                                      \
    }                                                                          \
    bool _Prefix##_vector_equal(const _Prefix##_vector_type *vector1,          \
                                const _Prefix##_vector_type *vector2) {        \
        return vector1->equal(*vector2);                                       \
    }                                                                          \
    int _Prefix##_vector_first_equal(const _Prefix##_vector_type *vector1,     \
                                     const _Prefix##_vector_type *vector2,     \
                                     int offset) {                             \
        return vector1->first_equal(*vector2, offset);                         \
    }                                                                          \
    int _Prefix##_vector_first_not_equal(const _Prefix##_vector_type *vector1, \
                                         const _Prefix##_vector_type *vector2, \
                                         int offset) {                         \
        return vector1->first_not_equal(*vector2, offset);                     \
    }                                                                          \
    void _Prefix##_vector_apply(_Prefix##_vector_type *vector,                 \
                                _Prefix##_ftype *func);                        \
    int _Prefix##_vector_count_equal(const _Prefix##_vector_type *vector,      \
                                     _Type cmp_value) {                        \
        return vector->count_equal(cmp_value);                                 \
    }                                                                          \
    int _Prefix##_vector_element_size(const _Prefix##_vector_type *vector) {   \
        return sizeof(_Type);                                                  \
    }                                                                          \
    void _Prefix##_vector_range_fill(_Prefix##_vector_type *vector,            \
                                     _Type limit1, _Type delta,                \
                                     _Type limit2) {                           \
        vector->range_fill(limit1, delta, limit2);                             \
    }                                                                          \
    void _Prefix##_vector_shift(_Prefix##_vector_type *vector, _Type delta) {  \
        vector->shift(delta);                                                  \
    }                                                                          \
    bool _Prefix##_vector_is_instance(const void *__arg) {                     \
        return _Prefix##_vector_type::is_instance(__arg);                      \
    }                                                                          \
    _Prefix##_vector_type *_Prefix##_vector_safe_cast(void *__ptr) {           \
        return _Prefix##_vector_type::safe_cast(__ptr);                        \
    }

#define DEFINE2(_Type) DEFINE(_Type, _Type, _Type)
extern "C" {
DEFINE2(int)
DEFINE2(long)
DEFINE2(bool)
DEFINE2(float)
DEFINE2(double)
DEFINE2(size_t)
DEFINE(time_t, time_t, strict_time)
}
