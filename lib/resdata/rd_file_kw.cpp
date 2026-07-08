#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ios>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <ert/util/size_t_vector.hpp>
#include <ert/util/util.hpp>

#include <resdata/rd_kw.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/FortIO.hpp>
#include <ert/util/perm_vector.hpp>
#include <resdata/rd_type.hpp>

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

struct inv_map_struct {
    size_t_vector_ptr file_kw_ptr = make_size_t_vector(0, 0);
    size_t_vector_ptr rd_kw_ptr = make_size_t_vector(0, 0);
    bool sorted = false;
};

inv_map_type *inv_map_alloc() { return new inv_map_type(); }

void inv_map_free(inv_map_type *map) { delete map; }

static void inv_map_assert_sort(inv_map_type *map) {
    if (!map->sorted) {
        std::unique_ptr<perm_vector_type, decltype(&perm_vector_free)> perm(
            size_t_vector_alloc_sort_perm(map->rd_kw_ptr.get()),
            perm_vector_free);

        size_t_vector_permute(map->rd_kw_ptr.get(), perm.get());
        size_t_vector_permute(map->file_kw_ptr.get(), perm.get());
        map->sorted = true;
    }
}

static void inv_map_drop_kw(inv_map_type *map, const rd_kw_type *rd_kw) {
    inv_map_assert_sort(map);
    {
        int index =
            size_t_vector_index_sorted(map->rd_kw_ptr.get(), (size_t)rd_kw);
        if (index == -1)
            throw std::logic_error(std::string(__func__) +
                                   ": trying to drop non-existent kw");

        size_t_vector_idel(map->rd_kw_ptr.get(), index);
        size_t_vector_idel(map->file_kw_ptr.get(), index);
        map->sorted = false;
    }
}

static void inv_map_add_kw(inv_map_type *map, const rd_file_kw_type *file_kw,
                           const rd_kw_type *rd_kw) {
    size_t_vector_append(map->file_kw_ptr.get(), (size_t)file_kw);
    size_t_vector_append(map->rd_kw_ptr.get(), (size_t)rd_kw);
    map->sorted = false;
}

rd_file_kw_type *inv_map_get_file_kw(inv_map_type *inv_map,
                                     const rd_kw_type *rd_kw) {
    inv_map_assert_sort(inv_map);
    {
        int index =
            size_t_vector_index_sorted(inv_map->rd_kw_ptr.get(), (size_t)rd_kw);
        if (index == -1)
            /* rd_kw ptr not found. */
            return NULL;
        else
            return (rd_file_kw_type *)size_t_vector_iget(
                inv_map->file_kw_ptr.get(), index);
    }
}

void rd_file_kw_free(rd_file_kw_type *file_kw) {
    file_kw->kw.reset(nullptr);
    delete file_kw;
}

static void rd_file_kw_assert_kw(const rd_file_kw_type *file_kw) {
    if (!file_kw->kw)
        throw std::runtime_error(
            "rd_file_kw: keyword could not be loaded from file "
            "(rd_kw_fread_alloc returned NULL)");

    if (!rd_type_is_equal(rd_file_kw_get_data_type(file_kw),
                          rd_kw_get_data_type(file_kw->kw.get())))
        throw std::runtime_error(std::string(__func__) +
                                 ": type mismatch between header and file.");

    if (file_kw->kw_size != rd_kw_get_size(file_kw->kw.get()))
        throw std::runtime_error(std::string(__func__) +
                                 ": size mismatch between header and file.");

    if (file_kw->header != rd_kw_get_header(file_kw->kw.get()))
        throw std::runtime_error(std::string(__func__) +
                                 ": name mismatch between header and file.");
}

static void rd_file_kw_drop_kw(rd_file_kw_type *file_kw,
                               inv_map_type *inv_map) {
    if (file_kw->kw) {
        inv_map_drop_kw(inv_map, file_kw->kw.get());
        file_kw->kw.reset(nullptr);
    }
}

