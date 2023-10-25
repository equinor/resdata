"""
Module with utility functions from util.c
"""

from resdata import ResdataPrototype

strcmp_int = ResdataPrototype("int util_strcmp_int( char* , char* )")
"""
Function to compare strings with embedded integers.

Will use proper numeric comparison when comparing strings with
embedded numbers, i.e. "CASE-9" will follow after "CASE-10" when
sorting:

   >> l = ["CASE-9" , "CASE-10"]
   >> l.sort()
   >> print(l)
      ["CASE-10" , "CASE-9"]
   >> l.sort( strcmp_int )
   >> print(l)
      ["CASE-9" , "CASE-10"]

When the standard strcmp() function is used for comparing strings
the '1' will compare as less than the '9' and the order will be
the reverse. Observe that the '-' is not interpreted as a sign
prefix. The strcmp_int function will interpret '.' as separating
character, wheras the strcmp_float() function will interpret '.'
as a descimal point.

@type: (str, str) -> int
"""

strcmp_float = ResdataPrototype("int util_strcmp_float( char* , char* )")
"""
Function to compare strings with embedded numbers.

See documentation of strcmp_int() for further details.
@type: (str, str) -> int
"""
