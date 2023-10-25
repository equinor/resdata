class SummaryNode(object):
    def __init__(self, report_step, days, date, mpl_date, value):
        """
        SummaryNode is a 'struct' with a summary value and time.

        SummaryNode - a small 'struct' with a summary value and time in
        several formats. When iterating over a SummaryVector instance
        you will get SummaryNode instances. The content of the
        SummaryNode type is stored as plain attributes:

            value       : The actual value
            report_step : The report step
            days        : Days since simulation start
            date        : The simulation date
            mpl_date    : A date format suitable for matplotlib

        """
        self.value = value
        self.report_step = report_step
        self.days = days
        self.date = date
        self.mpl_date = mpl_date

    def __repr__(self):
        return "SummaryNode(days=%d, value=%g)" % (self.days, self.value)
