#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <fstream>
#include <ios>
#include <memory>
#include <stdexcept>

#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>

#include "resdata/rd_type.hpp"
#include "tmpdir.hpp"

SCENARIO("A FileKW is constructed from explicit header information") {
    GIVEN("A FileKW created with an offset, type, size and header") {
        FileKW file_kw(128, RD_INT, 10, "TEST_KW");

        THEN("The accessors reflect the construction arguments") {
            REQUIRE(file_kw.get_offset() == 128);
            REQUIRE(file_kw.get_size() == 10);
            REQUIRE(file_kw.get_header() == "TEST_KW");
            REQUIRE(rd_type_is_equal(file_kw.get_data_type(), RD_INT));
        }

        THEN("No keyword has been loaded yet") {
            REQUIRE(file_kw.get_kw_ptr() == nullptr);
        }
    }
}

SCENARIO("A FileKW is constructed from an rd_kw instance") {
    GIVEN("An rd_kw with a known header, size and type") {
        auto kw = make_rd_kw("PORO", 5, RD_FLOAT);

        WHEN("A FileKW is created from it with an offset") {
            FileKW file_kw(kw.get(), 256);

            THEN("The header information is derived from the rd_kw") {
                REQUIRE(file_kw.get_offset() == 256);
                REQUIRE(file_kw.get_size() == 5);
                REQUIRE(file_kw.get_header() == "PORO");
                REQUIRE(rd_type_is_equal(file_kw.get_data_type(), RD_FLOAT));
            }
        }
    }
}

SCENARIO("Two FileKW instances are compared for equality") {
    GIVEN("A reference FileKW") {
        FileKW reference(64, RD_INT, 3, "KW");

        THEN("An identical FileKW compares equal") {
            FileKW same(64, RD_INT, 3, "KW");
            REQUIRE(reference == same);
        }

        THEN("A different offset compares unequal") {
            FileKW other(65, RD_INT, 3, "KW");
            REQUIRE_FALSE(reference == other);
        }

        THEN("A different size compares unequal") {
            FileKW other(64, RD_INT, 4, "KW");
            REQUIRE_FALSE(reference == other);
        }

        THEN("A different data type compares unequal") {
            FileKW other(64, RD_FLOAT, 3, "KW");
            REQUIRE_FALSE(reference == other);
        }

        THEN("A different header compares unequal") {
            FileKW other(64, RD_INT, 3, "OTHER");
            REQUIRE_FALSE(reference == other);
        }
    }
}

