# OpenFlap

**🚨 This is still a work in progress. Using the the code and files in this repository to create your own display is not recommended at the moment! 🚨**

The OpenFlap project aims to create a open source, affordable split-flap display for the makers and tinkerers of the world.
This repository houses all the required files to build, program and modify your very own split-flap display. 

![OpenFlap Module][module_gif] 

## Specifications

- 48 Flaps per module.
- Simple wiring.
- Daisy chain communication.
- Per module configuration & calibration.
- No homing sequence.
- HTTP API available.
- Customizable.
- Stackable design.

## Design Requirements

#### The split-flap display should be affordable but high quality. 
I would rather sink in more time than money.

#### The split-flap display shall not require a homing sequence. 
To achieve this, the OpenFlap modules contain an optical absolute encoder. This encoder allows the module to know it's position at any time.

#### The design shall consist only of printed circuit boards and 3D printable parts. 

#### The amount of wiring to connect multiple modules should be minimal.
The modules and top connector boards feature a smart switching mechanism that automatically routes the UART data signal.

![OpenFlap Signalpath][uart_signalpath]

Each module and top connector board contains an input that when pulled low, interrupts the default data return path and continues the data path to the next module instead. This is shown in the image above in red (interrupted signal path) and green (non interrupted signal path). 

#### The split-flap display shall only requires calibration once.
Each module contains a micro controller capable of storing calibration values.

#### The split-flap display shall be able to determine it's own size and dimensions.
Each module can sense if it is there is another module connected below itself. The controller can query this information and calculate the width and height of the display based on this information. This currently does constrain the system to only support rectangular displays. 

#### The split-flap display must be controllable through a local webpage.
![OpenFlap UI][webpage] 
![OpenFlap UI][webpage_modules] 

## Architecture

The OpenFlap system consists of 3 main components:

1) [OpenFlap Module](#openflap-module)
2) [OpenFlap Controller](#openflap-controller)
3) [OpenFlap Top-Connector](#openflap-top-connector)

The OpenFlap modules are designed to be stackable. Each stack of modules must be topped of with top-connector board, these top-connector boards can be connected together to chain together multiple stacks of modules. A controller board can be connected to the first (left-most) top-connector board, it will serve as the brain for the OpenFlap display.

### OpenFlap Module
![OpenFlap Module][module]

The brains of the OpenFlap module is a PUYA PY32F003x8, this is a cheap but capable ARM-cortex M0. 

The modules consist of multiple 3D printed parts, sandwiched between two PCB's. Only one of those PCB's should be populated with components. 

### OpenFlap Controller
![OpenFlap Controller][controller]

THe OpenFlap controller is build around an ESP-32, this versatile chip provides an easy way to connect the OpenFlap display to the internet.

The controller hosts a webpage through which the display can be used. The webpage should be accessible through http://openflap.local/. The controller provides an access point on SSID: `OpenFlap` with a default password: `myOpenFlap`. Through the webpage, the controller can be configured to join your local network. (Reboot required)

All required production files are available in [/hardware/controller](/hardware/controller). You might want to solder on [horizontal SMD header pins](https://aliexpress.com/item/32795058236.html) in order to connect your serial cable for programming. 

### OpenFlap Top Connector
![OpenFlap Top-Con][top_con]

You will need 1 _Top Connector_ and 1 idc cable for each column in your display. The _Top Connector_ provides power to the column of displays. in this way the displays don't all require their own power circuitry. The board also helps to rout the data through the modules and back to the controller.

[module_gif]: docs/images/module.gif "OpenFlap Module"
[module]: docs/images/module.png "OpenFlap Module"
[top_con]: docs/images/top_con.png "OpenFlap Top-Con"
[controller]: docs/images/flap_controller.png "OpenFlap Controller"
[webpage]: docs/images/webpage.png "OpenFlap Webpage"
[webpage_modules]: docs/images/webpage_modules.png "OpenFlap Webpage"
[uart_signalpath]: docs/images/signalpath.png "OpenFlap Signalath"
