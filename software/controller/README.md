# Controller firmware development with ESP-IDF

To get started, make sure you have build and opened the `esp-idf` devcontainer in VS-Code.


!!! info
    
    All commands below are to be run from the root of the repository.

!!! note

    You may set the `OPENFLAP_DEFAULT_SSID` and `OPENFLAP_DEFAULT_PASSWORD` environment variables to override the default WiFi SSID and password for the controller. If not set, the controller will default to `default_ssid` and `default_password`.

## Build the firmware

```bash
idf.py -B build/controller -C software/controller build
```

## Flash the firmware

```bash
idf.py -B build/controller -C software/controller flash monitor
```

!!! note 

    The first time you program the controller, you will need to use the UART0 RX/TX pads on the bottom of the board. After the initial programming, you can use the USB port to program and debug the controller.+

## Action Buttons

The VS Code devcontainer contains a toolbar with some useful actions for building and flashing the firmware.

![ESP-IDF Action Buttons](../../docs/images/esp-idf-dev-toolbar.png)