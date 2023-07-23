# OpenFlap

**DISCLAIMER:**

**This is still a work in progress. Using the the code and files in this repository to create your own display is not recommended at the moment!**

![OpenFlap Module][module_gif]

The OpenFlap project aims to create a open source, affordable split-flap display for the makers and tinkerers of the world.
This repository houses all the required files to build and program your very own split-flap display. Currently we only support rectangular display configurations. Each display module has 48 characters. The files to produce a basic (alphanumerical) character-set are provided, as well as the resources to produce your own characters sets.

In this document I will layout the steps required to build an exemplary 5 wide, 2 high display. 

## Requirments (E.g.: 5x2 display)

### Tools:
- soldering iron
- 3D printer
- torx T9 screwdriver
- idc crimp tool
- [PICkit 4](https://www.microchip.com/en-us/development-tool/pg164140) for flashing the display modules
- [3.3V serial cable](https://www.adafruit.com/product/954) for flashing an ESP32

### Components

- OpenFlap Controller (1 per display)
- OpenFlap Top_Connector (1 per column E.g.: 5)
- OpenFlap Module (E.g.: 5x2 = 10)
- idc cables 6.5cm (1 per column E.g.: 5)
- A 12V DC power supply with 5.5mm barrel jack

## Building an OpenFlap Display

### Module
![OpenFlap Module][module]

The modules consist of multiple 3D printed parts, sandwiched between two PCB's. Only one of those PCB's should be populated with components. 

All required production files are available in [/hardware/side_panel](/hardware/side_panel), [/hardware/flaps](/hardware/flaps) and [/hardware/encoder_wheel](/hardware/encoder_wheel).

To make one module, you will need:
- 48 flaps ()
- 1 populated side panel
- 1 unpopulated side panel
- 2 encoder wheels 
- 32x [M2.6x8mm screw](https://aliexpress.com/item/1005003094076706.html)
- 1x [12V 60RPM MicroMotor](https://aliexpress.com/item/33022320164.html)
- 1x [motor shaft coupler](https://aliexpress.com/item/4000342135388.html)

When ordering these parts form JLCPCB, their webpage might report errors on the flaps and encoder wheels because they do not contain any "copper layers". You must manually provide this info in the UI:

- flaps : 49mm x 35mm, **0.8mm board thickness**, Remove Order Number!
- encoder wheel: 74mm x 74mm, **Aluminum base material**, Remove Order Number!

3D print these parts, this should be possible without supports:
- [The shell](/3d/module_shell.stl)
- [The core](/3d/module_core.stl)
- [The large bearing](/3d/module_large_bearing.stl)
- [The small bearing](/3d/module_small_bearing.stl)


1) Assemble the *flaps* and the *core* in between the two *encoder wheels*. The printed side of the encode wheels must face outside. Make sure the orientation of the flaps in regard to the core is correct. The motor should be able to fit in the left side when the letters are facing you. Also note that the outer holes and the inner holes of the encoder wheels do not align in each position, make sure they do when you fasten them.

![Flap Assembler][flap_assembler]

2) Slide the motor through the large bearing and attach the shaft coupler.

3) Solder the motor onto the populated side of the *side panel* in such away that the '+' symbol on the motor matches the '+' symbol on the backside of the PCB. Apply solder to the solder bridges marked with the white stripes. If the motor where to turn with an incorrect direction, the solder bridges should be swapped to. Use 4 screws to attach the large bearing to the side panel.

4) Use 4 screws to attach the small bearing to the unpopulated side panel.

5) Insert the *core* into the *shell* and sandwich in between the two *side panels*.

### Controller
![OpenFlap Controller][controller]

The controller hosts a webpage through which the display can be used. The webpage should be accessible through http://openflap.local/. The controller provides an access point on SSID: `OpenFlap` with a default password: `myOpenFlap`. Through the webpage, the controller can be configured to join your local network. (Reboot required)

