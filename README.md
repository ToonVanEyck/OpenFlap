OpenFlap
========

The OpenFlap project aims to create a open source, affordable split-flap display for the makers and tinkerers of the world.

![OpenFlap Module Explode][module_explode]

Features
--------

- 48 characters per module
- Low cost (depending on how much money you own...)
- No configuration required
- customizable character sets
- No calibration rotation required
- Minimal wiring
- Web interface
- Firmware updates through web page.

Architecture
------------

The OpenFlap ecosystem consists of 3 different hardware components: the display _modules_, a _controller_ and a _top-con_. The _modules_ and _top_con_ boards feature a smart switching mechanism that automatically routes the uart data signal. This reduces the amount of wiring required.

### Automatic data routing

![OpenFlap Wiring][uart_wiring]

Each _module_ and _top-con_ board contains an input that when pulled low, interrupts the default data return path and continues the data path to the next module instead. This is shown in the image above in orange. 

### Display Module
![OpenFlap Module][module]


#### Construction
The construction of the OpenFlap _module_ consist of PCB's and 3D-printed parts. Only one of the PCB's is populated. The characters and encoder wheels are also PCB's but they only have solder mask and silkscreen layers and no copper layers.

#### Power
Each _module_ requires 5V for the microcontroller and other low voltage components and 12V to power the motor. Typically a stepper motor is used for this kind of application, this project however, uses a DC motor as they are cheaper and require less external components.

##### Signals
Besides power, there are 4 other signals on the top or bottom connecter of the _module_. 

A RX_IN and TX_RET allow uart communication with a _module_ through the top connector. 

The COL_END signal is an input on the bottom connector, this signal becomes grounded when another _module_ is connected below this one. Once this signal is grounded, the returned uart data from the _module_ no longer goes to TX_RET but it goes to TX_OUT instead. TX_OUT will connected with RX_IN of the _module_ below. In that case, TX_RET will become a passthrough for the _module_ below.

![OpenFlap Module Pinout][module_pinout]

### Controller

![OpenFlap Controller][controller]

The _controller_ is an ESP32 based board that serves a webpage through which a user can interact with the display.

![OpenFlap Webpage][webpage]

### Top-Con

![OpenFlap Top-Con][top_con]

The _OpenFlap display_ modules can be stacked, in this way all modules in a column are automatically wired correctly. The top most module however needs to be connected to the columns next to it or to the _OpenFlap controller_. This is where the _top-con_ board comes in handy. 

The _top-con_ board can be mounted on top most module of each column of display _modules_. This allows the columns to be connected to each other using a simple ribbon cable. 

The _top-con_ board features a 12V to 5V buck convertor and automatic data routing similar to the module boards, the routing is used here to detect the end of the row and not the end of the columns.

UART Communication Protocol _ToDo_
----------------------------------

Controller API _ToDo_
---------------------

Bill Of Materials API _ToDo_
----------------------------

Schematics _ToDo_
-----------------

Board designs _ToDo_
--------------------

3D designs _ToDo_
-----------------

Upcoming Changes _ToDo_
-----------------------

[module_explode]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module_explode.gif "OpenFlap Module Explode"
[module]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module.png "OpenFlap Module"
[module_pinout]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module_pinout.svg "OpenFlap Module Pinout"
[top_con]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/top_con.png "OpenFlap Top-Con"
[controller]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/flap_controller.png "OpenFlap Controller"
[webpage]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/webpage.png "OpenFlap Webpage"
[uart_wiring]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/OpenFlap_wiring.svg "OpenFlap Wiring"
