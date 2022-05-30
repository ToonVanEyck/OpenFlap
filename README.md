OpenFlap
========

The OpenFlap project aims to create a open source, affordable split-flap display for the makers and tinkerers of the world.

![OpenFlap Module][module_gif]

The OpenFlap ecosystem consists of 3 different hardware components: the display _modules_, the _controller_ and the _top-con_ board. 

Proof Of Concept Videos
-----------------------

[![Watch the video](https://img.youtube.com/vi/0Jo2y9TDpzU/default.jpg)](https://www.youtube.com/watch?v=0Jo2y9TDpzU)
[![Watch the video](https://img.youtube.com/vi/TG83y_r1YUk/default.jpg)](https://www.youtube.com/watch?v=TG83y_r1YUk)
[![Watch the video](https://img.youtube.com/vi/qD5cp83qmLA/default.jpg)](https://www.youtube.com/watch?v=qD5cp83qmLA)

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
Each _module_ requires 5V for the microcontroller and other low voltage components and 12V to power the motor. Typically a stepper motor is used for this kind of application, this project however, uses a DC motor as they are cheaper and require less external components.

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

The OpenFlap _modules_ communicate over uart at a baud rate of 115200 bps. The _modules_ are daisy chained, meaning that the TX of the previous _module_ is connected to the RX of the next _module_. There are 3 different types of commands:

- Regular command
- Extended command 
- Read Command

The structure of the command and the data that follows depends on the type. But the LSB nibble first byte in the command always denotes the meaning of the command itself. The number of data bytes following the command depends on the command.

If the MSB of the command byte is set, the command is seen as an extended command. This means that the _module_ will execute the command and send the command and it's data to the next _module_. This type of command is useful for writing a firmware update to all  _modules_.

If the MSB is not set, it is seen as a regular command. In this case the _module_ will remember the command for later execution. After the command is received, the module will go into passthrough mode. This means that all subsequent commands and data will be send to the next module. When no data has been received for 250ms, the _module_ will exit passthrough mode and execute the command. This type of command is useful for setting a different character to each _module_.

**get_** Commands usually don't require data bytes, while **set_** commands do require data bytes (the data to be set). When executing a **get_** command, the wanted data is stored in a buffer on the _module_. The contents of this buffer can be retreived by executing the **module_read_data** command. The **module_read_data** command will automatically be converted to an extended command, regardless of the MSB bit value. The **module_read_data** command requires 3 data bytes, the first 2 must be zero when send by the controller. These bytes act as a 16 bit counter, each _module_ that forwards the read command will increment its value. The 3th data byte contains the number of bytes that will be send from the buffer. So when the command has passed through all _modules_ and has returned to the _controller_ it will contain the following bytes:

- byte 0: 0x81 (indicating it is a read command)
- byte 1 - 2 : a 16 bit counter indicating the number of _modules_ in the  display. (**module_cnt**)
- byte 3: The amount of data retrieved from the _modules_. (**data_len**)
- byte 4-(n+4): The data, where n is equal to **module_cnt** x **data_len**

Value | Definition            | Data Bytes
----- | --------------------- | -----------
0x00  | module_do_nothing     | 0          
0x01  | module_read_data      | 3          
0x02  | module_write_page     | 66          
0x03  | module_goto_app       | 0          
0x04  | module_goto_btl       | 0          
0x05  | module_get_config     | 0          
0x06  | module_get_fw_version | 0              
0x07  | module_get_hw_id      | 0          
0x08  | module_get_rev_cnt    | 0          
0x09  | module_set_char       | 4          
0x0A  | module_get_char       | 0          
0x0B  | module_set_charset    | 192          
0x0C  | module_get_charset    | 0         
0x0D  | module_set_offset     | 1  
0x0E  | module_set_vtrim      | 1          

Controller API
--------------

Endpoints:

- /enable
- /reboot
- /offset
- /charset
- /statistics
- /dimensions
- /version
- /message
- /wifi_ap
- /wifi_sta

### enable (R/W)
```
{
    "enable": true
}
```

### reboot (W)
```
{
    "reboot": true
}
```

### offset (W)
```
{
    "dimensions":{
        "width":5,
        "height":1
    },
    "offset": [26,9,6,13,40]
}
```

```
[
    {
        "flap_id":0,
        "mode":"ABS", //default
        "offset":4
    },{
        "flap_id":1,
        "mode":"INC",
        "offset":1
    },{
        "flap_id":1,
        "mode":"DEC",
        "offset":1
    },
    ...
]
```
The flap_id must be given for each object in the array. The default mode is ABS. The default offset value is 0. The offset must be in the inclusive range from 0 to 63.

### vtrim (W)
The vtrim value adds a slight time delay between between when the encoder detects the corret position and when th emotor stops. This allows the display to turn a fraction more to ensure thr flap turns over.
```
{
    "dimensions":{
        "width":5,
        "height":1
    },
    "vtrim": [26,9,6,13,40]
}
```

```
[
    {
        "flap_id":0,
        "mode":"ABS", //default
        "vtrim":4
    },{
        "flap_id":1,
        "mode":"INC",
        "vtrim":1
    },{
        "flap_id":1,
        "mode":"DEC",
        "vtrim":1
    },
    ...
]
```
The flap_id must be given for each object in the array. The default mode is ABS. The default vtrim value is 0. The vtrim must be in the inclusive range from 0 to 63.


### charset (R/W)
```
{
    "flap_id":0,
    "charset":[" ","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","0","1","2","3","4","5","6","7","8","9","€","$","!","?",".",",",":","/","@","#","&"]
}
``` 

### statistics (R)
```
[
    {"flap_id":0,"revolutions":4},
    {"flap_id":1,"revolutions":5},
    {"flap_id":2,"revolutions":6},
    {"flap_id":3,"revolutions":5},
    {"flap_id":4,"revolutions":3}
]
```

### dimensions (R)
```
{
    "dimensions":{
        "width":5,
        "height":1
    }
}
```

### version (R)
```
{
    "controller_firmware_version":"v0.0.0-1-g6fed817*",
    "module_firmware":
    [
        {"flap_id":0, "version":"v0.0.0-1-g6fed817*"},
        {"flap_id":1, "version":"v0.0.0-1-g6fed817*"},
        {"flap_id":2, "version":"v0.0.0-1-g6fed817*"},
        {"flap_id":3, "version":"v0.0.0-1-g6fed817*"},
        {"flap_id":4, "version":"v0.0.0-1-g6fed817*"}
    ]
}
```

### message (R/W)
```
{
    "dimensions":{
        "width":5,
        "height":2
    },
    "message": "HELLOWORLD"
}
```

### wifi_ap (W)
Use these credentials to create a new access point.
```
{
    "ssid": "my_ssid",
    "password": "my_password",
}
```

### wifi_sta (W)
Use these credentials to connect to an existing an access point.
```
{
    "ssid": "my_ssid",
    "password": "my_password",
}
```

[module_explode]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module_explode.gif "OpenFlap Module Explode"
[module_gif]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module.gif "OpenFlap Module"
[module]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module.png "OpenFlap Module"
[module_pinout]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/module_pinout.svg "OpenFlap Module Pinout"
[top_con]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/top_con.png "OpenFlap Top-Con"
[controller]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/flap_controller.png "OpenFlap Controller"
[webpage]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/webpage.png "OpenFlap Webpage"
[uart_wiring]: https://github.com/ToonVanEyck/OpenFlap/blob/master/docs/images/OpenFlap_wiring.png "OpenFlap Wiring"
