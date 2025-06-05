cmake_minimum_required(VERSION 3.19)
cmake_policy(VERSION 3.19...3.22)

### include app.cmake (necessary everytime) ###
include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)
##############
### Block of including project folders
set(STLINK_UPLOAD_ENABLED TRUE)


set(MBED_APP_JSON_PATH mbed_app.json5)

set(CUSTOM_TARGETS_JSON_PATH custom_targets.json5)

include(mbed-os/tools/cmake/mbed_toolchain_setup.cmake)
#set(MBED_SKIP_LINK_SCRIPT TRUE)
project(Secuenciador # here you can change your project name
    LANGUAGES C CXX ASM) 
include(mbed_project_setup)

add_subdirectory(TARGET_BLUEPILL)
add_subdirectory(mbed-os)

add_executable(${PROJECT_NAME} main.cpp)
#target_link_options(${PROJECT_NAME} PRIVATE
#"-T${CMAKE_CURRENT_SOURCE_DIR}/TARGET_BLUEPILL/STM32F103C8.ld"
#)
target_link_libraries(${PROJECT_NAME} mbed-baremetal mbed-usb) # Can also link to mbed-baremetal here mbed-usb

mbed_set_post_build(${PROJECT_NAME})