# nRF Robot War firmware
 
Turn-based strategy game using robots based on the [nRF52840 development kit](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk). Game is played from a web app at [www.robotwar.cloud](https://robotwar.cloud). The robots are controlled over Bluetooth mesh to ease scalability, and an [nRF9160 development kit](https://www.nordicsemi.com/Products/Development-hardware/nrf9160-dk) acts as a gateway between AWS IoT over LTE and Bluetooth mesh.

This repository contains firmwares for both the gateway and the robots. Other repositories in this project are:
- Web App: https://github.com/NordicPlayground/robot-war-app
- Robot shield PCB: https://github.com/NordicPlayground/robot-war-shield
- AWS IoT backend: https://github.com/NordicPlayground/robot-war-cloud

-------------------
## This repository

The repo contains three separate firmwares which can be found in the [applications/robot_wars directory](/applications/robot_wars/):
- [gateway](/applications/robot_wars/gateway/)
  - Firmware for the nRF9160 in the gateway. Handles LTE connection, communication with AWS IoT, translating desired state from the device shadow into robot operations, and reporting robot state to AWS IoT.
  - Currently supports the following boards
    - ```nrf9160dk_nrf9160_ns``` Unsure if revision < 0.14.1 will work out of the box.
  
- [gateway_bridge](/applications/robot_wars/gateway_bridge/)
  - Firmware for the nRF52840 in the gateway. Receives robot operations from the gateway nRF9160 and sends them to the robots over Bluetooth mesh. Also receives messages from the mesh network and sends them to the gateway nRF9160.
    - Idealy this firmware would be replaced by the [HCI LPUART sample](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/samples/bluetooth/hci_lpuart/README.html) from nRF Connect SDK, but due to stability issues it is currently not an option.
  - Currently supports the following boards
    - ```nrf9160dk_nrf52840``` Unsure if revision < 0.14.1 will work out of the box.

- [robot](/applications/robot_wars/robot/)
  - Firmware for the nRF52840 controlling an individual robot. Receives commands from the mesh network, executes them, and reports back the results. 
  - Currently supports the following boards
    - ```nrf52840dk_nrf52840```

Common includes, primarily for defining mesh models, can be found in the [include directory](/include/).

## Documentation
Most of the firmware documentation can be found in the [docs directory](/docs). This documentation includes:
- [JSON schema for the device shadow state](/docs/gw_state_schema.json) 
- [High level description of the different firmware modules](/docs/nRF_Robot_war_firmware_module_description.md)
