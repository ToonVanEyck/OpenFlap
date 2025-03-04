# OpenFlap Controller

The controller is based around the espresif ESP32 microcontroller. The ESP32 is a low-cost, microcontroller with integrated WiFi and Bluetooth capabilities. This makes it ideal for the OpenFlap project.

The controller is responsible for managing and communicating with the modules over UART, as well presenting a web interface and API to the user. 

The controller features an optional OLED, RGB LED and 2 buttons for user interaction. None of these currently serve a purpose in the system, but they can be used for debugging and future features.

![OpenFlap Controller Render](../hardware/controller/controller-3D_blender_top.png)

## Additional Resources

- [Schematic](../hardware/controller/controller-schematic.pdf)
- [Interactive BOM](../hardware/controller/controller-ibom.html)