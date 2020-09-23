# -*- coding: utf-8 -*-
"""

Mission plan parser for DEFCON28 submissions

"""

import re
import pandas as pd
import datetime as dt
import pytz

def parse_mission_plan(filepath,oppWindow):
    """
    Parse text at given filepath

    Input Parameters
    ----------
    filepath : str
        - Filepath for file to be parsed
    oppWindow: str
        - Opportunity window allowed to capture target image 
        - Times must be in UTC in the form '<start>-to-<end>' where start and end are written as yr:mo:ddThh:mm:ssZ
            - Ex: oppWindow = '2020-03-19T21:11:15Z-to-2020-03-19T21:12:15Z'

    Returns
    -------
    t_image:       time to capture target image
    q_cmd:         commanded quaternion 
    oppWindowFlag: flag that checks whether t_image is within the opportunity window  
    slewCmdFlag:   flag that checks is slew to commanded quaternion exists in submission 
    cam_exp:       camera exposure time in micro-seconds
	actualCmdOrder:      command order in mission plan submission
    
    """
    
    ### Parse through opportunity window
    opp = re.split('[-to=T:Z\n]',oppWindow)
    
    # Remove empty cells in list 
    while ("" in opp):
        opp.remove("")
    while ('' in opp):
        opp.remove('')
    
    # Set epoch (this just serves as a starting reference to check the starting and ending windows) and convert 
    # start/end time to the same format as the epoch
    epoch = dt.datetime(1970,1,1,0,0,0)
    startOpp = dt.datetime(int(opp[0]), int(opp[1]), int(opp[2]), int(opp[3]), int(opp[4]), int(opp[5]))
    endOpp   = dt.datetime(int(opp[6]), int(opp[7]), int(opp[8]), int(opp[9]), int(opp[10]), int(opp[11]))
    
    # Convert opportunity endpoints to seconds
    startOpp = (startOpp - epoch).total_seconds()
    endOpp = (endOpp - epoch).total_seconds()

    ### Parse and check mission plan
    #Initialize variables required by the evaluation script (i.e. time to capture image, quaternion command and opportunity window flag)
    t_image = []
    q_cmd = []
    cam_exp = []
    slewCmdFlag = 0
    oppWindowFlag = 0
    tBtwnCmds = 0
    cmdExecTimeFlag = 0
    unknownCmdFlag = 0
    
    # Initialize missing command parameters
    actualCmdOrder = ['']*6      # Max 6 commands are expected
    cnt = 0;                   
    # Initialize error hints list
    errHintsTmp = ['']*40
    errHints = ['']*40
    hintsCnt = 0
    
    # Remove empty lines/spaces from the original submissions and convert all text to lower case. 
    # Write results to a new text file that will be parsed below
    new_filepath = rewriteMissionPlan(filepath)
    
    # Create error list
    errorFile = open("submissionErrors.txt","w")
    
    #Parse through re-written mission plan (content in new file should be identical to initial submission)
    with open(new_filepath, encoding="utf-8") as fp:
       
        # Read current line in text file 
        line = fp.readline()
        
        while line:
        
            ### Check if ACS init command has been provided 
            reg_match = _RegExLib(line)
            if reg_match.initACS:
                
                # Store command in actualCmdOrder list. Will be checked against expected command list later 
                cmdText = "Initialize ACS"
                actualCmdOrder[cnt] = cmdText

                # Read current line in text file 
                line = fp.readline()
                
                # Check if command execution time has been provided for init ACS command
                reg_match = _RegExLib(line)
                if reg_match.cmdExecTime:
                    [validTime, value] = parseTime(line)
                    if validTime:
                        # Initialize timing check between commands
                        tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                        tBtwnCmds = (tmpCheck - epoch).total_seconds()
    
                        # Proceed to next line 
                        line = fp.readline()
                    else:
                        # Save error hint
                        hintTxt = 'Errors exist in the command execution time format for the initialize ACS command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Bad command execution time format for initialize ACS command. \n'
                        errorFile.writelines(errTxt)
                        line = fp.readline()

    
                else:
                    # Save error hint
                    hintTxt = 'Initialize ACS command arguments are missing.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Command execution time for initialize ACS command was not provided. \n'
                    errorFile.writelines(errTxt)
 
            ### Check if slew to quaternion command has been provided             
            elif reg_match.slew2CmdQuat:
            
                cmdText = 'Slew to Commanded Quaternion'
                actualCmdOrder[cnt] = cmdText
                
                # Read current line in text file 
                line = fp.readline()
                
                # Check if command execution time has been provided for slew to quaternion command
                reg_match = _RegExLib(line)
                if reg_match.cmdExecTime:
                    [validTime, value] = parseTime(line)
                    
                    if validTime:
                        tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                        tmpCheck = (tmpCheck - epoch).total_seconds()
                        
                        # If initialize ACS command was provided, check if time between init ACS an slew to quat command is at 35 min (2100 s)
                        if tBtwnCmds > 1:
                            if actualCmdOrder[cnt] == 'Initialize ACS':
                                if ((tmpCheck - tBtwnCmds) < 2100):
                                    # Save error hint
                                    hintTxt = 'Errors exist in the timing between initialize ACS and slew to quaternion command.'
                                    errHintsTmp[hintsCnt] =  hintTxt
                                    hintsCnt += 1
                                    # Write error text to file
                                    errTxt = 'Warning: Time between initialize ACS and slew to quaternion command is less than the required 2100 sec. \n'
                                    errorFile.writelines(errTxt) 
                            else:
                                if ((tmpCheck - tBtwnCmds) < 5):
                                    # Save error hint
                                    hintTxt = 'Errors exist in the timing between the slew to quaternion command and the previous command.'
                                    errHintsTmp[hintsCnt] =  hintTxt
                                    hintsCnt += 1
                                    # Write error text to file
                                    errTxt = 'Warning: Time between the slew to quaternion command and the previous command is less than the required 5 sec. \n'
                                    errorFile.writelines(errTxt) 
                        else:
                            tBtwnCmds = tmpCheck
                        
                        tBtwnCmds = tmpCheck   
                        cmdExecTimeFlag = 1
                        line = fp.readline()
                    else:
                        # Save error hint
                        hintTxt = 'Errors exist in the command execution time format for the slew to quaternion command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Bad command execution time format for slew to quaternion command. \n'
                        errorFile.writelines(errTxt)
                        line = fp.readline()
                else: 
                    # Save error hint
                    hintTxt = 'Slew to quaternion command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Command execution time for slew to quaternion command was not provided. \n'
                    errorFile.writelines(errTxt)
                    
                # Check if ECI to Body quaternion has been provided for slew to quaternion command   
                argName = re.split("=",line)
                dum = re.sub("-"," ", argName[0])
                line = dum + argName[1]
                reg_match = _RegExLib(line)      
                if reg_match.eci2BodyQuat:
                    value = re.sub("[0-9a-z]+\.\s+|[\[\]]|\n","",line) 
                    value = re.split('[a-z]+|.[eci to body quaternion=]|,', value)

                    # Replace all non-ascii characters with a hyphen. 
                    for ii in range(len(value)):
                        value[ii] = re.sub(r'[^\x00-\x7F]+','-',value[ii])
                    while("" in value) : 
                        value.remove("")
                    while ('' in value):
                        value.remove('')
                    
                    try:
                        q_cmd = [float(value[0]), float(value[1]), float(value[2]), float(value[3])]   
                        line = fp.readline()
                        slewCmdFlag = 1
                    except:
                        slewCmdFlag = 0
                        hintTxt = 'Error in ECI-to-Body quaternion. \n'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Error in ECI-to-Body quaternion. \n'
                        errorFile.writelines(errTxt)
                        line = fp.readline()
                else:
                    # Save error hint
                    hintTxt = 'Slew to quaternion command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'ECI-to-Body quaternion for slew to quaternion command was not provided. \n'
                    errorFile.writelines(errTxt)
                    
                   
                # Check if slew completion time has been provided for slew to quaternion command        
                reg_match = _RegExLib(line)
                if reg_match.slewCompTime:
                    value = re.sub("[0-9a-z]+\.\s+","",line)
                    value = re.split('[-slew completion time.=t:z\n]', value)

                    while("" in value) : 
                        value.remove("")
                        
                    try :
                        
                        if cmdExecTimeFlag:
                            tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                            tmpCheck = (tmpCheck - epoch).total_seconds()
                            if ((tmpCheck - tBtwnCmds) < 180):
                                # Save error hint
                                hintTxt = 'Errors exist in the timing between command execution time and slew completion time in the slew to quaternion command.'
                                errHintsTmp[hintsCnt] =  hintTxt
                                hintsCnt += 1
                                # Write error text to file
                                errTxt = 'Warning: Time between command execution time and slew completion time in the slew to quaternion command is less than the required 180 sec. \n'
                                errorFile.writelines(errTxt)      
                        line = fp.readline()
                    except:
                        # Save error hint
                        hintTxt = 'Errors exist in the slew completion time format for the slew to quaternion command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Bad slew completion time format for slew to quaternion command. \n'
                        errorFile.writelines(errTxt)
                        line = fp.readline()
                else:
                    # Save error hint
                    hintTxt = 'Slew to quaternion command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Slew completion time for slew to quaternion command was not provided. \n'
                    errorFile.writelines(errTxt)

            ### Check if camera on command has been provided
            elif reg_match.turnCamOn: 

                cmdText = 'Turn Camera On'
                actualCmdOrder[cnt] = cmdText
                
                line = fp.readline()
                
                # Check if command execution time has been provided for camera on command
                reg_match = _RegExLib(line)
                if reg_match.cmdExecTime:
                    [validTime, value] = parseTime(line)
                    
                    if validTime:
                        
                        tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                        tmpCheck = (tmpCheck - epoch).total_seconds()
                        # If initialize ACS command was provided, check if time between init ACS an slew to quat command is at 35 min (2100 s)
                        if tBtwnCmds > 1:
                            if ((tmpCheck - tBtwnCmds) < 5):
                                # Save error hint
                                hintTxt = 'Errors exist in the timing between the camera on command and the previous command.'
                                errHintsTmp[hintsCnt] =  hintTxt
                                hintsCnt += 1
                                # Write error text to file
                                errTxt = 'Warning: Time between camera on and previous command is less than the required 5 sec. \n'
                                errorFile.writelines(errTxt) 
                            
                        tBtwnCmds = tmpCheck    
                        line = fp.readline()
                    else:
                        # Save error hint
                        hintTxt = 'Errors exist in the execution time format for the camera on command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Bad command execution time format for camera on command. \n'
                        errorFile.writelines(errTxt)        
                        line = fp.readline()
                else:
                    # Save error hint
                    hintTxt = 'Camera on command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Command execution time for camera on command was not provided. \n'
                    errorFile.writelines(errTxt)                 

            ### Check if camera exposure command has been provided
            elif reg_match.setCamExp: 

                cmdText = 'Set Camera Exposure'
                actualCmdOrder[cnt] = cmdText
                
                line = fp.readline()
                
                # Check if command execution time has been provided for camera exposure command
                reg_match = _RegExLib(line)
                if reg_match.cmdExecTime:
                    [validTime, value] = parseTime(line)
                    
                    if validTime:
                        tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                        tmpCheck = (tmpCheck - epoch).total_seconds()
                        
                        # If camera exposure command was provided, check if time between camera exposure and capture image command is at least 5 sec
                        if tBtwnCmds > 1:
                            if ((tmpCheck - tBtwnCmds) < 5):
                                # Save error hint
                                hintTxt = 'Errors exist in the timing between the camera exposure command and the previous command.'
                                errHintsTmp[hintsCnt] =  hintTxt
                                hintsCnt += 1
                                # Write error text to file
                                errTxt = 'Warning: Time between camera exposure and previous command is less than the required 5 sec. \n'
                                errorFile.writelines(errTxt) 
    
                        tBtwnCmds = tmpCheck  
                        line = fp.readline()
                    else:
                        # Save error hint
                        hintTxt = 'Errors exist in the command execution time format for the camera exposure command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Bad command execution time format for camera exposure command. \n'
                        errorFile.writelines(errTxt)
                        line = fp.readline()
                else:
                    # Save error hint
                    hintTxt = 'Camera exposure command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Command execution time for camera exposure command was not provided. \n'
                    errorFile.writelines(errTxt)

                reg_match = _RegExLib(line)
                if reg_match.setExpTime:
                    
                    value = re.sub("[0-9a-z]+\.\s+","",line)
                    value = re.split('[set exposure time.=\n]', value)
                    
                    while ("" in value): 
                        value.remove("") 
                    
                    cam_exp = int(value[0])
                    # Check if camera exposure  time is betweeen [20, 3000] micro-sec 
                    if (cam_exp <= 20 or cam_exp >= 3000):
                        # Save error hint
                        hintTxt = 'Argument for the camera exposure time command is not within the valid range.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Warning: Camera exposure time is not within [20, 3000] micro-sec. \n'
                        errorFile.writelines(errTxt) 
                      
                    line = fp.readline()
                else:
                    cam_exp = '[]'
                    # Save error hint
                    hintTxt = 'Camera exposure command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt =  'Exposure time in camera exposure command was not provided.\n'
                    errorFile.writelines(errTxt)
                    
            ### Check if image capture command has been provided
            elif reg_match.captureImg:

                cmdText = 'Capture Target Image'
                actualCmdOrder[cnt] = cmdText
                
                line = next(fp)

                # Check if command execution time has been provided for image capture command
                reg_match = _RegExLib(line)
                if reg_match.cmdExecTime:

                    [validTime, value] = parseTime(line)
                        
                    if validTime:
                        t_image = [int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5])]
    
                        tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                        tmpCheck = (tmpCheck - epoch).total_seconds()
                        
                        # Check if command execution time is within opportunity window
                        if (tmpCheck >= startOpp) and (tmpCheck <= endOpp):
                            oppWindowFlag = 1
                            t_image = [int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5])]
                        else:
                            oppWindowFlag = 0
                            # Save error hint
                            hintTxt = 'Command execution time to capture the image is not within the opportunity window!'
                            errHintsTmp[hintsCnt] =  hintTxt
                            hintsCnt += 1
                            # Write error text to file
                            errTxt = 'Command execution time to capture the image is not within the opportunity window! \n'
                            errorFile.writelines(errTxt)
    
                        # If camera on command was provided, check if time between camera on and capture image command is at least 5 sec
                        if tBtwnCmds > 1:
                            if ((tmpCheck - tBtwnCmds) < 5):
                                # Save error hint
                                hintTxt = 'Errors exist in the timing between the camera off command and the previous command.'
                                errHintsTmp[hintsCnt] =  hintTxt
                                hintsCnt += 1
                                # Write error text to file
                                errTxt = 'Warning: Time between camera off and previous command is less than the required 5 sec. \n'
                                errorFile.writelines(errTxt) 
                            
                        tBtwnCmds = tmpCheck  
                        line = fp.readline()  
                    else:
                        # Save error hint
                        hintTxt = 'Errors exist in the execution time format for the capture target image command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Warning: Bad command execution time format for capture target image command. \n'
                        errorFile.writelines(errTxt) 
                        line = fp.readline()
                else:
                    # Save error hint
                    hintTxt = 'Capture target image command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Command execution time for image capture command was not provided. \n'
                    errorFile.writelines(errTxt)                    

            # Check if camera off command has been provided 
            elif reg_match.turnCamOff:

                cmdText = 'Turn Camera Off'
                actualCmdOrder[cnt] = cmdText   
                
                line = fp.readline()

                # Check if command execution time has been provided for camera off command
                reg_match = _RegExLib(line)
                if reg_match.cmdExecTime:
                    [validTime, value] = parseTime(line)
                    
                    if validTime:
                        tmpCheck = dt.datetime(int(value[0]), int(value[1]), int(value[2]), int(value[3]), int(value[4]), int(value[5]))
                        tmpCheck = (tmpCheck - epoch).total_seconds()
                        # If camera oFF command was provided, check if time between camera off and capture image command is at least 5 sec
                        if tBtwnCmds > 1:
                            if ((tmpCheck - tBtwnCmds) < 5):
                                # Save error hint
                                hintTxt = 'Errors exist in the timing between the camera off command and the previous command.'
                                errHintsTmp[hintsCnt] =  hintTxt
                                hintsCnt += 1
                                # Write error text to file
                                errTxt = 'Warning: Time between camera off and previous command is less than the required 5 sec. \n'
                                errorFile.writelines(errTxt) 
                        
                        line = fp.readline()  
                    else:
                        # Save error hint
                        hintTxt = 'Errors exist in the command execution time format for the camera off command.'
                        errHintsTmp[hintsCnt] =  hintTxt
                        hintsCnt += 1
                        # Write error text to file
                        errTxt = 'Bad command execution time format for camera off command. \n'
                        errorFile.writelines(errTxt)
                        break 

                else:
                    # Save error hint
                    hintTxt = 'Camera off command has missing arguments.'
                    errHintsTmp[hintsCnt] =  hintTxt
                    hintsCnt += 1
                    # Write error text to file
                    errTxt = 'Command execution time for camera off command was not provided. \n'
                    errorFile.writelines(errTxt)
                    break 
            else:
                # If an unknown command has been detected, record an error 
                line = line.rstrip('\n')
                errTxt = 'The following command in the mission plan is invalid: ' + line 
                # Save error hint
                errHintsTmp[hintsCnt] = errTxt
                hintsCnt += 1
                # Write error text to file
                errorFile.writelines(errTxt)
                # Set unknown command flag to true
                unknownCmdFlag = 1
            
            # If an unknown command was detected, clear the unknown command flag and proceed to the next line.
            # Otherwise, continue increasing the command order counter
            if unknownCmdFlag == 1:
                # Reset unknown command flag and proceed to next line
                unknownCmdFlag = 0
                line = fp.readline()
            else:
                # Increasing the command order counter
                cnt += 1
            
            ### Check if we're at the end of a file
            if line == "":
                break
           
        ### Check for missing or out of order commands
        [wrongOrderList, missingCmdList, correctOrderFlag, correctCmdOrder] = missionPlanCmdOrder(actualCmdOrder)
        emptyList = [['']*len(correctCmdOrder),['']*len(correctCmdOrder)]
        emptyHintsList = ['']*40
 
        # Missing commands
        if missingCmdList[0]:
            # Save missing command hint
            errHints[0] = '\nWarning: Commands may be missing from the mission plan.'
            for i in range(0,len(missingCmdList[0])):   
                errTxt = 'Missing Command Warning: %s command was not provided. \n' % missingCmdList[0][i]
                # Write error text to file
                errorFile.writelines(errTxt)
                
        # Save "command argument error" hint
        if errHintsTmp != emptyHintsList:
            errHints[1] = '\nWarning: Errors exist in some command arguments (listed below)'        
        
        # Out of order commands
        if not correctOrderFlag: 
            errHints[2] = '\nWarning: Command order may be incorrect.'
            for i in range(0,len(wrongOrderList[0])):
                if (wrongOrderList[0][i] != wrongOrderList[1][i]):
                    if wrongOrderList[1][i] == -1:
                        errTxt = 'Command Order Warning: Expected %s command as command #%s, but the command is missing from the plan. \n' % (correctCmdOrder[wrongOrderList[0][i]], (i+1))
                    else:
                        errTxt = 'Command Order Warning: Expected %s command as command #%s. \n' % (correctCmdOrder[wrongOrderList[0][i]], (i+1))
                
                    # Write error text to file
                    errorFile.writelines(errTxt)
        
        # End of while loop, close errorFile
        errorFile.close()    
        
        # Compile all error hints into one array and remove empty cells
        for k in range(3,len(errHintsTmp)):
            errHints[k] = errHintsTmp[k-3]
        while ("" in errHints):
            errHints.remove("")
        
        # Return time of image and quaternion command required for evaluation script
        return t_image, q_cmd, oppWindowFlag, slewCmdFlag, cam_exp, errHints

