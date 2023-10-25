from resdata.util.util import monkey_the_camel
from .rd_sum import Summary


class ResdataCase(object):
    def __init__(self, case):
        self.case = case

        self.grid = None
        self.restart = None
        self.init = None
        self.summary = None

        self.loadSummary()

    def __contains__(self, key):
        return key in self.summary

    def keys(self):
        return self.summary.keys()

    def wells(self):
        return self.summary.wells()

    def load_summary(self):
        self.summary = Summary(self.case)

    def start_time_equal(self, other):
        if self.summary.getDataStartTime() == other.summary.getDataStartTime():
            return True
        else:
            return False

    def end_time_equal(self, other):
        if self.summary.getEndTime() == other.summary.getEndTime():
            return True
        else:
            return False

    def cmp_summary_vector(self, other, key, sample=100):
        if key in self and key in other:
            days_total = min(
                self.summary.getSimulationLength(), other.summary.getSimulationLength()
            )
            dt = days_total / (sample - 1)
            days = [x * dt for x in range(sample)]

            ref_data = self.summary.get_interp_vector(key, days_list=days)
            test_data = other.summary.get_interp_vector(key, days_list=days)
            diff_data = ref_data - test_data

            ref_sum = sum(ref_data)
            diff_sum = sum(abs(diff_data))
            return (diff_sum, ref_sum)
        else:
            raise KeyError("Key:%s was not present in both cases" % key)


class ResdataCmp(object):
    def __init__(self, test_case, ref_case):
        """Class to compare to simulation cases with Eclipse formatted result files.

        The first argument is supposed to be the test_case and the
        second argument is the reference case. The arguemnts should be
        the basenames of the simulation, with an optional path
        prefix - an extension is accepted, but will be ignored.

        The constructor will start be calling the method initCheck()
        to check that the two cases are 'in the same ballpark'.
        """
        self.test_case = ResdataCase(test_case)
        self.ref_case = ResdataCase(ref_case)

        self.initCheck()

    def init_check(self):
        """A crude initial check to verify that the cases can be meaningfully
        compared.
        """
        if not self.test_case.startTimeEqual(self.ref_case):
            raise ValueError(
                "The two cases do not start at the same time - can not be compared"
            )

    def has_summary_vector(self, key):
        """
        Will check if both test and refernce have @key.
        """
        return (key in self.test_case, key in self.ref_case)

    def end_time_equal(self):
        """
        Will check that ref_case and test_case are equally long.
        """
        return self.test_case.endTimeEqual(self.ref_case)

    def cmp_summary_vector(self, key, sample=100):
        """Will compare the summary vectors according to @key.

        The comparison is based on evaluating the integrals:

           I0 = \int R(t) dt

           delta = \int | R(t) - T(t)| dt

        numericall. R(t) is the reference solution and T(t) is
        testcase solution. The return value is a tuple:

             (delta, I0)

        So that a natural way to evaluate the check for numerical
        equality, based on the relative error could be:

           delta, scale = rd_cmp.cmpSummaryVector("WWCT:OP_1")

           if delta/scale < 0.0001:
               print("Equal enough")
           else:
               print("Different ..")

        The upper limit for the integrals is:

           max(length(ref_case), length(test_case))

        meaning that two simulations which don't have the same
        end-time will compare as equal if they compare equal in the
        common part. If that is not OK you should call the
        endTimeEqual() method independently.
        """
        return self.test_case.cmpSummaryVector(self.ref_case, key, sample=sample)

    def test_keys(self):
        """
        Will return a list of summary keys in the test case.
        """
        return self.test_case.keys()

    def test_wells(self):
        """
        Will return a list of wells keys in the test case.
        """
        return self.test_case.wells()


monkey_the_camel(ResdataCase, "loadSummary", ResdataCase.load_summary)
monkey_the_camel(ResdataCase, "startTimeEqual", ResdataCase.start_time_equal)
monkey_the_camel(ResdataCase, "endTimeEqual", ResdataCase.end_time_equal)
monkey_the_camel(ResdataCase, "cmpSummaryVector", ResdataCase.cmp_summary_vector)

monkey_the_camel(ResdataCmp, "initCheck", ResdataCmp.init_check)
monkey_the_camel(ResdataCmp, "hasSummaryVector", ResdataCmp.has_summary_vector)
monkey_the_camel(ResdataCmp, "endTimeEqual", ResdataCmp.end_time_equal)
monkey_the_camel(ResdataCmp, "cmpSummaryVector", ResdataCmp.cmp_summary_vector)
monkey_the_camel(ResdataCmp, "testKeys", ResdataCmp.test_keys)
monkey_the_camel(ResdataCmp, "testWells", ResdataCmp.test_wells)
