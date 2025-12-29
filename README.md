# Wireless Charger

This repository contains the project for the development and management of a Wireless Charger system.

## Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Contributing](#contributing)

## Introduction
The Wireless Charger project aims to provide an efficient and reliable solution for wireless power transfer to compatible devices. The project focuses on embedded software and hardware integration to ensure safe charging, optimal power management, and ease of use.

## Features
- Wireless power transfer management
- Automatic device detection and charging control
- Power regulation and efficiency optimization
- Safety features (overvoltage, overcurrent, and thermal protection)
- Status monitoring and diagnostic capabilities
- Logging and debugging support

## Installation
To install the project, follow these steps:
1. Clone the repository:
    ```sh
    git clone https://github.com/eas-engineering/Wireless_charger.git
    ```
2. Navigate to the project directory:
    ```sh
    cd Wireless_Charger
    ```
3. Build the project using your preferred build system (e.g., CMake or Make).

## Usage
To run the project, follow these steps:
1. Build the project to generate the firmware binary:
    ```sh
    ./build.sh [Debug|Release]
    ```
2. Flash the generated binary to the target hardware using your preferred flashing tool.
3. Power the Wireless Charger and place a compatible device on the charging pad to start charging.

Ensure that all dependencies are correctly configured and the hardware setup complies with the project requirements.

## Contributing
Contributions are welcome and appreciated. To contribute to this project:
1. Fork the repository.
2. Create a new branch for your feature or bugfix.
3. Commit your changes with clear and descriptive messages.
4. Push your branch to your fork.
5. Open a pull request including a detailed description of your changes.
