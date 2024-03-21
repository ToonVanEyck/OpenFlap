# Puya Playground

This is a VS-Code devcontainer based development environment for the Puya PY32 family of ARM Cortex M0 microcontrollers. It's based on [IOsetting's work](https://github.com/IOsetting/py32f0-template) but it has been wrapped in a devcontainer and I changed the build system to CMake.

I have tested this setup with [this devkit from aliexpress](https://nl.aliexpress.com/item/1005004959178538.html).

## Getting Started

Connect the devkit as shown in the image below.

![devkit](./docs/puya_devkit.jpg)

Start the development container

In the development container verify that the debug probe is connected:
```
vscode ➜ /workspace/build $ pyocd list
  #   Probe/Board             Unique ID                                          Target         
------------------------------------------------------------------------------------------------
  0   ARM DAPLink CMSIS-DAP   070000003233d72f083932363836374da5a5a5a597969908   ✖︎ stm32f103rb  
      NUCLEO-F103RB
```

## Building
```
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=puya_toolchain.cmake ..
make
make flash
```

## Debugging
A `launch.json` file is configured to use the debug capabilities of VS-Code 