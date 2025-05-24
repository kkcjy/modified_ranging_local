import re
import numpy as np
import matplotlib.pyplot as plt

def modified_time(data, file_path):
    real_times = []
    timestamps = []
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or not line[0].isdigit():
                continue
            parts = line.split()
            if len(parts) >= 4:
                real_times.append(parts[0])
                timestamps.append(int(parts[3]))
    time_labels = []
    for i in range(len(data)):
        data_timestamp = int(data[i, 2]) if data.shape[1] > 2 else 0  # 你的data第三列是time
        idx = np.argmin(np.abs(np.array(timestamps) - data_timestamp))
        tstr = real_times[idx][-10:-4]
        time_labels.append(tstr[:2] + ':' + tstr[2:4] + ':' + tstr[4:6])
    return time_labels

def read_data(file_path, addr_pair):
    # addr_pair: (addr1, addr2)
    data = []
    pattern = r'\[current-(\d+)-(\d+)\]: ModifiedD = ([\d.]+), ClassicD = ([\d.]+), time = (\d+)'
    with open(file_path, 'r') as f:
        for line in f:
            line = line.strip()
            m = re.match(pattern, line)
            if m:
                addr_a, addr_b = int(m.group(1)), int(m.group(2))
                if (addr_a, addr_b) == addr_pair:
                    mod = float(m.group(3))
                    cls = float(m.group(4))
                    t = float(m.group(5))
                    data.append((mod, cls, t))
    return np.array(data)

def plot_multi_lines(time_labels, datas, labels):
    plt.figure(figsize=(14, 8))
    x = np.arange(len(time_labels))
    for i, data in enumerate(datas):
        if len(data) > 0:
            plt.plot(x[:len(data)], data[:, 0], label=f'{labels[i]} ModifiedD')
    plt.title('Multi Drone Distance')
    plt.xlabel('real_time')
    plt.ylabel('distance')
    plt.legend(loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.xticks(x[::max(1, len(x)//10)], time_labels[::max(1, len(x)//10)], rotation=45)
    plt.savefig('data/data_process.png')
    plt.show()

if __name__ == "__main__":
    file_path = 'data/modified_Log.txt'
    flight_Log_path = 'data/flight_Log.txt'
    pairs = [(34697, 34698), (34698, 34699), (34699, 34697)]
    labels = ['34697-34698', '34698-34699', '34699-34697']
    datas = [read_data(file_path, pair) for pair in pairs]
    min_len = min([len(d) for d in datas if len(d) > 0])
    datas = [d[:min_len] for d in datas]
    time_labels = modified_time(datas[0], flight_Log_path)
    plot_multi_lines(time_labels, datas, labels)