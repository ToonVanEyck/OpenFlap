## Wiring, signaling and power distribution

The system is wired by connecting the top connectors together with a ribbon cable. The first top connector is connected to the OpenFlap Controller. The OpenFlap Modules can be stacked on top of each other and connected to a top connector.

The top connectors and modules a equipped with a sensor witch allows them to detect if they are connected to another module or top connector. This allows the system to determine the size of the display and simplifies the wiring process.

The image below show the signal path from the OpenFlap Controller through the OpenFlap Modules and back:

![OpenFlap Signal Path](images/signalpath.drawio.png)

The controller can be provided with 12V DC power through a barrel jack. This 12V is passed on to the top connectors and modules through the ribbon cable. The top connectors have a voltage regulator that converts the 12V to 5V. This 5V supply is shared by all the modules connected below the top connector.

The maximum number of modules that can be connected to a single top connector is *T.B.D.*. This is due to the current limitations of the voltage regulator on the top connector.

The maximum number of modules in the entire system is *T.B.D.*. This is due to the current limitations of the PCB traces and the ribbon cable.