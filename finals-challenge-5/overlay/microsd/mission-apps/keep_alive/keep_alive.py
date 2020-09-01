#!/usr/bin/env python3

####################################################################
# Checks to see if the I2C app is running, and if not, starts it up.
####################################################################

import app_api
import argparse
import os
import sys

def check_app_running(logger, app_name):
    request = '''
query{
    (name: "%s") {
        success
        errors
    }
}''' % app_name

    logger.info(f"Checking app running: {app_name}.")
    try:
        response = SERVICES.query(service='monitor-service', query=request)                                #FIX
        logger.debug(f"App running checked: {response}.")
    except Exception as e:
        error_msg =  f"Checking app {app_name} failed: {str(e)}"
        logger.error(error_msg)

    return True

def start_app(logger):

    return True

def main():

    logger = app_api.logging_setup("keep_alive")
    
    parser = argparse.ArgumentParser()
    
    parser.add_argument('--config', '-c', nargs=1)
    parser.add_argument('--stop', '-s', action='store_true')
    
    args = parser.parse_args()
    
    if args.config is not None:
        global SERVICES
        SERVICES = app_api.Services(args.config[0])
    else:
        SERVICES = app_api.Services()
    
    if args.stop is not None:
        sys.exit(0)

    ######### START #########

    app_name = "leon3_i2c"

    if check_app_running(logger, app_name) == False:
        logger.warning("I2C application not running. Starting...")
        start_app(logger)
    
    
if __name__ == "__main__":
    main()