### Rewrite submitted mission plan after removing blank lines/spaces and converting all text to lower case
def rewriteMissionPlan(filepath):
    tmpFile = open("tempSubmissionTxt.txt","w")
    with open(filepath, encoding="utf-8") as fp:
        # Read current line in text file and convert to lower case 
        line = fp.readline()
        line = line.lower()
        
        while line:
            if line != "\n":
                tmpTxt = line.lower()
                tmpFile.writelines(tmpTxt)
            
            line = fp.readline()
        tmpFile.close()

        return tmpFile.name
        
### Check mission plan command order
def missionPlanCmdOrder(actualOrder):
    # Possible mission plan order combinations (6 total with 6 commands each)
    cmdOrd1 = ['Initialize ACS', 'Slew to Commanded Quaternion', 'Turn Camera On',
               'Set Camera Exposure', 'Capture Target Image', 'Turn Camera Off']
    cmdOrd2 = ['Initialize ACS', 'Turn Camera On', 'Set Camera Exposure',
               'Slew to Commanded Quaternion', 'Capture Target Image', 'Turn Camera Off']
    cmdOrd3 = ['Initialize ACS', 'Turn Camera On','Slew to Commanded Quaternion', 
               'Set Camera Exposure','Capture Target Image', 'Turn Camera Off']              
    cmdOrd  = [cmdOrd1, cmdOrd2, cmdOrd3]
    
    #Initialize variables (initializing to 999 since 0 is a possible index in the command order)
    ordIdx =  [[999]*len(cmdOrd1), [999]*len(cmdOrd1)]
    missCmd = [[999]*len(cmdOrd1), [999]*len(cmdOrd1)]
    for k1 in range(0,len(cmdOrd)):
        idxCntr = 0
        cntr = 0
        sortFlag = 1
        expectedOrder = cmdOrd[k1]
        sortIdx = ['']*len(cmdOrd1)
        ### Check for missing and out of order commands
        for k in range(0,len(expectedOrder)):
            tmp = str([j for j, i in enumerate(actualOrder) if expectedOrder[k] in i])
            tmp = re.sub("[\[\]]","",tmp)
            tmp1 = tmp
            # If a command is missing, populate tmp with a negative number marker, save the missing command,
            # and also save an empty string to the sorting array
            if not tmp:
                tmp = '-1'
                missCmd[0][cntr] = expectedOrder[idxCntr]
                missCmd[1][cntr] = k
                tmp1 = ''
                cntr += 1
            # Save index from expected order list in 1st column and actual order list in 2nd column of temp 1
            ordIdx[0][idxCntr] = k 
            ordIdx[1][idxCntr] = int(tmp)

            # Save sorting index
            sortIdx[idxCntr] = tmp1

            # Increase index counter
            idxCntr += 1

        # Remove any elements in the missing command array that are equal to 0
        while (999 in missCmd[0]):
            missCmd[0].remove(999)
        while (999 in missCmd[1]):
            missCmd[1].remove(999)
        
        #Remove empty elements from the sorting array
        while ("" in sortIdx):
            sortIdx.remove("")

        # Determine if the provided mission plan is equal to any of the 6 possible plan combinations
        if (sortIdx != sorted(sortIdx)): # If not, set sortFlag to flase and check the next combination
            sortFlag = 0
        else: # If plan matches current combination, save the plan combination order (needed for error list) and break out of forloop
            correctOrd = cmdOrd[k1]
            break
        
        # If not finished checking provided plan against the 6 possible combinations, continue checking
        if k1 < (len(cmdOrd)-1):
            correctOrd = cmdOrd[k1]
        else: # Otherwise, save the first combination. Evaluation team will be require to check the order further.
            correctOrd = cmdOrd[0]

    return ordIdx, missCmd, sortFlag, correctOrd
        
