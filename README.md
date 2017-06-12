topband zigbee daemon
----------------------
zigbee的网关程序，测试代码和升级程序，默认运行在OpenWrt系统上。
对应的协调器程序版本为**JN-AN-1223-ZigBee-IoT-Gateway-Control-Bridge-v1008**

#### zigbee_daemon
实现Zigbee网络的管理和控制，里面包含设备管理，和APP端通信，串口通信，zeromq的网络通信，数据库管理几个部分，可酌情裁剪。
运行方式可以用下面命令获取信息：
```
./zigbee_daemon -h
```

#### zigbee_client
zigbee网关的测试程序，有C和Python两个版本，在没有APP的情况下可以使用这个程序和zigbee daemon程序通信测试。

#### zigbee_flash
zigbee芯片的烧录程序，可以将bin文件烧录到芯片中。

#### Doc
部分文档，APP和zigbee_daemon的通信协议文档。



