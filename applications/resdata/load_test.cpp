/*
   Copyright (C) 2011  Equinor ASA, Norway.

   The file 'load_test.cpp' is part of ERT - Ensemble based Reservoir Tool.

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

#include <cstdlib>
#include <cstdio>

#include <resdata/rd_grid.hpp>
#include <resdata/rd_file.hpp>
#include <filesystem>

namespace fs = std::filesystem;

void test_case(const std::string &base, bool load_all) {
    fs::path grid_file = rd::filename(base, RD_EGRID_FILE, false, 0);
    std::string init_file = rd::filename(base, RD_INIT_FILE, false, 0);
    std::string restart_file =
        rd::filename(base, RD_UNIFIED_RESTART_FILE, false, 0);

    clock_t begin = clock();
    rd_grid_ptr grid = read_grid(grid_file);
    clock_t end = clock();
    double grid_time = (double)(end - begin) / CLOCKS_PER_SEC;

    begin = clock();
    rd_file_ptr init(rd_file_open(init_file.c_str(), 0), &rd_file_close);
    if (load_all)
        rd_file_load_all(init.get());

    end = clock();
    double init_time = (double)(end - begin) / CLOCKS_PER_SEC;

    begin = clock();
    rd_file_ptr restart(rd_file_open(restart_file.c_str(), 0), &rd_file_close);
    if (load_all)
        rd_file_load_all(restart.get());
    end = clock();
    double restarts_time = (double)(end - begin) / CLOCKS_PER_SEC;

    printf("%-64s  Restart:%8.4f    Grid:%8.4f     Init:%8.4f \n", base.c_str(),
           restarts_time, grid_time, init_time);
}

int main(int argc, char **argv) {
    fprintf(stderr, "** Warning: load_test.x is deprecated\n");
    bool load_all = true;
    for (int i = 1; i < argc; i++)
        test_case(std::string(argv[i]), load_all);
}
