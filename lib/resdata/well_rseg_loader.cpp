#include <cstdlib>
#include <memory>
#include <vector>

#include <resdata/rd_file_view.hpp>
#include <resdata/rd_kw_magic.hpp>

#include <resdata/well/well_const.hpp>
#include <resdata/well/well_rseg_loader.hpp>
#include <resdata/rd_file_flag.hpp>
#include <ert/util/util.hpp>

struct well_rseg_loader_struct {
    rd::FileView *rst_view;
    std::vector<size_t> relative_index_map;
    std::vector<size_t> absolute_index_map;
    double buffer[4];
    const char *kw;
};

well_rseg_loader_type *well_rseg_loader_alloc(rd::FileView *rst_view) {
    auto loader = std::make_unique<well_rseg_loader_struct>();

    loader->rst_view = rst_view;
    loader->kw = RSEG_KW;

    loader->relative_index_map.push_back(RSEG_DEPTH_INDEX);
    loader->relative_index_map.push_back(RSEG_LENGTH_INDEX);
    loader->relative_index_map.push_back(RSEG_TOTAL_LENGTH_INDEX);
    loader->relative_index_map.push_back(RSEG_DIAMETER_INDEX);

    return loader.release();
}

void well_rseg_loader_free(well_rseg_loader_type *loader) {
    if (loader->rst_view->has_flags(FileMode::CLOSE_STREAM))
        loader->rst_view->close();
    delete loader;
}

double *well_rseg_loader_load_values(well_rseg_loader_type *loader,
                                     size_t rseg_offset) {

    loader->absolute_index_map.resize(loader->relative_index_map.size(), 0);
    for (size_t index = 0; index < loader->relative_index_map.size(); index++)
        loader->absolute_index_map[index] =
            loader->relative_index_map.at(index) + rseg_offset;

    loader->rst_view->index_fload_kw(loader->kw, 0, loader->absolute_index_map,
                                     (char *)loader->buffer);

    return loader->buffer;
}
