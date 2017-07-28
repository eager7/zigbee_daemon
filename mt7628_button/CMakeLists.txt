cmake_minimum_required(VERSION 3.5)
project(linux_driver)
SET(TARGET linux_driver)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m32")
include_directories(
        button_driver/src
        button_application/src/
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/linux-3.10.14-p112871/include
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14-p112871/arch/mips/include
)
include_directories(/usr/include/sys)
link_directories(./lib)

set(SOURCE_FILES  
        button_driver/src/button_driver.c
        button_application/src/button_application.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/linux-3.10.14-p112871/drivers/mtd/ubi/cdev.c
        )

#add_library(libsrp.a STATIC)
add_definitions(-DCONFIG_RALINK_MT7628 -DCONFIG_PRINTK -DMODULE
        -DMODULES -DCONFIG_KERNEL_PRINTK_TIME -CONFIG_RALINK_GPIO
        -DCONFIG_EVA
        )
add_executable(${TARGET} ${SOURCE_FILES} )
#target_link_libraries(${TARGET} -lpthread)
