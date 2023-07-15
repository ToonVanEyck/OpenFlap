Open Flap Module
================

The open flap modules are controlled by a PIC16F15225. This micro controller can be programmed using the (PICkit 4)[https://www.microchip.com/en-us/development-tool/PG164140]. The firmware should be compiled using the microchip (XC8 compiler)[https://www.microchip.com/en-us/tools-resources/develop/mplab-xc-compilers].

Build instructions
------------------

Note: make sure all git submodules are cloned!
```
cd software/module/
mkdir build
cd build
cmake ..
make
```

Flashing with the PICkit 4
--------------------------

```
make flash
```

Preforming an over-the-air update
---------------------------------

```
make update
```