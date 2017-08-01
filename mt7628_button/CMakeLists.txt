cmake_minimum_required(VERSION 3.5)
project(linux_driver)
SET(TARGET linux_driver)

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -m32")
include_directories(
        button_driver/lib
        button_driver/src
        button_application/src/
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/linux-3.10.14-p112871/include
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14-p112871/arch/mips/include
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/
)
include_directories(/usr/include/sys)
link_directories(./lib)

set(SOURCE_FILES  
        button_driver/src/button_driver.c
        button_application/src/button_application.c
        button_driver/lib/ralink_gpio.c
        button_driver/lib/gpio.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/linux-3.10.14-p112871/drivers/mtd/ubi/cdev.c



        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/cmdline.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/dev-dma.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/dev-i2c.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/dev-nand.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/display.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/ehci_ohci.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/gdb_hook.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/init.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/irq.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/lm.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/malta-amon.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/memory.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/nvram.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/pci.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/power-button.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/printf.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/ralink_wdt.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/reset.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/rt_timer.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/setup.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/time.c
        /home/work/Work/TopBand/DoorLock/Codes/openwrt_mt7628/trunk/build_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/linux-ramips_mt7628/linux-3.10.14/arch/mips/ralink/udc.c
        )

#add_library(libsrp.a STATIC)
add_definitions(-DCONFIG_RALINK_MT7628 -DCONFIG_PRINTK -DMODULE
        -DMODULES -DCONFIG_KERNEL_PRINTK_TIME -CONFIG_RALINK_GPIO
        -DCONFIG_EVA -DRALINK_GPIO_HAS_9532
        )
add_executable(${TARGET} ${SOURCE_FILES} )
#target_link_libraries(${TARGET} -lpthread)
