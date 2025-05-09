Local test of modified_ranging without the support of crazyflie

Build both components:
make all

Build specific components:
make drone              # Build only the drone simulator
make center_control     # Build only the control center


Running the System

Start the Control Center
make run-center

Start the Drone Simulator
make run-drone