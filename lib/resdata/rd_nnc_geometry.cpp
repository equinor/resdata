#include <vector>
#include <algorithm>

#include <resdata/rd_nnc_geometry.hpp>

#define RD_NNC_GEOMETRY_TYPE_ID 6124343

struct rd_nnc_geometry_struct {
    UTIL_TYPE_ID_DECLARATION;
    std::vector<rd_nnc_pair_struct> *data;
};

UTIL_IS_INSTANCE_FUNCTION(rd_nnc_geometry, RD_NNC_GEOMETRY_TYPE_ID)

int rd_nnc_geometry_size(const rd_nnc_geometry_type *nnc_geo) {
    return nnc_geo->data->size();
}

/*
  Will go through the grid and add links for all NNC connections in
  the grid. The endpoints of an NNC are defined by the tuple:

     (grid_nr, global_index),

  and a NNC link is defined by a pair of such connections, linking
  cells (grid_nr1, global_index1) and (grid_nr2, global_index2).
*/

static void rd_nnc_geometry_add_pairs(const rd_nnc_geometry_type *nnc_geo,
                                      const rd_grid_type *grid) {
    int lgr_nr1 = rd_grid_get_lgr_nr(grid);
    const rd_grid_type *global_grid = rd_grid_get_global_grid(grid);

    if (!global_grid)
        global_grid = grid;

    for (int global_index1 = 0; global_index1 < rd_grid_get_global_size(grid);
         global_index1++) {
        const nnc_info_type *nnc_info =
            rd_grid_get_cell_nnc_info1(grid, global_index1);
        if (!nnc_info)
            continue;

        for (int lgr_index2 = 0; lgr_index2 < nnc_info_get_size(nnc_info);
             lgr_index2++) {
            const nnc_vector_type *nnc_vector =
                nnc_info_iget_vector(nnc_info, lgr_index2);
            const std::vector<int> &grid2_index_list =
                nnc_vector_get_grid_index_list(nnc_vector);
            const std::vector<int> &nnc_index_list =
                nnc_vector_get_nnc_index_list(nnc_vector);
            int lgr_nr2 = nnc_vector_get_lgr_nr(nnc_vector);

            for (int index2 = 0; index2 < nnc_vector_get_size(nnc_vector);
                 index2++) {
                rd_nnc_pair_type pair;
                pair.grid_nr1 = lgr_nr1;
                pair.global_index1 = global_index1;
                pair.grid_nr2 = lgr_nr2;
                pair.global_index2 = grid2_index_list[index2];
                pair.input_index = nnc_index_list[index2];
                nnc_geo->data->push_back(pair);
            }
        }
    }
}

static bool rd_nnc_cmp(const rd_nnc_pair_type &nnc1,
                       const rd_nnc_pair_type &nnc2) {
    if (nnc1.grid_nr1 != nnc2.grid_nr1)
        return nnc1.grid_nr1 < nnc2.grid_nr1;

    if (nnc1.grid_nr2 != nnc2.grid_nr2)
        return nnc1.grid_nr2 < nnc2.grid_nr2;

    if (nnc1.global_index1 != nnc2.global_index1)
        return nnc1.global_index1 < nnc2.global_index1;

    if (nnc1.global_index2 != nnc2.global_index2)
        return nnc1.global_index2 < nnc2.global_index2;

    return false;
}

rd_nnc_geometry_type *rd_nnc_geometry_alloc(const rd_grid_type *grid) {
    rd_nnc_geometry_type *nnc_geo =
        (rd_nnc_geometry_type *)util_malloc(sizeof *nnc_geo);
    UTIL_TYPE_ID_INIT(nnc_geo, RD_NNC_GEOMETRY_TYPE_ID);
    nnc_geo->data = new std::vector<rd_nnc_pair_type>();

    rd_nnc_geometry_add_pairs(nnc_geo, grid);
    for (int lgr_index = 0; lgr_index < rd_grid_get_num_lgr(grid);
         lgr_index++) {
        rd_grid_type *igrid = rd_grid_iget_lgr(grid, lgr_index);
        rd_nnc_geometry_add_pairs(nnc_geo, igrid);
    }
    std::sort(nnc_geo->data->begin(), nnc_geo->data->end(), rd_nnc_cmp);
    return nnc_geo;
}

void rd_nnc_geometry_free(rd_nnc_geometry_type *nnc_geo) {
    delete nnc_geo->data;
    free(nnc_geo);
}

const rd_nnc_pair_type *
rd_nnc_geometry_iget(const rd_nnc_geometry_type *nnc_geo, int index) {
    const std::vector<rd_nnc_pair_type> &nnc_data = *nnc_geo->data;
    return &nnc_data[index];
}

bool rd_nnc_geometry_same_kw(const rd_nnc_pair_type *nnc1,
                             const rd_nnc_pair_type *nnc2) {
    if ((nnc1->grid_nr1 == nnc2->grid_nr1) &&
        (nnc1->grid_nr2 == nnc2->grid_nr2))
        return true;
    else
        return false;
}
