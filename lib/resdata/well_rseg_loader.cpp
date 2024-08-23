#include <stdbool.h>

#include <ert/util/util.hpp>
#include <ert/util/int_vector.hpp>

#include <resdata/rd_file.hpp>
#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw_magic.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_rseg_loader.hpp>

#include <resdata/fortio.h>

struct well_rseg_loader_struct {
    rd_file_view_type *rst_view;
    int_vector_type *relative_index_map;
    int_vector_type *absolute_index_map;
    char *buffer;
    const char *kw;
};

well_rseg_loader_type *well_rseg_loader_alloc(rd_file_view_type *rst_view) {
    well_rseg_loader_type *loader =
        (well_rseg_loader_type *)util_malloc(sizeof *loader);

    int element_count = 4;

    loader->rst_view = rst_view;
    loader->relative_index_map = int_vector_alloc(0, 0);
    loader->absolute_index_map = int_vector_alloc(0, 0);
    loader->buffer = (char *)util_malloc(element_count * sizeof(double));
    loader->kw = RSEG_KW;

    int_vector_append(loader->relative_index_map, RSEG_DEPTH_INDEX);
    int_vector_append(loader->relative_index_map, RSEG_LENGTH_INDEX);
    int_vector_append(loader->relative_index_map, RSEG_TOTAL_LENGTH_INDEX);
    int_vector_append(loader->relative_index_map, RSEG_DIAMETER_INDEX);

    return loader;
}

void well_rseg_loader_free(well_rseg_loader_type *loader) {

    if (rd_file_view_flags_set(loader->rst_view, RD_FILE_CLOSE_STREAM))
        rd_file_view_fclose_stream(loader->rst_view);

    int_vector_free(loader->relative_index_map);
    int_vector_free(loader->absolute_index_map);
    free(loader->buffer);
    free(loader);
}

double *well_rseg_loader_load_values(const well_rseg_loader_type *loader,
                                     int rseg_offset) {
    int_vector_type *index_map = loader->absolute_index_map;

    int index = 0;
    int_vector_resize(index_map, int_vector_size(loader->relative_index_map),
                      0);
    for (index = 0; index < int_vector_size(loader->relative_index_map);
         index++) {
        int relative_index = int_vector_iget(loader->relative_index_map, index);
        int_vector_iset(index_map, index, relative_index + rseg_offset);
    }

    rd_file_view_index_fload_kw(loader->rst_view, loader->kw, 0, index_map,
                                loader->buffer);

    return (double *)loader->buffer;
}
