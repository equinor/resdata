#!/usr/bin/env python
import os.path
import os

from unittest import skipIf

from resdata.util.test import TestAreaContext
from tests import ResdataTest


class WorkAreaTest(ResdataTest):
    def test_full_path(self):
        with TestAreaContext("TestArea") as test_area:
            with open("test_file", "w") as fileH:
                fileH.write("Something")

                self.assertTrue(os.path.isfile("test_file"))

            with self.assertRaises(IOError):
                full_path = test_area.getFullPath("does/not/exists")

            with self.assertRaises(IOError):
                full_path = test_area.getFullPath("/already/absolute")

            full_path = test_area.getFullPath("test_file")
            self.assertTrue(os.path.isfile(full_path))
            self.assertTrue(os.path.isabs(full_path))

    def test_IOError(self):
        with TestAreaContext("TestArea") as test_area:
            with self.assertRaises(IOError):
                test_area.copy_file("Does/not/exist")

            with self.assertRaises(IOError):
                test_area.install_file("Does/not/exist")

            with self.assertRaises(IOError):
                test_area.copy_directory("Does/not/exist")

            with self.assertRaises(IOError):
                test_area.copy_parent_directory("Does/not/exist")

            os.makedirs("path1/path2")
            with open("path1/file.txt", "w") as f:
                f.write("File ...")

            with self.assertRaises(IOError):
                test_area.copy_directory("path1/file.txt")

    def test_multiple_areas(self):
        original_dir = os.getcwd()
        context_dirs = []
        for i in range(3):
            loop_dir = os.getcwd()
            self.assertEqual(
                loop_dir,
                original_dir,
                "Wrong folder before creating TestAreaContext. Loop: {} -- CWD: {} ".format(
                    i, loop_dir
                ),
            )

            with TestAreaContext("test_multiple_areas") as t:
                t_dir = t.get_cwd()

                self.assertNotIn(
                    t_dir,
                    context_dirs,
                    "Multiple TestAreaContext objects in the same folder. Loop {} -- CWD: {}".format(
                        i, loop_dir
                    ),
                )
                context_dirs.append(t_dir)

                # It is possible to make the following assert fail, but whoever run the tests should
                # try really really hard to make that happen
                self.assertNotEqual(
                    t_dir,
                    original_dir,
                    "TestAreaContext in the current working directory. Loop: {} -- CWD: {}".format(
                        i, loop_dir
                    ),
                )

            loop_dir = os.getcwd()
            self.assertEqual(
                loop_dir,
                original_dir,
                "Wrong folder after creating TestAreaContext. Loop: {} -- CWD: {} ".format(
                    i, loop_dir
                ),
            )


def test_make_files(tmp_path):
    file = tmp_path / "file"
    dir = tmp_path / "dir"
    dir2 = tmp_path / "dir2"
    dir3 = tmp_path / "dir3"
    dir4 = tmp_path / "dir4"
    dir_file = dir2 / "file2"
    dir_file2 = dir3 / "file3"
    dir_file3 = dir4 / "file4"
    dir.mkdir()
    dir2.mkdir()
    dir3.mkdir()
    dir4.mkdir()
    file.write_text("text")
    dir_file.write_text("text2")
    dir_file2.write_text("text3")
    dir_file3.write_text("text4")

    abs_file = file.absolute()
    abs_dir_file = dir_file.absolute()
    abs_dir = dir.absolute()
    abs_dir2 = dir2.absolute()
    abs_dir_file2 = dir_file2.absolute()
    abs_dir_file3 = dir_file3.absolute()

    with TestAreaContext("TestArea") as test_area:
        test_area.copy_file(str(abs_file))
        assert os.path.exists(test_area.get_cwd() / file)

        test_area.install_file(str(abs_dir_file))
        assert os.path.exists(test_area.get_cwd() / dir_file)

        test_area.copy_directory(str(abs_dir))
        assert os.path.exists(test_area.get_cwd() / dir)
        assert os.path.isdir(test_area.get_cwd() / dir)

        test_area.copy_directory_content(str(abs_dir2))
        assert os.path.exists(os.path.join(test_area.get_cwd(), "file2"))

        test_area.copy_parent_content(str(abs_dir_file2))
        assert os.path.exists(os.path.join(test_area.get_cwd(), "file3"))

        test_area.copy_parent_directory(str(abs_dir_file3))
        assert os.path.exists(os.path.join(test_area.get_cwd(), "dir4", "file4"))
