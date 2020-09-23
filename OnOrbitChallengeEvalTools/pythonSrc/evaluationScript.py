# -*- coding: utf-8 -*-
"""
This script inputs a time of image, quaternion mapping from ECI to Body, and a
TLE and then provides a metric for how closely the camera LOS vector is pointed
toward the moon and the vehicle -Z axis toward nadir.


Input:
    time = array of 6 element = [Year, Month, Day, Hour, Minute, Second]
    q_BA = q_Body_ECI = quaternion mapping from ECI j2000 to body frame
    line 1 = first line of the TLE
    line 2 = second line of the TLE
"""
import ephem as eph
import numpy as np
import string


def checkErrors(time, q_BA, line1, line2):
    
    # Define conversion from degrees to radians
    rpd = np.pi/180;
    
    # Arbitrary name for satellite
    tleName = 'DEFCONSat'
    
    # Validate checksum of TLE
    validTLE = validateTLE(line1, line2)
    
    if not validTLE:
        return
    
    # Instantiate satellite object based on TLE
    DEFCONSat = eph.readtle(tleName, line1, line2);
    
    # Instantiate moon object
    moonEphem = eph.Moon();

    # Construct date object based on input time array 
    # [Year, month, day, hour, minute, second] in UTC
    d = eph.date((time[0], time[1], time[2], time[3], time[4], time[5]))

    # Set positions of satellite and moon objects at input time
    DEFCONSat.compute(d);
    moonEphem.compute(d)
    
    # Grab Satellite Right Ascension/Declination at input time
    dec = np.degrees(DEFCONSat.g_dec)*rpd;
    ra = np.degrees(DEFCONSat.g_ra)*rpd;
    
    # Convert Satellite RA/Dec to LOS vector in J2000 ECI
    uvec =np.array([np.cos(dec)*np.cos(ra), np.cos(dec)*np.sin(ra), np.sin(dec)])
    U_satPos_ECI = uvec;
    
    # Determine position in ECI J2000 by scaling by distance from Earth center
    satPos_ECI = uvec*(eph.earth_radius+DEFCONSat.elevation)/1000;
    
    # Grab moon Right Ascension/Declination at input time
    dec = np.degrees(moonEphem.g_dec)*rpd;
    ra = np.degrees(moonEphem.g_ra)*rpd;
    
    # Convert moon RA/DEC to Los Vector in J2000 ECI
    uvec =np.array([np.cos(dec)*np.cos(ra), np.cos(dec)*np.sin(ra), np.sin(dec)])
    
    # Determine position in ECI J2000 by scaling by mean moon distance from Earth 
    moonPos_ECI = uvec*moonEphem.earth_distance*eph.meters_per_au/1000;
    
    # Calculate vector from satellite to moon in ECI
    R_sat2Moon_ECI = moonPos_ECI  - satPos_ECI;
    
    # Determine unit vector of satellite-to-moon vector in ECI
    U_sat2Moon_ECI = R_sat2Moon_ECI/np.sqrt(np.dot(R_sat2Moon_ECI, R_sat2Moon_ECI));    
    
    
    # -Y Panel Camera LOS_Body ~ [0;-1;0];
    # Define true camera vector in body frame
    LOS_Body = np.array([0.007196099926469, -0.999687104708689, -0.023956394240496])
    
    # Convert q_BA to T_BA, q_BA = q_Body_ECI 
    q1q1 = q_BA[0]*q_BA[0] 
    q1q2 = q_BA[0]*q_BA[1] 
    q1q3 = q_BA[0]*q_BA[2] 
    q1q4 = q_BA[0]*q_BA[3] 
    q2q2 = q_BA[1]*q_BA[1] 
    q2q3 = q_BA[1]*q_BA[2] 
    q2q4 = q_BA[1]*q_BA[3]
    q3q3 = q_BA[2]*q_BA[2] 
    q3q4 = q_BA[2]*q_BA[3] 
    q4q4 = q_BA[3]*q_BA[3] 
    dd = -q1q1-q2q2-q3q3+q4q4 ;
    T_Body_ECI = np.zeros((3,3))
    
    T_Body_ECI[0,:] = np.array([dd+q1q1+q1q1,   2*(q1q2+q3q4),  2*(q1q3-q2q4)]);
    T_Body_ECI[1,:] = np.array([2*(q1q2-q3q4),  dd+q2q2+q2q2,   2*(q2q3+q1q4)])
    T_Body_ECI[2,:] = np.array([2*(q1q3+q2q4),  2*(q2q3-q1q4),  dd+q3q3+q3q3 ])
    
    # Transpose T_body_eci to get T_eci_body
    T_ECI_Body = np.transpose(T_Body_ECI)
    
    # Convert camera vector from body frame to ECI
    LOS_ECI = np.dot(T_ECI_Body, LOS_Body);
    
    # Normalize for safety
    LOS_ECI = LOS_ECI/np.sqrt(np.dot(LOS_ECI, LOS_ECI));   
    
    
    # Dot product the two and inverse cosine to get angle error
    angErr = np.arccos(np.dot(U_sat2Moon_ECI, LOS_ECI))/rpd;
    
    print('Angular Error from Camera Boresight to Moon Center: %f deg' % angErr)
    
    
    # Now check the secondary pane
    # Secondary panel vector in body frame (-Z face)
    secondaryPanel_Body = np.array([0,0,-1]);
    
    
    # Get secondary panel vector in ECI
    secondaryPanel_ECI = np.dot(T_ECI_Body, secondaryPanel_Body);
    
    # Should be normalized, but normalize anyways
    secondaryPanel_ECI = secondaryPanel_ECI / np.sqrt(np.dot(secondaryPanel_ECI, secondaryPanel_ECI))
    
    # Now to compare angle
    angErr = np.arccos(np.dot(-U_satPos_ECI, secondaryPanel_ECI))/rpd;
    
    print('Angular Error from -Z Panel to Earth Center: %f deg' % angErr)
    

def calcTLEChecksum(line):
    line.strip()
    val = 0
    for ii in range(68):
        var = line[ii]
        if (var == ' ' or var == '.' or var == '+' or var in string.ascii_letters):
            continue
        elif var == '-':
            val = val + 1
        else:
            val = val + int(var)

    val %= 10

    return val
    
    
def validateTLE(line1, line2):
    line1.strip()
    line2.strip()
    if (len(line1)!=69 or len(line2)!=69):
        print('Invalid TLE: Check Length')
        return 0
    
    checkSum1 = calcTLEChecksum(line1)
    checkSum2 = calcTLEChecksum(line2)

    if (int(line1[68])!=checkSum1):
        print('Invalid Line 1 TLE Checksum')
        return 0
    elif (int(line2[68])!=checkSum2):
        print('Invalid Line 2 TLE Checksum')
        return 0
    else:
        return 1
    
    
    