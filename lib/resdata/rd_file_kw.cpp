#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <ert/util/size_t_vector.hpp>
#include <ert/util/util.h>

#include <resdata/rd_util.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/fortio.h>

/*
  This file implements the datatype rd_file_kw which is used to hold
  header-information about an rd_kw instance on file. When a
  rd_file_kw instance is created it is initialized with the header
  information (name, size, type) for an rd_kw instance and the offset
  in a file actually containing the keyword.

  If and when the keyword is actually queried for at a later stage the
  rd_file_kw_get_kw() method will seek to the keyword position in an
  open fortio instance and call rd_kw_fread_alloc() to instantiate
  the keyword itself.

  The rd_file_kw datatype is mainly used by the rd_file datatype;
  whose index tables consists of rd_file_kw instances.
*/

#define RD_FILE_KW_TYPE_ID 646107

struct inv_map_struct {
    size_t_vector_type *file_kw_ptr;
    size_t_vector_type *rd_kw_ptr;
    bool sorted;
};

struct rd_file_kw_struct {
    UTIL_TYPE_ID_DECLARATION;
    offset_type file_offset;
    rd_data_type data_type;
    int kw_size;
    int ref_count;
    char *header;
    rd_kw_type *kw;
};

inv_map_type *inv_map_alloc() {
    inv_map_type *map = (inv_map_type *)util_malloc(sizeof *map);
    map->file_kw_ptr = size_t_vector_alloc(0, 0);
    map->rd_kw_ptr = size_t_vector_alloc(0, 0);
    map->sorted = false;
    return map;
}

void inv_map_free(inv_map_type *map) {
    size_t_vector_free(map->file_kw_ptr);
    size_t_vector_free(map->rd_kw_ptr);
    free(map);
}

static void inv_map_assert_sort(inv_map_type *map) {
    if (!map->sorted) {
        perm_vector_type *perm = size_t_vector_alloc_sort_perm(map->rd_kw_ptr);

        size_t_vector_permute(map->rd_kw_ptr, perm);
        size_t_vector_permute(map->file_kw_ptr, perm);
        map->sorted = true;

        perm_vector_free(perm);
    }
}

static void inv_map_drop_kw(inv_map_type *map, const rd_kw_type *rd_kw) {
    inv_map_assert_sort(map);
    {
        int index = size_t_vector_index_sorted(map->rd_kw_ptr, (size_t)rd_kw);
        if (index == -1)
            util_abort("%s: trying to drop non-existent kw \n", __func__);

        size_t_vector_idel(map->rd_kw_ptr, index);
        size_t_vector_idel(map->file_kw_ptr, index);
        map->sorted = false;
    }
}

static void inv_map_add_kw(inv_map_type *map, const rd_file_kw_type *file_kw,
                           const rd_kw_type *rd_kw) {
    size_t_vector_append(map->file_kw_ptr, (size_t)file_kw);
    size_t_vector_append(map->rd_kw_ptr, (size_t)rd_kw);
    map->sorted = false;
}

rd_file_kw_type *inv_map_get_file_kw(inv_map_type *inv_map,
                                     const rd_kw_type *rd_kw) {
    inv_map_assert_sort(inv_map);
    {
        int index =
            size_t_vector_index_sorted(inv_map->rd_kw_ptr, (size_t)rd_kw);
        if (index == -1)
            /* rd_kw ptr not found. */
            return NULL;
        else
            return (rd_file_kw_type *)size_t_vector_iget(inv_map->file_kw_ptr,
                                                         index);
    }
}

static UTIL_SAFE_CAST_FUNCTION(rd_file_kw, RD_FILE_KW_TYPE_ID)
    UTIL_IS_INSTANCE_FUNCTION(rd_file_kw, RD_FILE_KW_TYPE_ID)

        rd_file_kw_type *rd_file_kw_alloc0(const char *header,
                                           rd_data_type data_type, int size,
                                           offset_type offset) {
    rd_file_kw_type *file_kw = (rd_file_kw_type *)util_malloc(sizeof *file_kw);
    UTIL_TYPE_ID_INIT(file_kw, RD_FILE_KW_TYPE_ID);

    file_kw->header = util_alloc_string_copy(header);
    memcpy(&file_kw->data_type, &data_type, sizeof data_type);
    file_kw->kw_size = size;
    file_kw->file_offset = offset;
    file_kw->ref_count = 0;
    file_kw->kw = NULL;

    return file_kw;
}

/**
   Create a new rd_file_kw instance based on header information from
   the input keyword. Typically only the header has been loaded from
   the keyword.

   Observe that it is the users responsability that the @offset
   argument in rd_file_kw_alloc() comes from the same fortio instance
   as used when calling rd_file_kw_get_kw() to actually instatiate
   the rd_kw. This is automatically assured when using rd_file to
   access the rd_file_kw instances.
*/

rd_file_kw_type *rd_file_kw_alloc(const rd_kw_type *rd_kw, offset_type offset) {
    return rd_file_kw_alloc0(rd_kw_get_header(rd_kw),
                             rd_kw_get_data_type(rd_kw), rd_kw_get_size(rd_kw),
                             offset);
}

