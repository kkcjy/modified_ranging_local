import re
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import KMeans

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

def find_concentrated_region(values, n_clusters=2):
    if len(values) < n_clusters:
        return np.ones_like(values, dtype=bool)
    
    kmeans = KMeans(n_clusters=n_clusters, random_state=0).fit(values.reshape(-1, 1))
    cluster_counts = np.bincount(kmeans.labels_)
    main_cluster = np.argmax(cluster_counts)
    
    cluster_values = values[kmeans.labels_ == main_cluster]
    mean = np.mean(cluster_values)
    std = np.std(cluster_values)
    
    in_region = (np.abs(values - mean) <= std) & (kmeans.labels_ == main_cluster)
    return in_region

def calculate_offsets(data, left_idx, right_idx):
    if left_idx < 0 or right_idx >= len(data) or left_idx > right_idx:
        raise ValueError(f"Invalid index range: left={left_idx}, right={right_idx}, data length={len(data)}")
    
    selected_data = data[left_idx:right_idx+1]
    
    mod_mask = find_concentrated_region(selected_data[:, 0])
    cls_mask = find_concentrated_region(selected_data[:, 1])
    tru_mask = find_concentrated_region(selected_data[:, 2])
    
    common_mask = mod_mask & cls_mask & tru_mask
    
    if not np.any(common_mask):
        common_mask = mod_mask | cls_mask | tru_mask
        print("Warning: No common concentrated region found, using union of individual regions")
    
    mod_offset = np.mean(selected_data[common_mask, 0] - selected_data[common_mask, 2])
    cls_offset = np.mean(selected_data[common_mask, 1] - selected_data[common_mask, 2])
    
    print(f"Used {np.sum(common_mask)}/{len(selected_data)} points in concentrated region")
    return mod_offset, cls_offset, common_mask

def plot_adjustment(data, left_idx, right_idx):
    plt.figure(figsize=(12, 8))
    
    time = data[:, 3]
    mod_data = data[:, 0]
    cls_data = data[:, 1]
    tru_data = data[:, 2]
    
    mod_offset, cls_offset, common_mask = calculate_offsets(data, left_idx, right_idx)
    
    adjusted_mod = mod_data - mod_offset
    adjusted_cls = cls_data - cls_offset
    
    plt.plot(time, tru_data, 'g-^', label='TrueD', alpha=0.8, linewidth=1.5)
    plt.plot(time, adjusted_mod, 'b-o', label=f'ModifiedD (Offset={-mod_offset:.2f})', alpha=0.8)
    plt.plot(time, adjusted_cls, 'r-s', label=f'ClassicD (Offset={-cls_offset:.2f})', alpha=0.8)
    
    plt.axvspan(time[left_idx], time[right_idx], color='yellow', alpha=0.2, label='Alignment Region')
    
    selected_times = time[left_idx:right_idx+1]
    plt.scatter(selected_times[common_mask], tru_data[left_idx:right_idx+1][common_mask],
               color='purple', marker='*', s=100, label='Concentrated Points')
    
    plt.title('Vertical Alignment Using Concentrated Regions')
    plt.xlabel('Time')
    plt.ylabel('Distance')
    plt.legend(loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig('vertical_alignment_concentrated.png')
    print(f"Alignment comparison chart saved: vertical_alignment_concentrated.png")
    print(f"Alignment parameters: ModifiedD Offset = {-mod_offset:.4f}, ClassicD Offset = {-cls_offset:.4f}")
    plt.show()

if __name__ == "__main__":
    # file_path = 'validData/2.txt'
    file_path = 'dataLog.txt'
    
    LEFT_IDX = 0     
    RIGHT_IDX = 30 
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