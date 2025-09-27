# OpenFlap

[![Mechanical and Electronics Validation](https://github.com/ToonVanEyck/OpenFlap/actions/workflows/ecad_mcad_ci.yml/badge.svg)](https://github.com/ToonVanEyck/OpenFlap/actions/workflows/ecad_mcad_ci.yml)

**🚨 This is still a work in progress. Using the the code and files in this repository to create your own display is not recommended at the moment! 🚨**

The OpenFlap project aims to create a open source, affordable split-flap display for the makers and tinkerers of the world.
This repository houses all the required files to build, program and modify your very own split-flap display. 

![OpenFlap Module](docs/images/OpenFlap.gif)

## Specifications

- 48 Flaps per module.
- 49mm x 70mm Character size.
- Stackable & Chainable.
- Daisy chain communication.
- Per module configuration & calibration.
- HTTP API available.
- Modular design.
- 3D printable parts.

## Design Philosophy

The OpenFlap system is designed to be modular and expandable, using common components and manufacturing techniques such as 3D printing and printed circuit boards.

The OpenFlap system uses DC geared motors instead of traditional stepper motors. This simplifies the mechanical design and reduces the cost of the system. Additionally this reduces the power consumption of the system and allows the split-flap display to rotate faster than the traditionally used stepper motors. 

The OpenFlap system is designed to be as plug and play as possible. The system is able to determine it's own display size, and should only require calibration once.

The OpenFlap system has a web interface that allows local control of the display. The web interface uses a HTTP API to communicate with the display, this API also allows external services to communicate with the display. No authentication or security measures are implemented at the moment.

The OpenFlap system can connect to your local WiFi network or host it's own network.

## I want to ...

[... know more about how the OpenFlap system works.](docs/architecture.md)

[... build my own OpenFlap display.](todo)

[... contribute to the OpenFlap project.](todo)
