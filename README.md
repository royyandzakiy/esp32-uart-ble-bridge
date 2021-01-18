# ESP32: UART - BLE Bridge

This is a simple project which uses and ESP32 to act as a bridge to communicate messages received from a UART sender towards a BLE Client receiver, and vice versa.

Communication Diagram:

`Arduino -- (UART) -- ESP32 -- (BLE) -- Android`