cmake_minimum_required(VERSION 3.5)
project(zigbee_daemon)
SET(TARGET zigbee_daemon)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m32")
include_directories(
		include
		./
		../link_message
        )
include_directories(/usr/include/sys)
link_directories(./lib)

set(SOURCE_FILES
		list.c  mthread.c  serial_link.c   zigbee_control_bridge.c  zigbee_discovery.c  zigbee_socket.c
		main.c  serial.c   zigbee_cloud.c  zigbee_devices.c         zigbee_node.c       zigbee_sqlite.c
		#../link_message/Serial_LinkCommand.c
        )

#add_library(libsrp.a STATIC)

add_executable(${TARGET} ${SOURCE_FILES} )
target_link_libraries(${TARGET} -lpthread -ljson-c -lsqlite3 -lzmq)