### Time parsing wrapper
def parseTime(timeString):
    timeString = re.sub("[0-9a-z]+\.\s+","",timeString)
    timeString = re.split('[-command execution time.=t:z\n]', timeString)
    while("" in timeString) : 
        timeString.remove("")
    
    if len(timeString)!=6 :
        valid = 0
    else:
        valid = 1

    return valid, timeString 


class _RegExLib:
    """Set up regular expressions"""
    # use https://regexper.com to visualise these if required
    _reg_initACS      = re.compile(r'initialize acs')
    _reg_slew2CmdQuat = re.compile(r'slew to commanded quaternion')
    _reg_turnCamOn    = re.compile(r'turn camera on')
    _reg_setCamExp    = re.compile(r'set camera exposure')
    _reg_captureImg   = re.compile(r'capture target image')
    _reg_turnCamOff   = re.compile(r'turn camera off')
    _reg_cmdExecTime  = re.compile(r'command execution time')
    _reg_eci2BodyQuat = re.compile(r'eci to body quaternion')
    _reg_slewCompTime = re.compile(r'slew completion time')
    _reg_setExpTime   = re.compile(r'set exposure time')
    
    def __init__(self, line):
        # check whether line has a positive match with all of the regular expressions
        self.initACS      = self._reg_initACS.search(line)
        self.slew2CmdQuat = self._reg_slew2CmdQuat.search(line)
        self.turnCamOn    = self._reg_turnCamOn.search(line)
        self.setCamExp    = self._reg_setCamExp.search(line)
        self.captureImg   = self._reg_captureImg.search(line)
        self.turnCamOff   = self._reg_turnCamOff.search(line)
        self.cmdExecTime  = self._reg_cmdExecTime.search(line)
        self.eci2BodyQuat = self._reg_eci2BodyQuat.search(line)
        self.slewCompTime = self._reg_slewCompTime.search(line)
        self.setExpTime   = self._reg_setExpTime.search(line)
        
        
