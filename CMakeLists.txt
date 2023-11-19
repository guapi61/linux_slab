cmake_minimum_required(VERSION 3.0.0)
project(demo VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/user/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/common/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/common/log/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/test/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/mqttclient/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/mqtt/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/platform/linux/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/network)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/lvgl/src/extra/libs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/lvgl/src/extra/libs/sjpg/)
include_directories(/home/book/100ask_stm32mp157_pro-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/include/glib-2.0/)
include_directories(/home/book/100ask_stm32mp157_pro-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/glib-2.0/include/)
include_directories(/home/book/100ask_stm32mp157_pro-sdk/ToolChain/arm-buildroot-linux-gnueabihf_sdk-buildroot/arm-buildroot-linux-gnueabihf/sysroot/usr/include/dbus-1.0/)


link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib/)
link_libraries(turbojpeg)
link_libraries(mqttclient)
link_libraries(pthread)
link_libraries(bluetooth)
link_libraries(glib-2.0)
link_libraries(dbus-glib-1)
link_libraries(m)
link_libraries(gio-2.0)
link_libraries(gobject-2.0)
link_libraries(dbus-1)


file(GLOB_RECURSE SRC_FILE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/user/*.c  
    ${CMAKE_CURRENT_SOURCE_DIR}/include/*.c
)

add_executable(demo main.c ${SRC_FILE})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
