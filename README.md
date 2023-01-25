# nRF Robot War firmware

The project has three separate but connected firmware. Two of them go on the gateway, the nRF9160 DK, one for the nRF9160 SiP and one for the nRF52840 SoC. The last is for the nRF52840 DK acting as the robots. All the firmware is built on the Application Event Manager system provided in the nRF Connect SDK.

The nRF9160 SiP firmware is responsible for communicating with AWS over LTE. It sets up and manages the AWS connection, subscribes to relevant topics, receives configuration updates, and reports the state of the system back to AWS. It also relays configuration updates and messages for the robots to the gateway's nRF52840 SoC over UART.

The nRF52840 SoC on the gateway is responsible for communicating directly with the robots over Bluetooth Mesh.

The different parts can be found in applications/robot_wars where the **gateway** is for the nRF91670 DK, the **gateway bridge** is for the communication between the nRF9160 DK and the nRF52840 DK and the **robot** is the nRF52840 DK.