/**
    Does NOT copy the kw pointer which must be reloaded.
*/
rd_file_kw_type *rd_file_kw_alloc_copy(const rd_file_kw_type *src) {
    return rd_file_kw_alloc0(src->header, rd_file_kw_get_data_type(src),
                             src->kw_size, src->file_offset);
}

void rd_file_kw_free(rd_file_kw_type *file_kw) {
    if (file_kw->kw != NULL) {
        rd_kw_free(file_kw->kw);
        file_kw->kw = NULL;
    }
    free(file_kw->header);
    free(file_kw);
}

void rd_file_kw_free__(void *arg) {
    rd_file_kw_type *file_kw = rd_file_kw_safe_cast(arg);
    rd_file_kw_free(file_kw);
}

bool rd_file_kw_equal(const rd_file_kw_type *kw1, const rd_file_kw_type *kw2) {
    if (kw1->file_offset != kw2->file_offset)
        return false;

    if (kw1->kw_size != kw2->kw_size)
        return false;

    if (!rd_type_is_equal(kw1->data_type, kw2->data_type))
        return false;

    return util_string_equal(kw1->header, kw2->header);
}

static void rd_file_kw_assert_kw(const rd_file_kw_type *file_kw) {
    if (!rd_type_is_equal(rd_file_kw_get_data_type(file_kw),
                          rd_kw_get_data_type(file_kw->kw)))
        util_abort("%s: type mismatch between header and file.\n", __func__);

    if (file_kw->kw_size != rd_kw_get_size(file_kw->kw))
        util_abort("%s: size mismatch between header and file.\n", __func__);

    if (strcmp(file_kw->header, rd_kw_get_header(file_kw->kw)) != 0)
        util_abort("%s: name mismatch between header and file.\n", __func__);
}

static void rd_file_kw_drop_kw(rd_file_kw_type *file_kw,
                               inv_map_type *inv_map) {
    if (file_kw->kw != NULL) {
        inv_map_drop_kw(inv_map, file_kw->kw);
        rd_kw_free(file_kw->kw);
        file_kw->kw = NULL;
    }
}

static void rd_file_kw_load_kw(rd_file_kw_type *file_kw, fortio_type *fortio,
                               inv_map_type *inv_map) {
    if (fortio == NULL)
        util_abort("%s: trying to load a keyword after the backing file has "
                   "been detached.\n",
                   __func__);

    if (file_kw->kw != NULL)
        rd_file_kw_drop_kw(file_kw, inv_map);

    {
        fortio_fseek(fortio, file_kw->file_offset, SEEK_SET);
        file_kw->kw = rd_kw_fread_alloc(fortio);
        rd_file_kw_assert_kw(file_kw);
        inv_map_add_kw(inv_map, file_kw, file_kw->kw);
    }
}

/*
  Calling scope will handle the NULL return value, and (optionally)
  reopen the fortio stream and then call the rd_file_kw_get_kw()
  function.
*/

rd_kw_type *rd_file_kw_get_kw_ptr(rd_file_kw_type *file_kw) {
    if (file_kw->ref_count == 0)
        return NULL;

    file_kw->ref_count++;
    return file_kw->kw;
}

/*
  Will return the rd_kw instance of this file_kw; if it is not
  currently loaded the method will instantiate the rd_kw instance
  from the @fortio input handle.

  After loading the keyword it will be kept in memory, so a possible
  subsequent lookup will be served from memory.

  The rd_file layer maintains a pointer mapping between the
  rd_kw_type pointers and their rd_file_kw_type containers; this
  mapping needs the new_load return value from the
  rd_file_kw_get_kw() function.
*/

rd_kw_type *rd_file_kw_get_kw(rd_file_kw_type *file_kw, fortio_type *fortio,
                              inv_map_type *inv_map) {
    if (file_kw->ref_count == 0)
        rd_file_kw_load_kw(file_kw, fortio, inv_map);

    if (file_kw->kw)
        file_kw->ref_count++;

    return file_kw->kw;
}

bool rd_file_kw_ptr_eq(const rd_file_kw_type *file_kw,
                       const rd_kw_type *rd_kw) {
    if (file_kw->kw == rd_kw)
        return true;
    else
        return false;
}

void rd_file_kw_replace_kw(rd_file_kw_type *file_kw, fortio_type *target,
                           rd_kw_type *new_kw) {
    if (!rd_type_is_equal(rd_file_kw_get_data_type(file_kw),
                          rd_kw_get_data_type(new_kw)))
        util_abort("%s: sorry type mismatch between in-file keyword and new "
                   "keyword \n",
                   __func__);
    if (file_kw->kw_size != rd_kw_get_size(new_kw))
        util_abort("%s: sorry size mismatch between in-file keyword and new "
                   "keyword \n",
                   __func__);

    if (file_kw->kw != NULL)
        rd_kw_free(file_kw->kw);

    file_kw->kw = new_kw;
    fortio_fseek(target, file_kw->file_offset, SEEK_SET);
    rd_kw_fwrite(file_kw->kw, target);
}

