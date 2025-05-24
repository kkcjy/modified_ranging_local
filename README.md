# Modified Ranging Local Simulation with Sniffer Timestamp Integration

## Project Overview

This project simulates the full process of drone-to-drone communication using the modified_ranging protocol, driven by timestamp data collected via a sniffer. It supports multi-drone simulation and enables analysis and comparison of different ranging protocols. The workflow includes simulation data driving, protocol operation, log collection, and data visualization.

## Usage Instructions

### 1. Environment Preparation

- Recommended: Linux or macOS
- GCC compiler required (for C code)
- Python 3 with matplotlib, numpy, scipy

### 2. Build the Project

In the project root directory, run:

```sh
make
```

To build individual modules:

- Drone simulator only:
  ```sh
  make drone
  ```
- Center control only:
  ```sh
  make center_control
  ```

### 3. Configuration and Startup

#### Start Center Control

```sh
./center_control
```
The center control listens on port 8888 by default, waiting for drone connections.

#### Start Three Drone Simulators

**You must always start three drones** (e.g., drone1, drone2, drone3):

```sh
./drone 127.0.0.1 drone1
./drone 127.0.0.1 drone2
./drone 127.0.0.1 drone3
```

> **Note:**  
> - Drone names should share a prefix and differ only by number (e.g., drone1, drone2, drone3).  
> - If you change the names, ensure the minimum address among the three drones matches `ADDRESS_BASE` in `defs`, and preferably only the number differs. Otherwise, you may need to modify some code logic.

#### Timestamp-Driven Simulation

The project runs according to timestamp data pre-stored in `flight_Log.txt`. Each run outputs results to `modified_Log.txt`.

### 4. Data Comparison and Visualization

To compare with swarm_ranging protocol or real-world data:

- Place swarm protocol logs in `swarm_Log.txt`
- Place ground-truth distance data in `true_Log.csv`

### 5. Data Processing and Plotting

After simulation, run the data processing script:

```sh
python3 data_process.py
```

The script will read the above log files, plot multi-drone ranging comparison curves, and save the result as `data.png`.

## Directory Structure

- `local_host`: Host initialization and basic configuration
- `base_struct`: Basic data structures and enums
- `ranging_buffer`: Ranging buffer structures and operations
- `table_linked_list`: Message buffer linked list structures and operations
- `ranging_table`: Ranging table structures and operations
- `modified_ranging`: Core algorithm for the modified_ranging protocol
- `socket_frame`: Communication protocol and message structures
- `drone`: Drone simulator main program
- `center_control`: Center control main program
- `data_process.py`: Data processing and visualization script
- `lock`: Thread safety mechanisms
- `debug`: Debug output

## FAQ

- **Build errors:** Ensure all dependencies are installed and the compiler is configured correctly.
- **Connection issues:** Check that drone and center control IP/port settings match.
- **Data processing errors:** Ensure log formats are correct and Python dependencies are installed.

---

For custom simulation workflows or protocol details, please refer to the source code and comments.