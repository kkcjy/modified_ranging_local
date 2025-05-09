Local test of modified_ranging without the support of crazyflie

Build both components:
make all

Build specific components:
make drone              # Build only the drone simulator
make center_control     # Build only the control center


Running the System

Start the Control Center
./center_control

Start the Drone Simulator
./drone "port" "local_address"