SCENARIO_METHOD(Tmpdir, "A FileKW index entry is written and read back") {
    GIVEN("A FileKW written to a plain stream") {
        FileKW original(4096, RD_INT, 7, "INDEXKW");

        auto filename = (dirname / "index").string();
        {
            FILE *stream = std::fopen(filename.c_str(), "wb");
            REQUIRE(stream != nullptr);
            original.write_header(stream);
            std::fclose(stream);
        }

        WHEN("The entry is read back from the stream") {
            FILE *stream = std::fopen(filename.c_str(), "rb");
            REQUIRE(stream != nullptr);
            auto kw_list = FileKW::read(stream, 1);
            std::fclose(stream);

            THEN("A single FileKW equal to the original is recovered") {
                REQUIRE(kw_list.size() == 1);
                REQUIRE(*kw_list[0] == original);
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "Several FileKW index entries are written and read") {
    GIVEN("Three FileKW instances written to the same stream") {
        FileKW first(66, RD_FLOAT, 1000, "PRESSURE");
        FileKW second(1066, RD_FLOAT, 2000, "TEST1_KW");
        FileKW third(2066, RD_DOUBLE, 3000, "TEST2_KW");

        auto filename = (dirname / "index").string();
        {
            FILE *stream = std::fopen(filename.c_str(), "wb");
            REQUIRE(stream != nullptr);
            first.write_header(stream);
            second.write_header(stream);
            third.write_header(stream);
            std::fclose(stream);
        }

        WHEN("All three entries are read back") {
            FILE *stream = std::fopen(filename.c_str(), "rb");
            REQUIRE(stream != nullptr);
            auto kw_list = FileKW::read(stream, 3);
            std::fclose(stream);

            THEN("Each recovered FileKW equals its original in order") {
                REQUIRE(kw_list.size() == 3);
                REQUIRE(*kw_list[0] == first);
                REQUIRE(*kw_list[1] == second);
                REQUIRE(*kw_list[2] == third);
            }
        }

        WHEN("More entries are requested than are present") {
            FILE *stream = std::fopen(filename.c_str(), "rb");
            REQUIRE(stream != nullptr);

            THEN("read throws a runtime_error") {
                REQUIRE_THROWS_AS(FileKW::read(stream, 10), std::runtime_error);
            }
            std::fclose(stream);
        }
    }
}

SCENARIO_METHOD(Tmpdir, "A full width eight character header round-trips") {
    GIVEN("A FileKW whose header fills all RD_STRING8_LENGTH bytes") {
        FileKW original(4096, RD_DOUBLE, 42, "EIGHTLEN");
        REQUIRE(original.get_header().size() == RD_STRING8_LENGTH);

        auto filename = (dirname / "index").string();
        {
            FILE *stream = std::fopen(filename.c_str(), "wb");
            REQUIRE(stream != nullptr);
            original.write_header(stream);
            std::fclose(stream);
        }

        WHEN("The entry is read back") {
            FILE *stream = std::fopen(filename.c_str(), "rb");
            REQUIRE(stream != nullptr);
            auto kw_list = FileKW::read(stream, 1);
            std::fclose(stream);

            THEN("The full eight character header is recovered intact") {
                REQUIRE(kw_list.size() == 1);
                REQUIRE(kw_list[0]->get_header() == "EIGHTLEN");
                REQUIRE(*kw_list[0] == original);
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "Reading zero FileKW entries yields an empty list") {
    GIVEN("An open but empty stream") {
        auto filename = (dirname / "empty").string();
        {
            FILE *stream = std::fopen(filename.c_str(), "wb");
            REQUIRE(stream != nullptr);
            std::fclose(stream);
        }

        WHEN("read is asked for zero entries") {
            FILE *stream = std::fopen(filename.c_str(), "rb");
            REQUIRE(stream != nullptr);
            auto kw_list = FileKW::read(stream, 0);
            std::fclose(stream);

            THEN("An empty list is returned without error") {
                REQUIRE(kw_list.empty());
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "Reading a FileKW past the end of the stream fails") {
    GIVEN("A stream containing a single FileKW entry") {
        FileKW original(66, RD_FLOAT, 1000, "PRESSURE");

        auto filename = (dirname / "index").string();
        {
            FILE *stream = std::fopen(filename.c_str(), "wb");
            REQUIRE(stream != nullptr);
            original.write_header(stream);
            std::fclose(stream);
        }

        WHEN("The single entry has already been consumed") {
            FILE *stream = std::fopen(filename.c_str(), "rb");
            REQUIRE(stream != nullptr);
            auto disk_kw = FileKW::read(stream, 1);
            REQUIRE(*disk_kw[0] == original);

            THEN("Reading another entry throws a runtime_error") {
                REQUIRE_THROWS_AS(FileKW::read(stream, 1), std::runtime_error);
            }
            std::fclose(stream);
        }
    }
}

SCENARIO_METHOD(Tmpdir, "A FileKW lazily loads its keyword from file") {
    GIVEN("A keyword written to a fortran formatted file") {
        auto filename = (dirname / "DATA").string();

        auto kw = make_rd_kw("MYKW", 4, RD_INT);
        for (int i = 0; i < 4; i++)
            rd_kw_iset_int(kw.get(), i, i * 10);

        offset_type offset;
        {
            ERT::FortIO fortio(filename, std::ios_base::out);
            offset = fortio.ftell();
            rd_kw_fwrite(kw.get(), fortio);
        }

        FileKW file_kw(kw.get(), offset);

        THEN("The keyword is not loaded until requested") {
            REQUIRE(file_kw.get_kw_ptr() == nullptr);
        }

        WHEN("get_kw is called with a reading fortio handle") {
            ERT::FortIO fortio(filename, std::ios_base::in);
            rd_kw_type *loaded = file_kw.get_kw(fortio);

            THEN("The keyword is loaded and cached") {
                REQUIRE(loaded != nullptr);
                REQUIRE(file_kw.get_kw_ptr() == loaded);
                REQUIRE(rd_kw_get_size(loaded) == 4);
                for (int i = 0; i < 4; i++)
                    REQUIRE(rd_kw_iget_int(loaded, i) == i * 10);
            }

            AND_WHEN("clear is called") {
                file_kw.clear();

                THEN("The cached keyword is dropped") {
                    REQUIRE(file_kw.get_kw_ptr() == nullptr);
                }
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "An unloaded FileKW cannot be written back in place") {
    GIVEN("A FileKW whose keyword has not been loaded") {
        FileKW file_kw(0, RD_INT, 10, "TEST_KW");

        auto filename = (dirname / "dummy").string();
        {
            std::ofstream ofs(filename);
            REQUIRE(ofs);
        }

        WHEN("inplace_write is attempted") {
            ERT::FortIO fortio(filename, std::ios_base::in, false, true);

            THEN("A runtime_error is raised") {
                REQUIRE_THROWS_AS(file_kw.inplace_write(fortio),
                                  std::runtime_error);
            }
        }
    }
}
