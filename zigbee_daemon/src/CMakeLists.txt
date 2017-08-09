cmake_minimum_required(VERSION 2.8)
project(zigbee_daemon)
SET(TARGET zigbee_daemon)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m32")
include_directories(
		comm  control-bridge  devices utils  zigbee
        )
include_directories(/usr/include/sys)
#link_directories(./lib)

set(SOURCE_FILES
        main.c
		comm/zigbee_cloud.c comm/zigbee_discovery.c  comm/zigbee_socket.c
        control-bridge/serial.c  control-bridge/serial_link.c  control-bridge/zigbee_control_bridge.c
		utils/list.c  utils/mthread.c  utils/zigbee_sqlite.c
		devices/zigbee_devices.c
		devices/door_lock_controller.c devices/coordinator.c devices/door_lock.c
		zigbee/zigbee_node.c
		utils/zigbee_button.c
        )

#add_library(libsrp.a STATIC)

add_executable(${TARGET} ${SOURCE_FILES} )
target_link_libraries(${TARGET} -lpthread -ljson-c -lsqlite3)
