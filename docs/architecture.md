# Architecture

The OpenFlap system can be divided into 3 main components:

- The [**OpenFlap Module**](module.md)
- The [**OpenFlap Top Connector**](top_connector.md)
- The [**OpenFlap Controller**](controller.md)

To build an OpenFlap display, you need one OpenFlap Controller for the first column of modules, a Top Connector for all other columns of modules, and some OpenFlap Modules.

This chapter describes how these 3 components works and communicate.

## Display Size restrictions

There are two factors that limit the maximum size of the display:

### 1. The power required to drive all the modules.

The lowest rated connection allows for a maximum current of 5A. At 12V this means that the maximum power that can be delivered to the display is 60W. Each module draws around 1W while moving. This limits the maximum 'safe' display size to around 60 modules. As the modules do not move all the time, it is possible to drive more modules, but this will require testing and is not recommended.

### 2. The memory required by the controller to keep track of all the modules.

The controller keeps an internal representation of the display in memory. The maximum number of modules which can be controlled a the same time without running out of memory is untested.