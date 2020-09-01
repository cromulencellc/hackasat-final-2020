#!/usr/bin/env python3

####################################################################
# Initialize kubOS with the mission applications that will be used
# to communicate through I2C with the 
####################################################################

import app_api
import argparse
import sys

def create_mode(logger, mode_name):

    # GraphQL request to the scheduler service to create a mode.

    request = '''
mutation {
    createMode(name: "%s") {
        success
        errors
    }
}''' % mode_name

    logger.info(f"Creating mode: {mode_name}.")
    try:
        response = SERVICES.query(service='scheduler-service', query=request)
        logger.info(f"Mode created: {response['createMode']['success']}")                                #FIX
    except Exception as e:
        error_msg =  f"Creating mode {mode_name} failed: {str(e)}"
        logger.error(error_msg)

    return

def activate_mode(logger, mode_name):
    
    # Graphql request to the scheduler service to actually activate the mode created

    request = '''
mutation {
    activateMode(name: "%s") {
        success
        errors
    }
}   ''' % mode_name

    logger.info(f"Activating mode {mode_name}.")

    try:
        response = SERVICES.query(service='scheduler-service', query=request)
        logger.info(f"Mode {mode_name} activated: {response['activateMode']['success']}")
    except Exception as e:
        error_msg = f"Starting mode {mode_name} failed: {str(e)}"
        logger.error(error_msg)
    
    return

def create_mode_schedule(logger, mode_name, sch_name):

    # GraphQL request to the scheduler service to register a schedule for a specific mode

    schedule_path = f"/home/microsd/mission-apps/{sch_name}.json"

    request = '''    
mutation {
    importTaskList(name: "%s", path: "%s", mode: "%s") {
        success
        errors
    }
}   ''' % (sch_name, schedule_path, mode_name)

    logger.info(f"Creating mode {mode_name} with schedule {sch_name}.")
    try:
        response = SERVICES.query(service='scheduler-service', query=request)
        logger.info(f"Mode {mode_name} created: {response['importTaskList']['success']}")
    except Exception as e:
        error_msg =  f"Importing schedule {sch_name} in mode {mode_name} failed: {str(e)}"
        logger.error(error_msg)

    return

def register_all_apps(logger, apps_registrar):

    # Takes a list of app directories and calls a function to register each app
    
    for app in apps_registrar:
        register_app(logger, app)

    return

def register_app(logger, app_path):
    
    # GraphQL request to app-service to actually register the mission application. 

    request = """
mutation {
  register(path: "%s") {
    success,
    errors,
    entry {
      app {
        name
        executable
      }
    }
  }
}   """ % app_path

    logger.info(f"Registering app: {app_path.split('/')[-1]}")
    
    try:
        response = SERVICES.query(service='app-service', query=request)
        logger.info(f"App {app_path.split('/')[-1]} registered: {response['register']['success']}")
    except Exception as e:
        error_msg =  f"Registering {app_path.split('/')[-1]} failed: {str(e)}"
        logger.error(error_msg)

def main():

    logger = app_api.logging_setup("initchallenge")
    
    parser = argparse.ArgumentParser()
    
    parser.add_argument('--config', '-c', nargs=1)
    parser.add_argument('--mode', '-m', nargs=1)
    parser.add_argument('--schedule', '-s', nargs=1)
    
    args = parser.parse_args()
    
    if args.config is not None:
        global SERVICES
        SERVICES = app_api.Services(args.config[0])
    else:
        SERVICES = app_api.Services()
    
    ######### START #########

    logger.info("Initializing system. This will only run once.")

    if args.mode is None:
        mode_name = "nominal"       # Name of mode to create
    else:
        mode_name = args.mode

    if args.schedule is None:
        schedule_name = "imager"    # Name of schedule to insert into mode
    else:
        schedule_name = args.schedule 

    # Apps to register
    i2c_app   = "/home/microsd/mission-apps/leon3_i2c"
    # alive_app = "/home/microsd/mission-apps/keep_alive"

    ### Alive app was removed in order to speed up i2c. ###
    # apps_registrar = [i2c_app, alive_app]
    apps_registrar = [i2c_app]

    register_all_apps(logger, apps_registrar)

    create_mode(logger, mode_name)
    create_mode_schedule(logger, mode_name, schedule_name)

    activate_mode(logger, mode_name)

    logger.info("Initialized successfully.")
    
    
if __name__ == "__main__":
    main()