# ESP32 RC Tx/Rx software for remote control vehicles

## Why?

I don't want to pay over two digits for a controller/receiver combo.

Radio transmission is not a very valuable commodity in 2020. Hobbyist controllers from China are starting to become popular while being very cheap.

## Basics

Basic aspects of the application are controlled by the main .ino file. Most of the configuration should be moved there, really.

#### controller.h

Uploading the controller is as simple as leaving #define CONTROLLER uncommented.

The header contains analog and digital pin configuration for buttons and variable resistors.
FloatingAnalog handles mapping from Xbox One replacement thumbsticks to transmitter compatible 1000-2000 ranges.
DigitalSwitch turns push buttons into digital switches, cheapening the project further.
The controller is designed for a TFT screen, allowing the user to view telemetry, controller and radio information data at a glance.

#### receiver.h

Uploading the receiver is as simple as commenting out #define CONTROLLER.

The receiver takes advantage of the fastest available protocol, SBus by default. 
Configure the MAC address and TX pin correctly. 
The receiver should then work out of the box.

Telemetry support is being developed.
SmartPort seems to be the most flexible protocol, with telemetry datatype discovery by scanning over a given range.
The message format lends itself to vendor specific alterations and the simplicity of the wire protcol should allow data to be transmitted in odd ways.
