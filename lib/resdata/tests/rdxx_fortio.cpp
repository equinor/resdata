#include <stdexcept>
#include <fstream>

#include <ert/util/test_work_area.hpp>
#include <ert/util/test_util.hpp>
#include <resdata/ResdataKW.hpp>
#include <resdata/FortIO.hpp>

void test_open() {
    rd::util::TestArea ta("fortioxx");
    ERT::FortIO fortio;
    fortio.open("new_file", std::fstream::out);

    {
        std::vector<int> data;
        for (size_t i = 0; i < 1000; i++)
            data.push_back(i);

        fortio_fwrite_record(fortio.get(),
                             reinterpret_cast<char *>(data.data()), 1000 * 4);
    }
    fortio.close();

    fortio.open("new_file", std::fstream::app);
    {
        std::vector<int> data;
        for (size_t i = 0; i < 1000; i++)
            data.push_back(i);

        fortio_fwrite_record(fortio.get(),
                             reinterpret_cast<char *>(data.data()), 1000 * 4);
    }
    fortio.close();

    fortio.open("new_file", std::fstream::in);
    {
        std::vector<int> data;
        for (size_t i = 0; i < 1000; i++)
            data.push_back(99);

        test_assert_true(fortio_fread_buffer(
            fortio.get(), reinterpret_cast<char *>(data.data()), 1000 * 4));
        for (size_t i = 0; i < 1000; i++)
            test_assert_size_t_equal(data[i], i);

        test_assert_true(fortio_fread_buffer(
            fortio.get(), reinterpret_cast<char *>(data.data()), 1000 * 4));
        for (size_t i = 0; i < 1000; i++)
            test_assert_size_t_equal(data[i], i);
    }
    test_assert_false(fortio.ftruncate(0));
    fortio.close();
}

void test_fortio() {
    rd::util::TestArea ta("FORTIO");
    ERT::FortIO fortio("new_file", std::fstream::out);
    {
        std::vector<int> data;
        for (size_t i = 0; i < 1000; i++)
            data.push_back(i);

        fortio_fwrite_record(fortio.get(),
                             reinterpret_cast<char *>(data.data()), 1000 * 4);
    }
    fortio.close();

    fortio = ERT::FortIO("new_file", std::fstream::in);
    {
        std::vector<int> data;
        for (size_t i = 0; i < 1000; i++)
            data.push_back(99);

        test_assert_true(fortio_fread_buffer(
            fortio.get(), reinterpret_cast<char *>(data.data()), 1000 * 4));
        for (size_t i = 0; i < 1000; i++)
            test_assert_size_t_equal(data[i], i);
    }
    fortio.close();

    test_assert_throw(
        ERT::FortIO fortio("file/does/not/exists", std::fstream::in),
        std::invalid_argument);
}

void test_fortio_kw() {
    rd::util::TestArea ta("fortio_kw");
    std::vector<int> vec(1000);

    for (size_t i = 0; i < vec.size(); i++)
        vec[i] = i;

    ERT::ResdataKW<int> kw("XYZ", vec);

    {
        ERT::FortIO fortio("new_file", std::fstream::out);
        kw.fwrite(fortio);
        fortio.close();
    }

    {
        ERT::FortIO fortio("new_file", std::fstream::in);
        ERT::ResdataKW<int> kw2 = ERT::ResdataKW<int>::load(fortio);
        fortio.close();
        for (size_t i = 0; i < kw.size(); i++)
            test_assert_int_equal(kw.at(i), kw2.at(i));
    }
}

int main(int argc, char **argv) {
    test_open();
    test_fortio();
    test_fortio_kw();
}
