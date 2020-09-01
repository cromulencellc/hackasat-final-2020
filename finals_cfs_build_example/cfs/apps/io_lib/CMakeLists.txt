cmake_minimum_required(VERSION 2.6.4)
project(CFS_IO_LIB C)

include_directories(fsw/public_inc)

aux_source_directory(fsw/src APP_SRC_FILES)
aux_source_directory(fsw/src/formats APP_SRC_FILES)
aux_source_directory(fsw/src/services APP_SRC_FILES)
# The shared OSAL and cFE include directories should always be used
# Note that this intentionally does NOT include PSP-specific includes, just the generic
include_directories(${CFECORE_SOURCE_DIR}/src/inc)
include_directories(${CFEPSP_SOURCE_DIR}/fsw/inc)

# Create the app module
add_cfe_app(io_lib ${APP_SRC_FILES})
