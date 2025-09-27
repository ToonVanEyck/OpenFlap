# OpenFlap Module

The OpenFlap module is the main component of the OpenFlap system. It consists of number of 3D prints and PCBs sandwiched together to create a display.

![OpenFlap Module CAD model](images/module_cad.png)

The construction of the module can be described as follows:
A 3D printed 'shell' is sandwiched between two side panel PCBs. A 3D printed bearing is mounted on each of the side panels and the 'flap wheel' is mounted on these bearings. The flap wheel consists of 3D printed core sandwiched between two PCBs. All the flaps rest between these two PCBs. 

Only the left side panel PCB is populated with components. The right side panel PCB is left unpopulated and is used to provide mechanical support. 


!!! note

    In theory, the unpopulated side panel PCB could be replaced with a 3D printed part, this 3D printed part could then also be fused with the 'shell' and on of the 'bearings' greatly simplifying the assembly process and reducing the part count. But it would make the design asymmetrical and I just won't have it...

## Side Panel PCB

Two side panels make up the main structure of the module. The left side panel is populated with components and the right side panel is left unpopulated. The side panel PCB is responsible for the following tasks:

- **Motor Control**: The side panel PCB is responsible for controlling the motor that drives the flaps.
- **Communication**: The side panel PCB is responsible for communicating with the controller and the other modules over UART.
- **Flap Detection**: The side panel PCB is responsible for detecting the position of the flaps.

Each left side panel features a [Puya PY32F003F1](https://download.py32.org/ReferenceManual/en/PY32F003%20Reference%20manual%20v1.1_EN.pdf), this is a low cost 32 bit ARM cortex M0+ microcontroller. 

Three [ITR8307](https://www.everlighteurope.com/custom/files/datasheets/DRX-0000321.pdf) reflective optical sensors are used to detect the position of the flaps. They do this by detecting an [incremental encoder](https://en.wikipedia.org/wiki/Incremental_encoder) pattern on the flap wheel. 

The motor is driven by a [L9110S](https://www.lcsc.com/datasheet/lcsc_datasheet_2203301130_LANKE-L9110S_C2984833.pdf) motor driver. PWM is used to modulate the speed of the motor. 

2x7 Pin headers sit at the top and bottom of the PCB to connect the side panel to the other modules. One pin in the bottom header is always pulled low by the next module below it. When the module is the last in the chain, this pin is pulled high by a pull-up resistor. This way the module can determine if it is the last in a column.

![OpenFlap Side Panel Render](../hardware/module/side_panel/side_panel-3D_blender_top.png)

!!! note

    There is a dependency between the encoder pattern and the position of the IR sensors. The encoder pattern is designed with 48 flaps in mind, this determines the IR sensors placement. If you want to change the number of flaps, you will have to change the encoder pattern and the position of the IR sensors.

### Additional Resources

- [Schematic](../hardware/module/side_panel/side_panel-schematic.pdf)
- [Interactive BOM](../hardware/module/side_panel/side_panel-ibom.html)

## Encoder Wheels PCB

Two encoder wheel PCB sandwiches the 3D printed "core" of the module. The encoder wheels holds all the flaps and allows the the side panel to track it's rotation.

Each encoder wheel has an A & B side. On the A side, the encoder pattern is offset 4.5° relative to the flap holes. On the B side this offset is 3.75°. This allows you tho switch the side in case the flaps are rotating to far / not far enough.

![OpenFlap Encoder Wheels Render](../hardware/module/encoder_wheel/encoder_wheel_48-top.png)

## Flaps

Initially the flaps were designed to be PCB's. But due to the increased accessibility of multicolor 3D printers, the flaps can now also be 3D printed. 

A software tool to generate a custom flap set is included in this repository. As well as script to generate the gerber files or STL files for the flaps. ( TODO: Add link to the tool )

![Flap Dimensions](images/flap_dimensions.svg)

## Shell 

The shell is designed to be 3D printed. Its features include:

- **Mounting holes**: The shell has mounting holes for the side panel PCBs and the top connector.
- **Flap retainer**: The flap retainer, retains the upper flap without covering it up entirely. 
- **Dovetails**: The shell has 2 dovetail cutouts on the back, allowing for flexible mounting options.
- **Ratchet notch**: A notch on the inside of the shell prevents the flap wheel from spinning in the wrong direction.

![Openflap Shell Ratchet Notch](images/ratchet_notch.png)

When the flap wheel spins and finally reaches its setpoint, the motor will spin in reverse for a short pulse. Because the flaps will lock up against the ratchet notch, all modules will end up at the same position height wise. Additionally, this action will take up the backlash in the gearbox, preventing the flaps from sagging.