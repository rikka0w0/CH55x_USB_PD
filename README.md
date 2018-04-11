# CH554 USB PD

This demo implements a USB PD sniffer, which receives PD packets from CC line and send them to PC via USB. (本程序试图实现一个USB PD的抓包器):

Author(作者): Rikka0w0 (小六花)

* `keilc51\CH554.H`, `Delay.C` and `Delay.h` comes from official CH554 demo (WCH), the USB MSD implementation is inspired by the USB MSD demo for STM32 (ST). 
(`keilc51\CH554.H`, `Delay.C`和`Delay.h`是从WCH官方的示例代码里提取的，U盘部分程序参考了STM32的USB MSD示例)
* All file in folder `includes` comes from Github repository [Blinkinlabs's ch554_sdcc](https://github.com/Blinkinlabs/ch554_sdcc).
(`includes`文件夹中的文件是从[Blinkinlabs's ch554_sdcc](https://github.com/Blinkinlabs/ch554_sdcc)里复制的)
* Compiler/IDE: Keil uVersion & SDCC. (编译器/开发环境：Keil 礦ision和SDCC)
* A Wiki page describes how to setup SDCC on Windows and Linux. (Wiki中会介绍如何在Windows和Linux上搭建SDCC编译环境)
* Feel free to use this demo as the template/start of a open-source or non-commercial project, modifying source code and republishing is allowed.
(对于开源项目和非商业项目，可以是用本演示代码作为起点或者模板，作者允许修改并重新发布这份代码)
* However you __MUST__ contact the author for permission prior to use this demo for commercial purposes.
(__如果用于商业目的的话，必须提前得到作者的允许__)

# Hardware Setup(硬件配置):
* The hardware is based on the minimal circuit setup of CH554
(硬件基于CH554的最小系统)
* Chip is directly powered by Vbus, which is approximately 5V
(芯片直接由Vbus驱动，大约是5V)

__ Incomplete yet__

# Capabilities(能做到什么):
The aim is to read PD traffics on the Type-C CC line between a DFP and a UFP, and forward them to PC via USB.
(本项目试图把CC线上的PD通信转发到PC上)
__ Incomplete yet__

# Notes:
* The only difference between CH554 and CH552 is that, CH552 only supports USB device mode while CH554 can also be programmed as a Host. 
So this demo may work on CH552 as well. (CH552只能作为USB的设备，而CH554还可以作为USB主机，这是两种芯片唯一的区别。因此本Demo可能也能在CH552上正常工作)
* 8051 systems including CH554 is big-endian while x86 and ARM processors are little-endian.
(8051系统包括CH554是大端存储的，即数据的高位放在低内存地址中，而x86和ARM处理器都是小端存储的)
* USB packets are little-endian, keep this in mind!
(USB包都是小端的，特别需要注意！) 
* SDCC requires that all interrupt service functions must be written inside the same .c file containing `void main(void)`, 
otherwise SDCC can not find them. According to official SDCC documentation, it's not a bug but a feature. Keil C51 doesn't have this limitaion.
(SDCC要求中断处理函数必须和main函数放在一个.c文件中，否则SDCC没办法找到这些函数。根据SDCC官方文档，这是一个特性不算Bug，Keil C51编译器没有这种限制)