static void rd_file_kw_load_kw(rd_file_kw_type *file_kw, ERT::FortIO &fortio,
                               inv_map_type *inv_map) {
    if (!fortio.assert_stream_open())
        throw std::ios_base::failure(
            std::string(__func__) +
            ": trying to load a keyword after the backing file has "
            "been detached.");

    if (file_kw->kw)
        rd_file_kw_drop_kw(file_kw, inv_map);

    {
        fortio.fseek(file_kw->file_offset, SEEK_SET);
        file_kw->kw.reset(rd_kw_fread_alloc(fortio));
        rd_file_kw_assert_kw(file_kw);
        inv_map_add_kw(inv_map, file_kw, file_kw->kw.get());
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
    return file_kw->kw.get();
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

rd_kw_type *rd_file_kw_get_kw(rd_file_kw_type *file_kw, ERT::FortIO &fortio,
                              inv_map_type *inv_map) {
    if (file_kw->ref_count == 0)
        rd_file_kw_load_kw(file_kw, fortio, inv_map);

    if (file_kw->kw)
        file_kw->ref_count++;

    return file_kw->kw.get();
}

const char *rd_file_kw_get_header(const rd_file_kw_type *file_kw) {
    return file_kw->header.c_str();
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
                           ERT::FortIO &fortio) {
    return rd_kw_fskip_data__(rd_file_kw_get_data_type(file_kw),
                              file_kw->kw_size, fortio);
}

/**
   This function will replace the file content of the keyword pointed
   to by @file_kw, with the new content given by @rd_kw. The new
   @rd_kw keyword must have identical header to the one already
   present in the file.
*/

void rd_file_kw_inplace_fwrite(rd_file_kw_type *file_kw, ERT::FortIO &fortio) {
    rd_file_kw_assert_kw(file_kw);
    fortio.fseek(file_kw->file_offset, SEEK_SET);
    rd_kw_fskip_header(fortio);
    fortio.fclean();
    rd_kw_fwrite_data(file_kw->kw.get(), fortio);
}

void rd_file_kw_fwrite(const rd_file_kw_type *file_kw, FILE *stream) {
    size_t header_length = file_kw->header.size();
    for (size_t i = 0; i < RD_STRING8_LENGTH; i++) {
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

std::vector<rd_file_kw_ptr> rd_file_kw_fread(FILE *stream, int num) {

    size_t file_kw_size = RD_STRING8_LENGTH + 2 * sizeof(int) +
                          sizeof(offset_type) + sizeof(size_t);
    size_t buffer_size = num * file_kw_size;
    std::unique_ptr<char, decltype(&std::free)> buffer(
        (char *)util_malloc(buffer_size), &std::free);
    size_t num_read = fread(buffer.get(), 1, buffer_size, stream);

    if (num_read != buffer_size) {
        throw std::runtime_error("error reading rd_file_type index file");
    }

    std::vector<rd_file_kw_ptr> kw_list(num);
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
                if (buffer.get()[index + buffer_offset] != ' ')
                    header[index] = buffer.get()[index + buffer_offset];
                else
                    break;

                index++;
                if (index == RD_STRING8_LENGTH)
                    break;
            }
            header[index] = '\0';
            buffer_offset += RD_STRING8_LENGTH;
        }

        memcpy(&kw_size, &buffer.get()[buffer_offset], sizeof kw_size);
        buffer_offset += sizeof kw_size;

        memcpy(&file_offset, &buffer.get()[buffer_offset], sizeof file_offset);
        buffer_offset += sizeof file_offset;

        memcpy(&rd_type, &buffer.get()[buffer_offset], sizeof rd_type);
        buffer_offset += sizeof rd_type;

        memcpy(&type_size, &buffer.get()[buffer_offset], sizeof type_size);
        buffer_offset += sizeof type_size;

        kw_list[ikw].reset(new rd_file_kw_struct(
            file_offset, rd_type_create(rd_type, type_size), kw_size, header));
    }
    return kw_list;
}

void rd_file_kw_start_transaction(const rd_file_kw_type *file_kw,
                                  int *ref_count) {
    *ref_count = file_kw->ref_count;
}

void rd_file_kw_end_transaction(rd_file_kw_type *file_kw, int ref_count) {
    if (ref_count == 0 && file_kw->ref_count > 0) {
        file_kw->kw.reset(nullptr);
    }
    file_kw->ref_count = ref_count;
}
