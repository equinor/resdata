/*
   Copyright (C) 2013  Equinor ASA, Norway.

   The file 'ert_util_rng.c' is part of ERT - Ensemble based Reservoir Tool.

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

#include <set>
#include <iostream>
#include <fstream>
#include <array>

#include <stdlib.h>
#include <stdbool.h>

#include <ert/util/test_util.hpp>
#include <ert/util/util.h>
#include <ert/util/rng.hpp>
#include <ert/util/mzran.hpp>

#define MAX_INT 666661

int main(int argc, char **argv) {
    rng_type *rng = rng_alloc(MZRAN, INIT_DEFAULT);
    {
        int val1 = rng_get_int(rng, MAX_INT);
        int val2 = rng_get_int(rng, MAX_INT);

        test_assert_int_not_equal(val1, val2);

        rng_init(rng, INIT_DEFAULT);
        val2 = rng_get_int(rng, MAX_INT);
        test_assert_int_equal(val1, val2);
    }
    {
        // Checking that no duplicates exist in the first 200 draws from the random number generator.
        // There is nothing special about the number 200, it was chosen by trial and error.
        // The rng is initialized with seed set to `INIT_DEFAULT`,
        // which makes it deterministic so this test should not fail.

        std::array<int, 200> random_array = {0};

        for (int &elem : random_array) {
            elem = rng_get_int(rng, MAX_INT);
        }

        // Constructing set to eliminate duplicates.
        std::set<int> random_set(random_array.begin(), random_array.end());

        int set_size = random_set.size();
        int array_size = random_array.size();

        test_assert_int_equal(set_size, array_size);
    }
    {
        int val2, val1;
        int state_size = rng_state_size(rng);
        char *buffer1 = (char *)util_calloc(state_size, sizeof *buffer1);
        char *buffer2 = (char *)util_calloc(state_size, sizeof *buffer2);
        test_assert_int_not_equal(state_size, 0);
        test_assert_int_equal(state_size, MZRAN_STATE_SIZE);

        rng_init(rng, INIT_DEFAULT);
        rng_get_state(rng, buffer1);
        val1 = rng_get_int(rng, MAX_INT);
        val2 = rng_get_int(rng, MAX_INT);

        test_assert_int_not_equal(val1, val2);
        rng_set_state(rng, buffer1);
        val2 = rng_get_int(rng, MAX_INT);
        test_assert_int_equal(val1, val2);

        rng_init(rng, INIT_DEFAULT);
        rng_get_state(rng, buffer2);
        test_assert_mem_equal(buffer1, buffer2, state_size);
        val2 = rng_get_int(rng, MAX_INT);
        rng_get_state(rng, buffer2);
        test_assert_mem_not_equal(buffer1, buffer2, state_size);

        free(buffer1);
        free(buffer2);
    }
    rng_free(rng);
    exit(0);
}
