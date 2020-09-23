Competition Evaluation Tools - testScript.Py
-Run testScript.py to see example usage of mission plan parser and pointing evaluation script.
	-Results will be written to console.
	-Currently, testScript.py contains two methods for evaluating the mission plan and pointing:
		-Method 1: checkPlan(submission_fp) only accepts the mission plan and has the opportunity window and TLE hard coded. The opportunity window and TLE must be updated inside the defconWrapper.py before deployment
		-Method 2: checkPlan_allInputs(submission_fp, oppWindow, tleLine1, tleLine2) takes as inputs the mission plan, opportunity window, and TLE
		-Either method can be chosen for the competition.	
		
-Example mission plan in expected format is in sample_mission_plan_submission.txt. Note: All submissions are expected to be in a text file in UTF-8 encoding.
-A submission template (mission_plan_submission_template.txt) has been provided that can be filled by each team.

-Any error existing in the mission plan will be written to console as well as submissionErrors.txt. 
	- Errors printed to the console are purposely abridged in testScript.py. These errors are meant as hints for the competitors. A detailed list of errors is logged in submissionErrors.txt for the judges.
---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

Unit Test Evaluation Tools - unitTestScript.py
-Run unitTestScript.py to evaluate performance of mission plan parser and pointing evaluation script across various mission plan submissions.
	-Results will be written to the console. 
	-All test submissions contain errors (i.e. missing commands, incorrect command ordering, invalid formats, etc.) that will ensure the mission plan parser functioning properly.

-Errors existing in any test submissions will be written to console as well as submissionErrors.txt.
-Note: unitTestScript.py should not be made available to competitors during the competition. It is intended for testing purposes only!




