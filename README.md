# ESP32: UART - BLE Bridge

This is a simple project which uses and ESP32 to act as a bridge to communicate messages received from a UART sender towards a BLE Client receiver, and vice versa.

Communication Diagram:

`Arduino -- (UART) -- ESP32 -- (BLE) -- Android`

## Getting Started

- Clone the repository `git clone https://github.com/royyandzakiy/espidf-arduino-bareminimum`
- Go into the downloaded/cloned repo folder `cd espidf-arduino-bareminimum`
- Download submodules `git submodule update --init --recursive`
- Start exploring!