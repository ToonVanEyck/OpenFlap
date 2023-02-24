API documentation
=================

The OpenFlap controller exposes various API endpoints that allow third party software to control and monitor an OpenFlap display. 

API endpoints can support reading and writing, through GET and POST requests.

All data in those requests will be formatted as json data.

Some requests can be indexed by extending the api endpoint with the index of the OpenFlap module that you wish to target. 

Example:

`GET http://openflap.local/api/calibration` will return the calibration data for all OpenFlap modules connected to the controller.

`GET http://openflap.local/api/calibration/5` will only return the calibration data for the 5th OpenFlap module in the display.


Endpoints
---------

Endpoint       | Direction 
-------------- | ----------
/powered       | R/W       
/calibration   | R/W       
/characterMap       | R/W       
/message       | R/W       
/effect        | R/W       
/dimensions    | R         
/version       | R         
/wifi_ap       | W         
/wifi_sta      | W         
/reboot        | W         
/firmware      | W         

### powered
*Read/Write, Not indexable*

This can be used to get and set the state of the relay on the OpenFlap controller.
powered | description
------- | ------------------------------------------------------------
true    | The relay is **closed**, providing power to the modules.
false   | The relay is **open**, disconnecting power from the modules.


#### read-response
```
{
    "powered": true
}
```
*try it:* 
```
curl -X GET http://openflap.local/api/powered
```

#### write-body
```
{
    "powered": true
}
```
*try it:* 
```
curl -X POST http://openflap.local/api/powered -H 'Content-Type: application/json' -d '{"powered":true}'
```

### calibration
*Read/Write, Indexable*

#### read-response
```
[
    {
        "index": 1,
        "offset": 5,
        "vtrim": 6
    },...
]
```

#### write-body

Both `offset` and `vtrim` are optional.

Key        | Behavior
---------- | -------------------------------------------
offset     | Set the encoder offset to this value.
vtrim      | Set the virtual trim to this value.

```
[
    {
        "index": 1,
        "offset": 1,
        "vtrim": 1,
    },...
]
```


### characterMap
*Read/Write, Indexable*

#### read-response
```
[
    {
        "index": 1,
        "characterMap":[" ","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","0","1","2","3","4","5","6","7","8","9","€","$","!","?",".",",",":","/","@","#","&"]
    },...
]
```

#### write-body
```
[
    {
        "index": 1,
        "characterMap":[" ","A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z","0","1","2","3","4","5","6","7","8","9","€","$","!","?",".",",",":","/","@","#","&"]
    },...
]
```


### message
*Read/Write, Not indexable*

#### read-response
```
{
    message:[
        "HELLO",
        "WORLD"
    ]
}
```

#### write-body
```
{
    message:[
        "HELLO",
        "WORLD"
    ]
}
```

The number of strings in the `message` array must exactly match the display `height`, the length of these strings must all be equal and exactly match the display `width`.


### effect
*Read/Write, Not indexable*

*Not Implemented*


### dimensions
*Read, Not indexable*

#### read-response
```
{
    width: 5,
    height: 2
}
```

### version
*Read, Indexable*

#### read-response
```
{
    "controller_firmware_version":"v1.0.0",
    "module_firmware":
    [
        {"index":0, "version":"v1.0.0"},
        ...
    ]
}
```

### wifi_ap
*Write, Not indexable*

#### write-body

```
{
    "ssid": "my_ssid",
    "password": "my_password",
}
```

### wifi_sta
*Write, Not indexable*

#### write-body

```
{
    "ssid": "my_ssid",
    "password": "my_password",
}
```

### reboot
*Write, Not indexable*


### firmware
*Write, Not indexable*