All required production files are available in [/hardware/controller](/hardware/controller). You might want to solder on [horizontal SMD header pins](https://aliexpress.com/item/32795058236.html) in order to connect your serial cable for programming. 

### Top Connector
![OpenFlap Top-Con][top_con]

You will need 1 _Top Connector_ and 1 idc cable for each column in your display. The _Top Connector_ provides power to the column of displays. in this way the displays don't all require their own power circuitry. The board also helps to rout the data through the modules and back to the controller.

All required production files are available in [/hardware/top_con](/hardware/top_con). Additionally you will need to add the following components manually:

- [2x8 pin 2.54mm, low-profile, bottom-entry Header Socket](https://aliexpress.com/item/1005003263426999.html)
- 2x [2x8 pin box header](https://aliexpress.com/item/1005001400147026.html)
- 2x [2x8 pin idc Female Connector](https://aliexpress.com/item/4001257530318.html)
- 6.5cm / 2.5" [16 strand flatcable](https://aliexpress.com/item/32998004363.html)
- 2x [M2.6x8mm screw](https://aliexpress.com/item/1005003094076706.html)

The notch of the connectors on the cable should point in the same direction. Either both left or both right. The cable length should be around 6.5cm or 2.5".

## Design choices

### Encoder + DC motor
Traditional split-flap displays use stepper motors and a homing sensor to know it's starting position. OpenFlap uses a combination of an optical encoder and a DC motor. This allows the OpenFlap system to boot without having to do a homing rotation to know it's position. Additionally a dc motor is cheaper and simpler to drive.  

### Automatic data routing

The _modules_ and _top_con_ boards feature a smart switching mechanism that automatically routes the uart data signal. This reduces the amount of wiring required.

![OpenFlap Signalpath][uart_signalpath]

Each _module_ and _top-con_ board contains an input that when pulled low, interrupts the default data return path and continues the data path to the next module instead. This is shown in the image above in red (interruped signal path) and green (not interrupted signal path). 

### Construction

The construction of the OpenFlap _module_ consist of PCB's and 3D-printed parts. Only one of the PCB's is populated. The characters and encoder wheels are also PCB's but they only have solder mask and silkscreen layers and no copper layers.

### Power

Each _module_ requires 5V for the micro controller and other low voltage components and 12V to power the motor. The *top connector* boards contain a 12V to 5V buck convertor to power each column. In this way, the modules don't need their own power circuit.

### Local Webpage

![OpenFlap UI][webpage]

![OpenFlap UI][webpage_modules]

## Development Environment 

A VS Code devcontainer is provided in this repository.

To flash the *controller* form inside the devcontainer you will need to forward your serial cable, see the [README.md](/.devcontainer/hardware/README.md).

I was unable to work with the PICkit 4 from inside the devcontainer. You can compile the hex files and flash the from your host machine using MPLABX IPE.

## Uart Interface Controller <--> Module

The OpenFlap _modules_ communicate over uart at a baud rate of 115200 bps. The _modules_ are daisy chained, meaning that the TX of the previous _module_ is connected to the RX of the next _module_. There are 4 different types of commands:

- No operation
- Read property from all modules
- Write property to single module
- Write property to all modules


Value | Definition                | Data Bytes
----- | ------------------------- | -----------
0x01  | firmware_property         | 66
0x02  | command_property          | 1
0x03  | columnEnd_property        | 1
0x04  | characterMapSize_property | 1
0x05  | characterMap_property     | 200
0x06  | offset_property           | 1
0x07  | vtrim_property            | 1
0x08  | character_property        | 1

## Controller HTTP API

Endpoints:

- /api/modules
- /api/wifi
- /firmware

### modules
A http GET request on the `/api/modules` endpoint returns a JSON array containing all module properties. 

```
[
    {
        "module":	0,
        "columnEnd":	true,
        "characterMapSize":	48,
        "characterMap":	[" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "$", "!", "?", ".", ",", ":", "/", "@", "#", "&"],
        "offset":	0,
        "vtrim":	0,
        "character":	"L"
    },{
        ...
    }
]
```

Try it with curl: `curl -X GET http://openflap.local/api/modules`

To write data to the modules, a http POST request can be used. The body of the request should be formatted in the same way as the response from a GET request. You only need to add `module` index and the properties which you want to update.

Try it with curl: `curl -X POST http://openflap.local/api/modules -H 'Content-Type: application/json' -d '[{"module":0,"character":"O"}]'`

### wifi

To change the used wifi credentials, a http POST request must be made to the `/api/wifi` endpoint.

Use these credentials for the hosted access point.
```
{
"host":{
        "ssid": "my_ssid",
        "password": "my_password",
    }
}
```
Use these credentials to connect to an existing an access point.
```
{
"join":{
        "ssid": "my_ssid",
        "password": "my_password",
    }
}
```

A reboot is requeued for these changes to take effect.

[module_explode]: docs/images/module_explode.gif "OpenFlap Module Explode"
[module_gif]: docs/images/module.gif "OpenFlap Module"
[module]: docs/images/module.png "OpenFlap Module"
[module_pinout]: docs/images/module_pinout.svg "OpenFlap Module Pinout"
[top_con]: docs/images/top_con.png "OpenFlap Top-Con"
[controller]: docs/images/flap_controller.png "OpenFlap Controller"
[webpage]: docs/images/webpage.png "OpenFlap Webpage"
[webpage_modules]: docs/images/webpage_modules.png "OpenFlap Webpage"
[uart_signalpath]: docs/images/signalpath.png "OpenFlap Signalath"
[flap_assembler]: docs/images/flap_assembler.png "Assembly tool"