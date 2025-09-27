# OpenFlap Top Connector

As the name suggests, the Top Connector is placed at the top of each column of modules. The Top Connector routes power and communication signals to the modules in the column. Top Connectors can be connected together using 2x9 IDC ribbon cables, Allowing the display to be daisy chained to any* width.


![OpenFlap Top Connector Render](../hardware/top_connector/top_connector-3D_blender_top.png)

## Functions

- Receive 12V from previous column and pass it to the next column.
- Convert 12V to 5V using a DC-DC converter and supply it to the modules in the column.
- Route UART signal from previous column through to this column and to the next column.
- Reroute UART back to previous column when this is the last column.

![Top Connector Diagram](images/top_connector_diagram.drawio.png)

## Additional Resources

- [Schematic](../hardware/top_connector/top_connector-schematic.pdf)
- [Interactive BOM](../hardware/top_connector/top_connector-ibom.html)

(*) Power consumption and voltage drop over long cables may limit the practical width of the display.