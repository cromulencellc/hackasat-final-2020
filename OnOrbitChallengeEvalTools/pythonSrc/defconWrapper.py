# -*- coding: utf-8 -*-
"""
This file contains two wrapper functions that can be used

1. The mission plan, TLE, and opportunity window are all inputs 
2. Only the mission plan is an input and the TLE/Opportunity window must be 
hard-coded in with the correct values before deployment

"""
from pythonSrc import mission_plan_parser as mp
from pythonSrc import submission_error_check as ec
from pythonSrc import evaluationScript as ev

# checkPlan_allInputs allows the input of the filepath, opportunity window, and TLE
def checkPlan_allInputs(filepath, oppWindow, tleLine1, tleLine2):
    
    # Parse through mission plan
    [time, q_BA, oppWindowFlag, slewCmdFlag, cam_exp, errHints] = mp.parse_mission_plan(filepath, oppWindow)
    print('Image capture time = %s' % time)
    print('Quaternion command = %s' % q_BA)
    print('Camera exposure time = %s micro-sec' % cam_exp)
    
    # The mission plan parser will provide a submission error list named submissionErrors.txt
    submission_err_fp = 'submissionErrors.txt'
    ec.submission_error_check(submission_err_fp, errHints)
    
    # Check command quaternion errors
    if (oppWindowFlag == 1 and slewCmdFlag == 1):
        ev.checkErrors(time, q_BA, tleLine1, tleLine2)
    elif (oppWindowFlag == 0):
        print('Image capture is not within the opportunity window!\n')
    elif (slewCmdFlag == 0):
        print('Invalid slew to quaternion command!\n')



# checkPlan hard-codes the opportunity window and TLE, so it must be updated 
# to the values provided to the competitors
def checkPlan(filepath):
    # Provide opportunity window
    # - Times must be in UTC in the form '<start>-to-<end>' where start and end are written as yr:mo:ddThh:mm:ssZ
    # - Ex: oppWindow = '2020-03-19T21:11:15Z-to-2020-03-19T21:12:15Z'
    oppWindow = '2020-08-09T00:20:00Z-to-2020-08-09T00:30:00Z'
    
    # Provide TLE
    # For example
    # tleLine1 = '1 44484U 19022C   20173.53266769 +.00000233 +00000-0 +20166-4 0  9992'
    # tleLine2 = '2 44484 051.6439 014.1229 0009579 327.4528 032.5861 15.30203504048868'
    tleLine1 = "1 46266U 19031D   20218.52876597 +.00001160 +00000-0 +51238-4 0  9991"
    tleLine2 = "2 46266 051.6422 157.7760 0010355 123.0136 237.1841 15.30304846055751"
    
    # Parse through mission plan
    [time, q_BA, oppWindowFlag, slewCmdFlag, cam_exp, errHints] = mp.parse_mission_plan(filepath, oppWindow)
    print('Image capture time = %s' % time)
    print('Quaternion command = %s' % q_BA)
    print('Camera exposure time = %s micro-sec' % cam_exp)
    
    # The mission plan parser will provide a submission error list named submissionErrors.txt
    submission_err_fp = 'submissionErrors.txt'
    ec.submission_error_check(submission_err_fp, errHints)
    
    # Check command quaternion errors
    if (oppWindowFlag == 1 and slewCmdFlag == 1):
        ev.checkErrors(time, q_BA, tleLine1, tleLine2)
    elif (oppWindowFlag == 0):
        print('Image capture is not within the opportunity window!\n')
    elif (slewCmdFlag == 0):
        print('Invalid slew to quaternion command!\n')    