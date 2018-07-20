# A fake CAN device

This device simulates the CAN device used in ESROCOS for demonstration purposes.
It suffices the Airbus ICD document and uses 3 types of CAN messages for control.
The code is based on pthreads and socket CAN.

## Setup

At first, you have to setup the CAN interface using

> ./setup\_pcan canX 1000000

Afterwards, you should compile and run the fake device

> ./build

> ./fake-dev canX

If the other side successfully transmits data, the output should be:

> [fake-can-dev] Got telemetry request. Sending response

> [fake-can-dev] Velocity mode. Setting velocity to 10922

> [fake-can-dev] Got telemetry request. Sending response

> [fake-can-dev] Velocity mode. Setting velocity to 10922

> [fake-can-dev] Got telemetry request. Sending response
