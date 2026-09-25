#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <fstream>
#include <ios>
#include <stdexcept>
#include <vector>

#include "ert/util/util.hpp"

#include <resdata/FortIO.hpp>
#include <resdata/rd_file_kw.hpp>
#include <resdata/rd_kw.hpp>
#include <resdata/rd_util.hpp>
#include <resdata/rd_type.hpp>

#include "tmpdir.hpp"

using Catch::Matchers::ContainsSubstring;

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

SCENARIO("A FileKW is constructed from an rd_kw's header") {
    GIVEN("An rd_kw with a known header, size and type") {
        rd::KW kw{"PORO", 5, RD_FLOAT};

        WHEN("A FileKW is created from its header with an offset") {
            FileKW file_kw(kw.header(), 256);

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
            std::ofstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            original.write_header(stream);
        }

        WHEN("The entry is read back from the stream") {
            std::ifstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            auto kw_list = FileKW::read(stream, 1);

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
            std::ofstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            first.write_header(stream);
            second.write_header(stream);
            third.write_header(stream);
        }

        WHEN("All three entries are read back") {
            std::ifstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            auto kw_list = FileKW::read(stream, 3);

            THEN("Each recovered FileKW equals its original in order") {
                REQUIRE(kw_list.size() == 3);
                REQUIRE(*kw_list[0] == first);
                REQUIRE(*kw_list[1] == second);
                REQUIRE(*kw_list[2] == third);
            }
        }

        WHEN("More entries are requested than are present") {
            std::ifstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);

            THEN("read throws a runtime_error") {
                REQUIRE_THROWS_AS(FileKW::read(stream, 10), std::runtime_error);
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "A full width eight character header round-trips") {
    GIVEN("A FileKW whose header fills all RD_STRING8_LENGTH bytes") {
        FileKW original(4096, RD_DOUBLE, 42, "EIGHTLEN");
        REQUIRE(original.get_header().size() == RD_STRING8_LENGTH);

        auto filename = (dirname / "index").string();
        {
            std::ofstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            original.write_header(stream);
        }

        WHEN("The entry is read back") {
            std::ifstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            auto kw_list = FileKW::read(stream, 1);

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
            std::ofstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
        }

        WHEN("read is asked for zero entries") {
            std::ifstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            auto kw_list = FileKW::read(stream, 0);

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
            std::ofstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            original.write_header(stream);
        }

        WHEN("The single entry has already been consumed") {
            std::ifstream stream(filename, std::ios_base::binary);
            REQUIRE(stream.good());
            stream.exceptions(std::ios_base::failbit | std::ios_base::badbit);
            auto disk_kw = FileKW::read(stream, 1);
            REQUIRE(*disk_kw[0] == original);

            THEN("Reading another entry throws") {
                REQUIRE_THROWS_AS(FileKW::read(stream, 1),
                                  std::ios_base::failure);
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir, "A FileKW lazily loads its keyword from file") {
    GIVEN("A keyword written to a fortran formatted file") {
        auto filename = (dirname / "DATA").string();

        rd::KW kw{"MYKW", std::vector<int>{0, 10, 20, 30}};

        offset_type offset;
        {
            ERT::FortIO fortio(filename, std::ios_base::out);
            offset = fortio.ftell();
            kw.fwrite(fortio);
        }

        FileKW file_kw(kw.header(), offset);

        THEN("The keyword is not loaded until requested") {
            REQUIRE(file_kw.get_kw_ptr() == nullptr);
        }

        WHEN("get_kw is called with a reading fortio handle") {
            ERT::FortIO fortio(filename, std::ios_base::in);
            rd::KW *loaded = file_kw.get_kw(fortio);

            THEN("The keyword is loaded and cached") {
                REQUIRE(loaded != nullptr);
                REQUIRE(file_kw.get_kw_ptr() == loaded);
                REQUIRE(loaded->size() == 4);
                for (int i = 0; i < 4; i++)
                    REQUIRE(loaded->at<int>(i) == i * 10);
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

SCENARIO_METHOD(Tmpdir, "get_kw reports a detailed mismatch between the cached "
                        "header and the file") {
    GIVEN("A keyword written to a fortran formatted file") {
        auto filename = (dirname / "DATA").string();

        rd::KW kw{"MYKW", std::vector<int>{0, 10, 20, 30}};

        offset_type offset;
        {
            ERT::FortIO fortio(filename, std::ios_base::out);
            offset = fortio.ftell();
            kw.fwrite(fortio);
        }

        WHEN("the cached header has the wrong name") {
            FileKW file_kw(offset, RD_INT, 4, "OTHER");
            ERT::FortIO fortio(filename, std::ios_base::in);

            THEN("get_kw throws with the expected and actual name, size and "
                 "type") {
                REQUIRE_THROWS_WITH(
                    file_kw.get_kw(fortio),
                    ContainsSubstring("expected name=\"OTHER\" size=4 "
                                      "type=INTE") &&
                        ContainsSubstring(
                            "got name=\"MYKW\" size=4 type=INTE"));
            }
        }

        WHEN("the cached header has the wrong size") {
            FileKW file_kw(offset, RD_INT, 3, "MYKW");
            ERT::FortIO fortio(filename, std::ios_base::in);

            THEN("get_kw throws with the expected and actual sizes") {
                REQUIRE_THROWS_WITH(
                    file_kw.get_kw(fortio),
                    ContainsSubstring(
                        "expected name=\"MYKW\" size=3 type=INTE") &&
                        ContainsSubstring(
                            "got name=\"MYKW\" size=4 type=INTE"));
            }
        }

        WHEN("the cached header has the wrong type") {
            FileKW file_kw(offset, RD_FLOAT, 4, "MYKW");
            ERT::FortIO fortio(filename, std::ios_base::in);

            THEN("get_kw throws with the expected and actual types") {
                REQUIRE_THROWS_WITH(
                    file_kw.get_kw(fortio),
                    ContainsSubstring(
                        "expected name=\"MYKW\" size=4 type=REAL") &&
                        ContainsSubstring(
                            "got name=\"MYKW\" size=4 type=INTE"));
            }
        }
    }
}

SCENARIO_METHOD(Tmpdir,
                "get_kw fails to load a keyword when the backing file has "
                "been detached") {
    GIVEN("A keyword written to a file, and a FortIO whose stream is closed "
          "and cannot be reopened") {
        auto filename = (dirname / "DATA").string();

        rd::KW kw{"MYKW", std::vector<int>{0, 10, 20, 30}};
        {
            ERT::FortIO fortio(filename, std::ios_base::out);
            kw.fwrite(fortio);
        }

        FileKW file_kw(kw.header(), 0);
        ERT::FortIO fortio(filename, std::ios_base::in);

        REQUIRE(fortio.fclose_stream());
        fs::remove(filename);

        THEN("get_kw throws an ios_base::failure") {
            REQUIRE_THROWS_AS(file_kw.get_kw(fortio), std::ios_base::failure);
            REQUIRE_THROWS_WITH(
                file_kw.get_kw(fortio),
                ContainsSubstring("trying to load a keyword after the "
                                  "backing file has been detached"));
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
                REQUIRE_THROWS_WITH(
                    file_kw.inplace_write(fortio),
                    ContainsSubstring("cannot write FileKW in place: "
                                      "keyword has not been loaded"));
            }
        }
    }
}