const char *rd_file_kw_get_header(const rd_file_kw_type *file_kw) {
    return file_kw->header;
}

int rd_file_kw_get_size(const rd_file_kw_type *file_kw) {
    return file_kw->kw_size;
}

rd_data_type rd_file_kw_get_data_type(const rd_file_kw_type *file_kw) {
    return file_kw->data_type;
}

offset_type rd_file_kw_get_offset(const rd_file_kw_type *file_kw) {
    return file_kw->file_offset;
}

bool rd_file_kw_fskip_data(const rd_file_kw_type *file_kw,
                           fortio_type *fortio) {
    return rd_kw_fskip_data__(rd_file_kw_get_data_type(file_kw),
                              file_kw->kw_size, fortio);
}

/**
   This function will replace the file content of the keyword pointed
   to by @file_kw, with the new content given by @rd_kw. The new
   @rd_kw keyword must have identical header to the one already
   present in the file.
*/

void rd_file_kw_inplace_fwrite(rd_file_kw_type *file_kw, fortio_type *fortio) {
    rd_file_kw_assert_kw(file_kw);
    fortio_fseek(fortio, file_kw->file_offset, SEEK_SET);
    rd_kw_fskip_header(fortio);
    fortio_fclean(fortio);
    rd_kw_fwrite_data(file_kw->kw, fortio);
}

void rd_file_kw_fwrite(const rd_file_kw_type *file_kw, FILE *stream) {
    int header_length = strlen(file_kw->header);
    for (int i = 0; i < RD_STRING8_LENGTH; i++) {
        if (i < header_length)
            fputc(file_kw->header[i], stream);
        else
            fputc(' ', stream);
    }

    util_fwrite_int(file_kw->kw_size, stream);
    util_fwrite_offset(file_kw->file_offset, stream);
    util_fwrite_int(rd_type_get_type(file_kw->data_type), stream);
    util_fwrite_size_t(rd_type_get_sizeof_iotype(file_kw->data_type), stream);
}

rd_file_kw_type **rd_file_kw_fread_alloc_multiple(FILE *stream, int num) {

    size_t file_kw_size = RD_STRING8_LENGTH + 2 * sizeof(int) +
                          sizeof(offset_type) + sizeof(size_t);
    size_t buffer_size = num * file_kw_size;
    char *buffer = (char *)util_malloc(buffer_size * sizeof *buffer);
    size_t num_read = fread(buffer, 1, buffer_size, stream);

    if (num_read != buffer_size) {
        free(buffer);
        return NULL;
    }

    {
        rd_file_kw_type **kw_list =
            (rd_file_kw_type **)util_malloc(num * sizeof *kw_list);
        for (int ikw = 0; ikw < num; ikw++) {
            int buffer_offset = ikw * file_kw_size;
            char header[RD_STRING8_LENGTH + 1];
            int kw_size;
            offset_type file_offset;
            rd_type_enum rd_type;
            size_t type_size;
            {
                int index = 0;
                while (true) {
                    if (buffer[index + buffer_offset] != ' ')
                        header[index] = buffer[index + buffer_offset];
                    else
                        break;

                    index++;
                    if (index == RD_STRING8_LENGTH)
                        break;
                }
                header[index] = '\0';
                buffer_offset += RD_STRING8_LENGTH;
            }

            memcpy(&kw_size, &buffer[buffer_offset], sizeof kw_size);
            buffer_offset += sizeof kw_size;

            memcpy(&file_offset, &buffer[buffer_offset], sizeof file_offset);
            buffer_offset += sizeof file_offset;

            memcpy(&rd_type, &buffer[buffer_offset], sizeof rd_type);
            buffer_offset += sizeof rd_type;

            memcpy(&type_size, &buffer[buffer_offset], sizeof type_size);
            buffer_offset += sizeof type_size;

            kw_list[ikw] =
                rd_file_kw_alloc0(header, rd_type_create(rd_type, type_size),
                                  kw_size, file_offset);
        }

        free(buffer);
        return kw_list;
    }
}

rd_file_kw_type *rd_file_kw_fread_alloc(FILE *stream) {
    rd_file_kw_type *file_kw = NULL;
    rd_file_kw_type **multiple = rd_file_kw_fread_alloc_multiple(stream, 1);

    if (multiple) {
        file_kw = multiple[0];
        free(multiple);
    }

    return file_kw;
}

void rd_file_kw_start_transaction(const rd_file_kw_type *file_kw,
                                  int *ref_count) {
    *ref_count = file_kw->ref_count;
}

void rd_file_kw_end_transaction(rd_file_kw_type *file_kw, int ref_count) {
    if (ref_count == 0 && file_kw->ref_count > 0) {
        rd_kw_free(file_kw->kw);
        file_kw->kw = NULL;
    }
    file_kw->ref_count = ref_count;
}
