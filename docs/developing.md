# Developing

This repository contains everything you need to build and develop the OpenFlap display system. To effectively develop and contribute to the project, it's important that you are familiar with the following technologies:

- VS-Code devcontainers
- ESP-IDF & FreeRTOS
- ARM Bare Metal development
- KiCad & FreeCAD

## Devcontainers 

All firmware development for this project is done using VS-Code devcontainers. The repository contains the following devcontainers:

### 1) esp-idf

This development container is set up for developing the controller firmware. It contains the ESP-IDF framework and all the required tools to build and flash the firmware to the controller.

### 2) puya 

This development container is set up for developing the module firmware. It contains the ARM toolchain and all the required tools to build and flash the firmware to the module.

### 3) ecad-mcad

This development container is used for automating the generation of PCB production files and 3D printing files. It contains KiCad, FreeCAD and all the required tools to generate the production files. Actual development of the PCB's and 3D models is done outside of the devcontainer.

## Repository Structure

This repository contains everything you need to build and develop the OpenFlap display system. The repository is structured as follows:

- `.devcontainer`: Contains the devcontainer definitions for the 3 development containers.
- `docs`: Contains the documentation for the project.
- `hardware`: Contains all the hardware (PCB) designs for the project.
- `software`: Contains all the software for the project.
- `mechanical`: Contains all the mechanical designs for the project.