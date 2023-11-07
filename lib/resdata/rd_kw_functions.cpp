/*
  This file is included from the rd_kw.c file during compilation. It
  contains functions which are not really related to the rd_kw as a
  datastructure, but rather use an rd_kw instance in a function.
*/

/*
  This is an extremely special-case function written for the region
  creation code. Given a completed rd_kw regions keyword, the purpose
  of this function is to "detect and correct" uninitialized cells with
  value 0. This function is purely heuristic:

   1. It only considers cells which are active in the grid, i.e. where
      actnum[] != 0.

   2. It will scan the four neighbours in the xy plane, if all
      neighbours agree on region value this value will be applied;
      otherwise the value will not be changed. Neighbouring cells with
      value zero are not considered when comparing.
*/

void rd_kw_fix_uninitialized(rd_kw_type *rd_kw, int nx, int ny, int nz,
                             const int *actnum) {
    int i, j, k;
    int *data = (int *)rd_kw_get_ptr(rd_kw);

    int_vector_type *undetermined1 = int_vector_alloc(0, 0);
    int_vector_type *undetermined2 = int_vector_alloc(0, 0);

    for (k = 0; k < nz; k++) {
        int_vector_reset(undetermined1);
        for (j = 0; j < ny; j++) {
            for (i = 0; i < nx; i++) {
                int g0 = i + j * nx + k * nx * ny;

                if (data[g0] == 0 && actnum[g0])
                    int_vector_append(undetermined1, g0);
            }
        }

        while (true) {
            int index;
            bool finished = true;

            int_vector_reset(undetermined2);
            for (index = 0; index < int_vector_size(undetermined1); index++) {
                int g0 = int_vector_iget(undetermined1, index);
                int j = (g0 - k * nx * ny) / nx;
                int i = g0 - k * nx * ny - j * nx;

                if (data[g0] == 0 && actnum[g0]) {
                    int n1 = 0;
                    int n2 = 0;
                    int n3 = 0;
                    int n4 = 0;

                    if (i > 0) {
                        int g1 = g0 - 1;
                        if (actnum[g1])
                            n1 = data[g1];
                    }

                    if (i < (nx - 1)) {
                        int g2 = g0 + 1;
                        if (actnum[g2])
                            n2 = data[g2];
                    }

                    if (j > 0) {
                        int g3 = g0 - nx;
                        if (actnum[g3])
                            n3 = data[g3];
                    }

                    if (j < (ny - 1)) {
                        int g4 = g0 + nx;
                        if (actnum[g4])
                            n4 = data[g4];
                    }

                    {
                        int new_value = 0;

                        if (n1)
                            new_value = n1;

                        if (n2) {
                            if (new_value == 0)
                                new_value = n2;
                            else if (new_value != n2)
                                new_value = -1;
                        }

                        if (n3) {
                            if (new_value == 0)
                                new_value = n3;
                            else if (new_value != n3)
                                new_value = -1;
                        }

                        if (n4) {
                            if (new_value == 0)
                                new_value = n4;
                            else if (new_value != n4)
                                new_value = -1;
                        }

                        if (new_value > 0) {
                            data[g0] = new_value;
                            finished = false;
                        }
                    }
                    if ((n1 + n2 + n3 + n4) == 0)
                        int_vector_append(undetermined2, g0);
                }
            }
            {
                int_vector_type *tmp = undetermined2;
                undetermined2 = undetermined1;
                undetermined1 = tmp;
            }
            if (finished || (int_vector_size(undetermined1) == 0))
                break;
        }
    }
    int_vector_free(undetermined1);
    int_vector_free(undetermined2);
}

rd_kw_type *rd_kw_alloc_actnum(const rd_kw_type *porv_kw, float porv_limit) {
    if (!rd_type_is_float(porv_kw->data_type))
        return NULL;

    if (!util_string_equal(PORV_KW, rd_kw_get_header(porv_kw)))
        return NULL;

    const int size = rd_kw_get_size(porv_kw);
    rd_kw_type *actnum_kw = rd_kw_alloc(ACTNUM_KW, size, RD_INT);
    const float *porv_values = rd_kw_get_float_ptr(porv_kw);
    int *actnum_values = rd_kw_get_int_ptr(actnum_kw);

    for (int i = 0; i < size; i++) {
        if (porv_values[i] > porv_limit)
            actnum_values[i] = 1;
        else
            actnum_values[i] = 0;
    }

    return actnum_kw;
}

/*
    Allocate actnum, and assign actnum_bitmask to all cells with pore volume
    larger than zero. The bit mask can be any combination of
    CELL_ACTIVE_MATRIX and CELL_ACTIVE_FRACTURE.
    See documentation in top of rd_grid.cpp
*/
rd_kw_type *rd_kw_alloc_actnum_bitmask(const rd_kw_type *porv_kw,
                                       float porv_limit, int actnum_bitmask) {
    if (!rd_type_is_float(porv_kw->data_type))
        return NULL;

    if (!util_string_equal(PORV_KW, rd_kw_get_header(porv_kw)))
        return NULL;

    const int size = rd_kw_get_size(porv_kw);
    rd_kw_type *actnum_kw = rd_kw_alloc(ACTNUM_KW, size, RD_INT);
    const float *porv_values = rd_kw_get_float_ptr(porv_kw);
    int *actnum_values = rd_kw_get_int_ptr(actnum_kw);

    for (int i = 0; i < size; i++) {
        if (porv_values[i] > porv_limit)
            actnum_values[i] = actnum_bitmask;
        else
            actnum_values[i] = 0;
    }

    return actnum_kw;
}
