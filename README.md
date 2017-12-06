# PiPyOS
### Bare-metal Python for Raspberry Pi

The PiPyOS project aims at providing a bare-metal Python image for the Raspberry Pi, for use in embedded and/or real-time applications. It provides the agile development that is possible with Python, without the overhead of the Linux OS that is commonly employed on the Raspberry Pi. This results in, among others, faster boot-times and more control over the hardware. By using a minimal real-time operating system (RTOS), it is possible to combine a real-time component (written in C) with a high-level application component (written in Python).

At the moment, PiPyOS only runs on the Raspberry Pi 1 (i.e., the BCM2835 based Raspberry Pi's, tested on a model B, possibly also on the model Zero). This limitation is due to the fact that ChibiOS has so far only been ported to this platform. However, it should also be possible to port to the other platforms.

PiPyOS depends on a few components that provide its main functionality:

* A cross-compiler toolchain with the Newlib c-library  
  This is required to build the code.

* [cpython](https://www.python.org)  
  This is the Python interpreter. Its C-source is used unchanged, but the build-process is customized when building from PiPyOS. At the current state, there are a few Python files in the standard library that are adapted, but this should not be necessary in te future.

* [ChibiOS](http://www.chibios.org)  
  This is the real-time OS. It provides a task scheduler with separate tasks for the Python interpreter and for the real-time functionality. It also provides a hardware abstraction layer (HAL) for several hardware components. PiPyOS uses the [Raspberry Pi port by Steven Bate](https://github.com/steve-bate/ChibiOS-RPi).

* [USPi](https://github.com/rsta2/uspi)  
  USPi provides basic USB functionality required for keyboard and ethernet functionality. This is not yet integrated.

In order to make things work, PiPyOS provides the following:

* A SCons build script
  Similar to a Makefile, this script controls the build process, building all the dependencies with the required options.
  
* Newlib interfaces for OS calls
  For calls like *open*, *read*, etc. the Newlib c-library does not provide the OS-functionality, since it is unaware of the OS. Thus, these calls need to be implemented. Some of these calls use ChibiOS to provide the required functionality (e.g. serial port terminal interface, clock, etc), some are manually implemented (e.g. sbrk(); the memory management below malloc()), and others interface to the initfs file system (see below)
  
* Initfs read-only memory filesystem
  Although ChibiOS provides an interface to a FAT filesystem library, this is currently not implemented in PiPyOS as there is no driver for the SD-card (yet). Instead, when building PiPyOS, several required files for starting Python (and additionally, files required for the application) are combined into a binary file. PyPiOS exposes these files (read-only) to Python via the open(), read(), readdir() etc. calls from Newlib
  
## Status and roadmap

The following functionality is currently implemented:

* Starting Python upto the >>> prompt
* Interface via the serial console
* readline
* Framebuffer (screen via HDMI) support
* evaluation of real-time performance

The following functionality is foreseen (in order of development)

* USB keyboard support
* SD-card interface
* FAT filesystem support
* Ethernet support via USB-to-Ethernet (either internal to Raspberry Pi or external for RPi Zero)
* TCP/IP support
* Loadable binary modules / shared libraries

  
  
  
