# -*- coding: utf-8 -*-
"""

This script checks for errors in mission plan submissions.
If verbosity flag is true (only for unit tests) detailed errors
will print to the console.

"""

import os  
  
def submission_error_check(filepath, errHints, verbosityFlag=None):
    fpSize = os.path.getsize(filepath)
    
    if fpSize > 0: 
        if verbosityFlag is None: 
            for i in range(0,len(errHints)):
                print(errHints[i])
        elif verbosityFlag:
            with open(filepath) as asd:
                print(asd.read())
     
    return fpSize