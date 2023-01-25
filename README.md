# nRF Robot War firmware

The project uses three separate firmwares. Two of them run on the gateway device, the nRF9160 DK, one for the nRF9160 SiP and one for the nRF52840 SoC. The third one runs on the nRF52840 DK acting as the robot. All the firmware is built using Application Event Manager system provided in the nRF Connect SDK.

The nRF9160 SiP firmware is responsible for communicating with AWS over LTE. It sets up and manages the AWS connection, subscribes to relevant topics, receives configuration updates, and reports the state of the system back to AWS. It also relays configuration updates and messages for the robots to the gateway's nRF52840 SoC over UART.

The nRF52840 SoC on the gateway is responsible for communicating directly with the robots over Bluetooth Mesh.

The different firmwares can be found in [`applications/robot_wars`](./applications/robot_wars) where the source code in the [`gateway` folder](./applications/robot_wars/gateway) is for the nRF91670 DK, the source code in the [`gateway_bridge` folder](./applications/robot_wars/gateway_bridge) implements the communication between the nRF9160 DK and the nRF52840 DK and the [`robot` folder](./applications/robot_wars/robot) contains the source code running on the nRF52840 DK.
