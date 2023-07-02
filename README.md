OpenFlap
========

**DISCLAIMER:**

**This is still a work in progress. Using the the code and files in this repository to create your own display is not recommended at the moment!**

The OpenFlap project aims to create a open source, affordable split-flap display for the makers and tinkerers of the world.

![OpenFlap Module][module_gif]

The OpenFlap ecosystem consists of 3 different hardware components: the display _modules_, the _controller_ and the _top-con_ board. 

Proof Of Concept Videos
-----------------------

[![Watch the video](https://img.youtube.com/vi/0Jo2y9TDpzU/default.jpg)](https://www.youtube.com/watch?v=0Jo2y9TDpzU)
[![Watch the video](https://img.youtube.com/vi/TG83y_r1YUk/default.jpg)](https://www.youtube.com/watch?v=TG83y_r1YUk)
[![Watch the video](https://img.youtube.com/vi/qD5cp83qmLA/default.jpg)](https://www.youtube.com/watch?v=qD5cp83qmLA)

Design Goals
------------

- 48 characters per module
- Low cost
- Minimal configuration required
- Customizable character sets
- No homing required
- Minimal wiring
- Web interface and api
- Firmware updates through web page

Automatic data routing
----------------------

The _modules_ and _top_con_ boards feature a smart switching mechanism that automatically routes the uart data signal. This reduces the amount of wiring required.

![OpenFlap Wiring][uart_wiring]

Each _module_ and _top-con_ board contains an input that when pulled low, interrupts the default data return path and continues the data path to the next module instead. This is shown in the image above in orange. 

Display Module
--------------
![OpenFlap Module][module]


### Construction

The construction of the OpenFlap _module_ consist of PCB's and 3D-printed parts. Only one of the PCB's is populated. The characters and encoder wheels are also PCB's but they only have solder mask and silkscreen layers and no copper layers.

![OpenFlap Module Explode][module_explode]

### Power
Each _module_ requires 5V for the micro controller and other low voltage components and 12V to power the motor. Typically a stepper motor is used for this kind of application, this project however, uses a DC motor as they are cheaper and require less external components.

#### Signals
Besides power, there are 4 other signals on the top or bottom connecter of the _module_. 

A RX_IN and TX_RET allow uart communication with a _module_ through the top connector. 

The COL_END signal is an input on the bottom connector, this signal becomes grounded when another _module_ is connected below this one. Once this signal is grounded, the returned uart data from the _module_ no longer goes to TX_RET but it goes to TX_OUT instead. TX_OUT will connected with RX_IN of the _module_ below. In that case, TX_RET will become a passthrough for the _module_ below.

![OpenFlap Module Pinout][module_pinout]

Controller
----------

![OpenFlap Controller][controller]

The _controller_ is an ESP32 based board that serves a webpage through which a user can interact with the display.

![OpenFlap Webpage][webpage]

Top-Con
-------

![OpenFlap Top-Con][top_con]

The _OpenFlap display_ modules can be stacked, in this way all modules in a column are automatically wired correctly. The top most module however needs to be connected to the columns next to it or to the _OpenFlap controller_. This is where the _top-con_ board comes in handy. 

The _top-con_ board can be mounted on topmost module of each _module_ stack. This allows the columns to be connected to each other using a simple ribbon cable. 

The _top-con_ board features a 12V to 5V buck convertor and automatic data routing similar to the module boards, the routing is used here to detect the end of the row and not the end of the columns.

UART Communication Protocol
---------------------------

The OpenFlap _modules_ communicate over uart at a baud rate of 115200 bps. The _modules_ are daisy chained, meaning that the TX of the previous _module_ is connected to the RX of the next _module_. There are 4 different types of commands:

- No opperation
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

Controller API
--------------

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

[module_explode]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module_explode.gif "OpenFlap Module Explode"
[module_gif]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module.gif "OpenFlap Module"
[module]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module.png "OpenFlap Module"
[module_pinout]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module_pinout.svg "OpenFlap Module Pinout"
[top_con]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/top_con.png "OpenFlap Top-Con"
[controller]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/flap_controller.png "OpenFlap Controller"
[webpage]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/webpage.png "OpenFlap Webpage"
[uart_wiring]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/OpenFlap_wiring.png "OpenFlap Wiring"
