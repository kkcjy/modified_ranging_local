import re
import numpy as np
import matplotlib.pyplot as plt

def read_data(file_path, address=None):
    data = []
    patterns = {
        'ModifiedD': r'ModifiedD\s*=\s*([\d.]+)',
        'ClassicD': r'ClassicD\s*=\s*([\d.]+)',
        # 'TrueD': r'TrueD\s*=\s*([\d.]+)',
        'TrueD': r'TrueD\s*=\s*(-?[\d.]+)',             # not support trueD now
        'time': r'time\s*=\s*(\d+)'
    }
    address_pattern = r'\[current_(\d+)\]:'

    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if address is not None:
                addr_match = re.search(address_pattern, line)
                if not addr_match or int(addr_match.group(1)) != address:
                    continue
            current_entry = {}
            for key, pattern in patterns.items():
                match = re.search(pattern, line)
                if match:
                    value = float(match.group(1)) if key != 'time' else int(match.group(1))
                    current_entry[key] = value
            if len(current_entry) == 4:
                data.append((
                    current_entry['ModifiedD'],
                    current_entry['ClassicD'],
                    current_entry['TrueD'],
                    current_entry['time']
                ))
    return np.array(data, dtype=np.float64)  

def plot_raw_data(data):
    plt.figure(figsize=(12, 8))
    time = data[:, 3]
    mod_data = data[:, 0]
    cls_data = data[:, 1]
    tru_data = data[:, 2]

    plt.plot(time, tru_data, 'g-^', label='TrueD', alpha=0.8, linewidth=1.5)
    plt.plot(time, mod_data, 'b-o', label='ModifiedD', alpha=0.8)
    plt.plot(time, cls_data, 'r-s', label='ClassicD', alpha=0.8)

    plt.title('Raw Distance Data')
    plt.xlabel('Time')
    plt.ylabel('Distance')
    plt.legend(loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    filename = 'data/data.png'
    plt.savefig(filename)
    plt.show()

if __name__ == "__main__":
    file_path = 'data/dataLog.txt'
    ADDRESS = 34698  
    data = read_data(file_path, address=ADDRESS)
    if len(data) == 0:
        print("Warning: No valid data found!")
        exit()
    plot_raw_data(data)