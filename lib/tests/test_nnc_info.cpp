#include <catch2/catch.hpp>
#include <memory>
#include <resdata/nnc_info.hpp>
#include <resdata/nnc_vector.hpp>

TEST_CASE("Test NNC info structure", "[unittest]") {
    GIVEN("An nnc_info structure") {
        int lgr_nr = 0;
        auto nnc_info = nnc_info_alloc(lgr_nr);
        REQUIRE(nnc_info != nullptr);

        THEN("Initially it should be empty") {
            REQUIRE(nnc_info_get_lgr_nr(nnc_info) == lgr_nr);
            REQUIRE(nnc_info_get_size(nnc_info) == 0);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 0);
            REQUIRE(nnc_info_get_vector(nnc_info, lgr_nr) == nullptr);
            REQUIRE(!nnc_info_has_grid_index_list(nnc_info, lgr_nr));
        }

        THEN("Adding NNC connections updates the structure") {
            nnc_info_add_nnc(nnc_info, lgr_nr, 10, 0);
            nnc_info_add_nnc(nnc_info, lgr_nr, 20, 1);
            nnc_info_add_nnc(nnc_info, lgr_nr, 30, 2);

            REQUIRE(nnc_info_get_size(nnc_info) == 1);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 3);
            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr));

            auto nnc_vector = nnc_info_get_vector(nnc_info, lgr_nr);
            REQUIRE(nnc_vector != nullptr);
            REQUIRE(nnc_vector_get_size(nnc_vector) == 3);

            const auto &grid_indices =
                nnc_info_get_grid_index_list(nnc_info, lgr_nr);
            REQUIRE(grid_indices.size() == 3);
            REQUIRE(grid_indices[0] == 10);
            REQUIRE(grid_indices[1] == 20);
            REQUIRE(grid_indices[2] == 30);
        }

        THEN("Self vector accessor works correctly") {
            nnc_info_add_nnc(nnc_info, lgr_nr, 5, 10);

            auto self_vector = nnc_info_get_self_vector(nnc_info);
            REQUIRE(self_vector != nullptr);
            REQUIRE(nnc_vector_get_size(self_vector) == 1);

            const auto &self_grid_indices =
                nnc_info_get_self_grid_index_list(nnc_info);
            REQUIRE(self_grid_indices.size() == 1);
            REQUIRE(self_grid_indices[0] == 5);
        }

        THEN("Multiple LGR support works") {
            int lgr_nr_1 = 1;
            int lgr_nr_2 = 2;

            nnc_info_add_nnc(nnc_info, lgr_nr, 10, 0);
            nnc_info_add_nnc(nnc_info, lgr_nr_1, 20, 1);
            nnc_info_add_nnc(nnc_info, lgr_nr_2, 30, 2);

            REQUIRE(nnc_info_get_size(nnc_info) == 3);
            REQUIRE(nnc_info_get_total_size(nnc_info) == 3);

            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr));
            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr_1));
            REQUIRE(nnc_info_has_grid_index_list(nnc_info, lgr_nr_2));
            REQUIRE(!nnc_info_has_grid_index_list(nnc_info, 99));

            auto vec0 = nnc_info_get_vector(nnc_info, lgr_nr);
            auto vec1 = nnc_info_get_vector(nnc_info, lgr_nr_1);
            auto vec2 = nnc_info_get_vector(nnc_info, lgr_nr_2);

            REQUIRE(vec0 != nullptr);
            REQUIRE(vec1 != nullptr);
            REQUIRE(vec2 != nullptr);

            REQUIRE(nnc_vector_get_lgr_nr(vec0) == lgr_nr);
            REQUIRE(nnc_vector_get_lgr_nr(vec1) == lgr_nr_1);
            REQUIRE(nnc_vector_get_lgr_nr(vec2) == lgr_nr_2);

            auto vec_idx0 = nnc_info_iget_vector(nnc_info, 0);
            auto vec_idx1 = nnc_info_iget_vector(nnc_info, 1);
            auto vec_idx2 = nnc_info_iget_vector(nnc_info, 2);

            REQUIRE(vec_idx0 != nullptr);
            REQUIRE(vec_idx1 != nullptr);
            REQUIRE(vec_idx2 != nullptr);
        }

        nnc_info_free(nnc_info);
    }
}

TEST_CASE("NNC Vector", "[unittest]") {
    auto nnc_vec = nnc_vector_alloc(0);
    REQUIRE(nnc_vec != nullptr);
    REQUIRE(nnc_vector_get_lgr_nr(nnc_vec) == 0);
    REQUIRE(nnc_vector_get_size(nnc_vec) == 0);

    nnc_vector_add_nnc(nnc_vec, 10, 100);
    nnc_vector_add_nnc(nnc_vec, 20, 200);
    nnc_vector_add_nnc(nnc_vec, 30, 300);

    REQUIRE(nnc_vector_get_size(nnc_vec) == 3);
    REQUIRE(nnc_vector_iget_grid_index(nnc_vec, 0) == 10);
    REQUIRE(nnc_vector_iget_nnc_index(nnc_vec, 0) == 100);
    REQUIRE(nnc_vector_iget_grid_index(nnc_vec, 1) == 20);
    REQUIRE(nnc_vector_iget_nnc_index(nnc_vec, 1) == 200);
    REQUIRE(nnc_vector_iget_grid_index(nnc_vec, 2) == 30);
    REQUIRE(nnc_vector_iget_nnc_index(nnc_vec, 2) == 300);

    auto nnc_vec_copy = nnc_vector_alloc_copy(nnc_vec);
    REQUIRE(nnc_vec_copy != nullptr);
    REQUIRE(nnc_vector_equal(nnc_vec, nnc_vec_copy));
    REQUIRE(nnc_vector_get_size(nnc_vec_copy) == 3);

    const auto &grid_indices = nnc_vector_get_grid_index_list(nnc_vec_copy);
    const auto &nnc_indices = nnc_vector_get_nnc_index_list(nnc_vec_copy);
    REQUIRE(grid_indices.size() == 3);
    REQUIRE(nnc_indices.size() == 3);
    REQUIRE(grid_indices[0] == 10);
    REQUIRE(grid_indices[1] == 20);
    REQUIRE(grid_indices[2] == 30);
    REQUIRE(nnc_indices[0] == 100);
    REQUIRE(nnc_indices[1] == 200);
    REQUIRE(nnc_indices[2] == 300);

    REQUIRE(nnc_vector_equal(nnc_vec, nnc_vec));
    REQUIRE(!nnc_vector_equal(nnc_vec, nullptr));
    REQUIRE(!nnc_vector_equal(nullptr, nnc_vec_copy));

    auto nnc_vec_diff = nnc_vector_alloc(1); // Different lgr_nr
    nnc_vector_add_nnc(nnc_vec_diff, 10, 100);
    REQUIRE(!nnc_vector_equal(nnc_vec, nnc_vec_diff));

    nnc_vector_free(nnc_vec);
    nnc_vector_free(nnc_vec_copy);
    nnc_vector_free(nnc_vec_diff);
}
