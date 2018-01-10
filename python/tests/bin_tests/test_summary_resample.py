#  Copyright (C) 2018  Statoil ASA, Norway.
#
#  This file is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.

import os.path
import subprocess
from subprocess import CalledProcessError as CallError

from ecl.ecl import Cell, EclGrid, EclSum
from tests import EclTest
from ecl.test.ecl_mock import createEclSum
from ecl.test import TestAreaContext


def fopr(days):
    return days

def fopt(days):
    return days

def fgpt(days):
    if days < 50:
        return days
    else:
        return 100 - days

def create_case():
    length = 100
    return createEclSum("CSV" , [("FOPT", None , 0) , ("FOPR" , None , 0), ("FGPT" , None , 0)],
                        sim_length_days = length,
                        num_report_step = 10,
                        num_mini_step = 10,
                        func_table = {"FOPT" : fopt,
                                      "FOPR" : fopr ,
                                      "FGPT" : fgpt })



class SummaryResampleTest(EclTest):

    @classmethod
    def setUpClass(cls):
        cls.script = os.path.join(cls.SOURCE_ROOT, "bin/summary_resample")
        cls.case = create_case()

    def test_run_default(self):
        with TestAreaContext(""):
            self.case.fwrite()

            # Too few arguments
            with self.assertRaises(CallError):
                subprocess.check_call([self.script])

            # Too few arguments
            with self.assertRaises(CallError):
                subprocess.check_call([self.script, "CSV"])

            # Invalid first arguments
            with self.assertRaises(CallError):
                subprocess.check_call([self.script, "DOES_NOT_EXIST", "OUTPUT"])

            # Should run OK:
            subprocess.check_call([self.script, "CSV", "OUTPUT"])
            output_case = EclSum("OUTPUT")
            self.assertEqual( output_case.get_data_start_time(), self.case.get_data_start_time())
            self.assertEqual( output_case.get_end_time(), self.case.get_end_time())
