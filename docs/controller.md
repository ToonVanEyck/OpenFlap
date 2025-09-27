# OpenFlap Controller

The controller is a special kind of Top Connector. It is meant to be used for the first column of modules in the display. It's based around the espresif ESP32-S3, a low-cost, microcontroller with integrated WiFi and Bluetooth capabilities. 
The controller is responsible for managing and communicating with the modules over UART, as well presenting a web interface and API to the user. 

![OpenFlap Controller Render](../hardware/controller/controller-3D_blender_top.png)

The controller must be powered from a 12V power supply using the barrel jack connector. A USB port is provided for programming and debugging.

## Additional Resources

- [Schematic](../hardware/controller/controller-schematic.pdf)
- [Interactive BOM](../hardware/controller/controller-ibom.html)
- [KiCanvas](https://kicanvas.org/?github=https%3A%2F%2Fgithub.com%2FToonVanEyck%2FOpenFlap%2Fblob%2Fmaster%2Fhardware%2Fcontroller%2Fsrc%2Fcontroller.kicad_pro)