cmake_minimum_required(VERSION 2.6.4)
project(CFS_UART_TO_CI C)

include_directories(fsw/mission_inc)
include_directories(fsw/platform_inc)
include_directories(fsw/src)
include_directories(./../inc)
include_directories(${cfs_lib_MISSION_DIR}/fsw/public_inc)
include_directories(${cfs_lib_PLATFORM_DIR}/fsw/public_inc)
include_directories(${osk_app_fw_MISSION_DIR}/fsw/platform_inc)
include_directories(${osk_app_fw_MISSION_DIR}/fsw/mission_inc)
include_directories(${expat_lib_MISSION_DIR}/fsw/platform_inc)
include_directories(${io_lib_MISSION_DIR}/fsw/public_inc)
include_directories(${osk_demo_app_MISSION_DIR}/fsw/platform_inc)
include_directories(${ds_MISSION_DIR}/fsw/platform_inc)
include_directories(${hs_MISSION_DIR}/fsw/platform_inc)
include_directories(${fm_MISSION_DIR}/fsw/platform_inc)
include_directories(${lc_MISSION_DIR}/fsw/platform_inc)
include_directories(${sc_MISSION_DIR}/fsw/platform_inc)

aux_source_directory(fsw/src APP_SRC_FILES)
aux_source_directory(fsw/tables APP_TABLE_FILES)

# Create the app module
add_cfe_app(uart_to_ci ${APP_SRC_FILES})

