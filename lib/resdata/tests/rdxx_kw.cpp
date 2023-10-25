#include <stdexcept>
#include <fstream>

#include <ert/util/test_util.hpp>

#include <resdata/rd_file.hpp>

#include <resdata/ResdataKW.hpp>
#include <resdata/FortIO.hpp>
#include <ert/util/test_work_area.hpp>

void test_kw_name() {
    ERT::ResdataKW<int> kw1("short", 1);
    ERT::ResdataKW<int> kw2("verylong", 1);

    test_assert_string_equal(kw1.name(), "short");
    test_assert_string_equal(kw2.name(), "verylong");
}

void test_kw_vector_assign() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    ERT::ResdataKW<int> kw("XYZ", vec);

    test_assert_size_t_equal(kw.size(), vec.size());

    for (size_t i = 0; i < kw.size(); ++i) {
        test_assert_int_equal(kw.at(i), vec[i]);
        test_assert_int_equal(kw[i], vec[i]);
    }

    for (size_t i = 0; i < kw.size(); ++i) {
        kw[i] *= 2;
        test_assert_int_equal(kw[i], 2 * vec[i]);
    }
}

void test_kw_vector_string() {
    std::vector<const char *> vec = {"short", "sweet", "padded  "};

    std::vector<const char *> too_long = {"1234567890"};
    ERT::ResdataKW<const char *> kw("XYZ", vec);

    test_assert_size_t_equal(kw.size(), vec.size());

    test_assert_string_equal(kw.at(0), "short   ");
    test_assert_string_equal(kw.at(1), "sweet   ");
    test_assert_string_equal(kw.at(2), vec.at(2));

    test_assert_throw(ERT::ResdataKW<const char *>("XY", too_long),
                      std::range_error);
}

void test_kw_vector_std_string() {
    std::vector<const char *> vec = {
        "short",
        "sweet",
        "padded  ",
    };
    std::vector<std::string> too_long = {"1234567890"};
    ERT::ResdataKW<std::string> kw("XYZ", vec);

    test_assert_size_t_equal(kw.size(), vec.size());

    test_assert_string_equal(kw.at(0).c_str(), "short   ");
    test_assert_string_equal(kw.at(1).c_str(), "sweet   ");
    test_assert_string_equal(kw.at(2).c_str(), vec.at(2));

    test_assert_throw(ERT::ResdataKW<std::string>("XY", too_long),
                      std::range_error);
}

void test_logical() {
    //std::vector<bool> vec = {true,false,true,false};
    //  ERT::ResdataKW<bool> kw("BOOL", vec);
    //  test_assert_int_equal(kw.size(), vec.size());

    // for (size_t i=0; i < vec.size(); i++)
    //       test_assert_true( kw.at(i) == vec[i] );
}

void test_move_semantics_no_crash() {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    ERT::ResdataKW<int> kw1("XYZ", vec);

    ERT::ResdataKW<int> kw2(std::move(kw1));
    test_assert_true(kw1.get() == nullptr);
}

void test_exception_assing_ref_wrong_type() {
    auto *ptr = rd_kw_alloc("XYZ", 1, RD_INT);

    try {
        ERT::ResdataKW<double> kw(ptr);
        test_assert_true(false);
    } catch (...) {
        ERT::ResdataKW<int> kw(ptr);
    }
}

void test_resize() {
    ERT::ResdataKW<int> kw1("short", 1);

    test_assert_int_equal(kw1.size(), 1);
    kw1.resize(100);
    test_assert_int_equal(kw1.size(), 100);
}

void test_data() {
    std::vector<double> d_data = {1, 2, 3, 4};
    std::vector<float> f_data = {10, 20, 30, 40};
    std::vector<int> i_data = {100, 200, 300, 400};
    std::vector<bool> b_data = {true, false};
    std::vector<std::string> s_data = {"S1", "S2", "S3"};

    ERT::ResdataKW<double> d_kw("DOUBLE", d_data);
    auto d_data2 = d_kw.data();
    for (size_t i = 0; i < d_data.size(); i++)
        test_assert_true(d_data[i] == d_data2[i]);

    ERT::ResdataKW<float> f_kw("FLOATx", f_data);
    auto f_data2 = f_kw.data();
    for (size_t i = 0; i < f_data.size(); i++)
        test_assert_true(f_data[i] == f_data2[i]);

    ERT::ResdataKW<int> i_kw("INT", i_data);
    auto i_data2 = i_kw.data();
    for (size_t i = 0; i < i_data.size(); i++)
        test_assert_true(i_data[i] == i_data2[i]);

    //ERT::ResdataKW<bool> b_kw("bbb", b_data);
    //auto b_data2 = b_kw.data();
    //for (size_t i=0; i < b_data.size(); i++)
    //  test_assert_true(b_data[i] == b_data2[i]);

    ERT::ResdataKW<std::string> s_kw("sss", s_data);
    auto s_data2 = s_kw.data();
    for (size_t i = 0; i < s_data.size(); i++)
        test_assert_true(s_data[i] == s_data2[i]);
}

void test_read_write() {
    std::vector<double> d_data = {1, 2, 3, 4};
    std::vector<float> f_data = {10, 20, 30, 40};
    std::vector<int> i_data = {100, 200, 300, 400};
    std::vector<bool> b_data = {true, false};
    std::vector<std::string> s_data = {"S1", "S2", "S3"};

    {
        rd::util::TestArea ta("kw-read-write");
        {
            ERT::FortIO f("test_file", std::ios_base::out);
            ERT::write_kw(f, "DOUBLE", d_data);
            ERT::write_kw(f, "FLOAT", f_data);
            ERT::write_kw(f, "INT", i_data);
            ERT::write_kw(f, "BOOL", b_data);
            ERT::write_kw(f, "STRING", s_data);
        }

        {
            rd_file_type *f = rd_file_open("test_file", 0);
            rd_kw_type *d_kw = rd_file_iget_named_kw(f, "DOUBLE", 0);
            rd_kw_type *f_kw = rd_file_iget_named_kw(f, "FLOAT", 0);
            rd_kw_type *i_kw = rd_file_iget_named_kw(f, "INT", 0);
            rd_kw_type *b_kw = rd_file_iget_named_kw(f, "BOOL", 0);
            rd_kw_type *s_kw = rd_file_iget_named_kw(f, "STRING", 0);

            for (size_t i = 0; i < d_data.size(); i++)
                test_assert_true(d_data[i] == rd_kw_iget_double(d_kw, i));

            for (size_t i = 0; i < f_data.size(); i++)
                test_assert_true(f_data[i] == rd_kw_iget_float(f_kw, i));

            for (size_t i = 0; i < i_data.size(); i++)
                test_assert_true(i_data[i] == rd_kw_iget_int(i_kw, i));

            for (size_t i = 0; i < b_data.size(); i++)
                test_assert_true(b_data[i] == rd_kw_iget_bool(b_kw, i));

            for (size_t i = 0; i < s_data.size(); i++) {
                std::string s8 = rd_kw_iget_char_ptr(s_kw, i);
                test_assert_int_equal(s8.size(), 8);
                s8.erase(s8.find_last_not_of(' ') + 1);
                test_assert_true(s_data[i] == s8);
            }

            rd_file_close(f);
        }
    }
}

int main(int argc, char **argv) {
    test_kw_name();
    test_kw_vector_assign();
    test_kw_vector_string();
    test_kw_vector_std_string();
    test_logical();
    test_move_semantics_no_crash();
    test_exception_assing_ref_wrong_type();
    test_resize();
    test_data();
    test_read_write();
}
