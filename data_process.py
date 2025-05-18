import re
import numpy as np
import matplotlib.pyplot as plt

# def read_data(file_path):
#     data = []
#     current_entry = {}
#     patterns = {
#         'ModifiedD': r'ModifiedD\s*=\s*([\d.]+)',
#         'ClassicD': r'ClassicD\s*=\s*([\d.]+)',
#         'TrueD': r'TrueD\s*=\s*([\d.]+)',
#         'time': r'time\s*=\s*(\d+)'
#     }
    
#     with open(file_path, 'r') as f:
#         for line in f:
#             line = line.strip()
#             for key, pattern in patterns.items():
#                 match = re.search(pattern, line)
#                 if match:
#                     value = float(match.group(1)) if key != 'time' else int(match.group(1))
#                     current_entry[key] = value
#                     if len(current_entry) == 4:
#                         data.append(tuple(current_entry.values()))
#                         current_entry = {}
#     return np.array(data)
def read_data(file_path, address=None):  
    data = []
    current_entry = {}
    patterns = {
        'ModifiedD': r'ModifiedD\s*=\s*([\d.]+)',
        'ClassicD': r'ClassicD\s*=\s*([\d.]+)',
        'TrueD': r'TrueD\s*=\s*([\d.]+)',
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
            
            for key, pattern in patterns.items():
                match = re.search(pattern, line)
                if match:
                    value = float(match.group(1)) if key != 'time' else int(match.group(1))
                    current_entry[key] = value
                    if len(current_entry) == 4:
                        data.append(tuple(current_entry.values()))
                        current_entry = {}
    return np.array(data)

def calculate_offsets(data, left_idx, right_idx):
    if left_idx < 0 or right_idx >= len(data) or left_idx > right_idx:
        raise ValueError(f"Invalid index range: left={left_idx}, right={right_idx}, data length={len(data)}")
    
    selected_data = data[left_idx:right_idx+1]
    mod_offset = np.mean(selected_data[:, 0] - selected_data[:, 2])
    cls_offset = np.mean(selected_data[:, 1] - selected_data[:, 2])
    
    return mod_offset, cls_offset

def plot_adjustment(data, left_idx, right_idx):
    plt.figure(figsize=(12, 8))
    
    time = data[:, 3]
    mod_data = data[:, 0]
    cls_data = data[:, 1]
    tru_data = data[:, 2]
    
    # Calculate offsets
    mod_offset, cls_offset = calculate_offsets(data, left_idx, right_idx)
    
    # Apply offsets
    adjusted_mod = mod_data - mod_offset
    adjusted_cls = cls_data - cls_offset
    
    # Plot the three lines
    plt.plot(time, tru_data, 'g-^', label='TrueD', alpha=0.8, linewidth=1.5)
    plt.plot(time, adjusted_mod, 'b-o', label=f'ModifiedD (Offset={-mod_offset:.2f})', alpha=0.8)
    plt.plot(time, adjusted_cls, 'r-s', label=f'ClassicD (Offset={-cls_offset:.2f})', alpha=0.8)
    
    # Highlight the alignment region
    plt.axvspan(time[left_idx], time[right_idx], color='yellow', alpha=0.2, label='Alignment Region')
    
    plt.title('Vertical Alignment Adjustment')
    plt.xlabel('Time')
    plt.ylabel('Distance')
    plt.legend(loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig('vertical_alignment.png')
    print(f"Alignment comparison chart saved: vertical_alignment.png")
    print(f"Alignment parameters: ModifiedD Offset = {-mod_offset:.4f}, ClassicD Offset = {-cls_offset:.4f}")
    plt.show()

if __name__ == "__main__":
    file_path = 'dataLog.txt'
    
    # Set the data range for alignment (modify these two parameters to select different data segments)
    LEFT_IDX = 0     
    RIGHT_IDX = 20   
    ADDRESS = 34698
    
    data = read_data(file_path, address=ADDRESS)
    if len(data) == 0:
        print("Warning: No valid data found!")
        exit()
    
    if RIGHT_IDX >= len(data):
        RIGHT_IDX = len(data) - 1
        print(f"Warning: End index ({RIGHT_IDX}) exceeds data range, adjusted to {RIGHT_IDX}")
    
    if LEFT_IDX < 0:
        LEFT_IDX = 0
        print(f"Warning: Start index ({LEFT_IDX}) is less than 0, adjusted to 0")
    
    if LEFT_IDX > RIGHT_IDX:
        print(f"Warning: Start index ({LEFT_IDX}) is greater than end index ({RIGHT_IDX}), swapped values")
        LEFT_IDX, RIGHT_IDX = RIGHT_IDX, LEFT_IDX
    
    plot_adjustment(data, LEFT_IDX, RIGHT_IDX)