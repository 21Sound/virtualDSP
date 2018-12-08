VirtualDSP
==============================

Turn your soundcard into a real-time DSP by using virtual DSP. Low-Latency duplex streaming which supports many devices and device combinations.

Build
-----

Run CMake without any errors and build the virtualDSP binary with your favourite compiler.

Tested development toolchains:

- Windows 7, 8 and 10 with MinGW-w64-i686 (32Bit), MinGW-w64-x86_64 (64Bit) 6.3.0 compiler via Eclipse Neon and QtCreator, MSVC (32+64Bit) compiler via Visual Studio 14.
- Linux Ubuntu 16.04, Debian 8.7 with GCC via Eclipse Neon and QtCreator.

on x86-32 (Win32) and x86-64 processor architecture.


Functionality
------------

Start the program and choose yoour favourite combination of input/output devices. virtualDSP automatically sets the number of channel strips to the maximum number of output channels.

At the moment, each channel is mapped by modulo logic to the outputs. e.g. if you have two inputs and 8 outputs, the mapping would be 1 to 1, 2 to 2, 1 to 3, 2 to 4, 1 to 5, 2 to 6, 1 to 7, 2 to 8.

Further functionalities that are planned to be implemented:
- Delay
- Channel mapping
- Transfer function graph
- Levelmeter
- ...

Dependencies
-----
This project depends on Qt5, qcustomplot (Source files included) and portaudio. Please ensure to install the appropriate libraries on a system-known path, so CMake can find them while configuring.

If you have any questions, run into bugs, or have feature ideas, please feel free to contact me via email: hagenvontronje1@gmx.de