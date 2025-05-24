import re
import numpy as np
import matplotlib.pyplot as plt

def read_modified_data(log_path, pairs):
    dis_list = [[] for _ in range(3)]
    time_list = [[] for _ in range(3)]
    pattern = re.compile(r"\[(\d)-(\d)\]: ModifiedD = ([\d\.]+), time = (\d+)")
    for idx, (a, b) in enumerate(pairs):
        pass 
    with open(log_path, 'r') as f:
        for line in f:
            m = pattern.search(line)
            if m:
                src, dst = int(m.group(1)), int(m.group(2))
                for i, (a, b) in enumerate(pairs):
                    if (src, dst) == (a, b) or (src, dst) == (b, a):
                        dis = float(m.group(3))
                        t = int(m.group(4))
                        dis_list[i].append(dis)
                        time_list[i].append(t)
                        break
    max_len = max(len(d) for d in dis_list)
    dis_arr = np.full((3, max_len), np.nan)
    time_arr = np.full((3, max_len), np.nan)
    for i in range(3):
        l = len(dis_list[i])
        dis_arr[i, :l] = dis_list[i]
        time_arr[i, :l] = time_list[i]
    return dis_arr, time_arr

# replace the timestamp in the modified log with the corresponding time in the flight log
def modify_realTime(timestamp, flight_log_path):
    ts2hms = {}
    with open(flight_log_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) == 4:
                ts = parts[3]
                hms = parts[0][8:14]
                ts2hms[ts] = hms
    real_timestamp = timestamp.astype('U32')
    it = np.nditer(real_timestamp, flags=['multi_index', 'refs_ok'], op_flags=['readwrite'])
    while not it.finished:
        t = it[0]
        try:
            if t == '' or t == 'nan':
                hms = '------'
            else:
                t_str = str(int(float(t)))
                hms = ts2hms.get(t_str, '------')
        except Exception:
            hms = '------'
        it[0][...] = hms
        it.iternext()
    return real_timestamp

def read_swarm_data(log_path, pairs):
    dis_list = [[] for _ in range(3)]
    time_list = [[] for _ in range(3)]
    for idx, (a, b) in enumerate(pairs):
        pass 
    with open(log_path, 'r') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 5:
                continue
            ts = parts[0][8:14]
            for i in [1, 3]:
                seq = parts[i]
                dis = float(parts[i+1])
                if '-' in seq:
                    src, dst = map(int, seq.split('-'))
                    for j, (a, b) in enumerate(pairs):
                        if (src, dst) == (a, b) or (src, dst) == (b, a):
                            dis_list[j].append(dis)
                            time_list[j].append(ts)
                            break
    max_len = max(len(d) for d in dis_list)
    dis_arr = np.full((3, max_len), np.nan)
    time_arr = np.full((3, max_len), '', dtype='U6')
    for i in range(3):
        l = len(dis_list[i])
        dis_arr[i, :l] = dis_list[i]
        time_arr[i, :l] = time_list[i]
    return dis_arr, time_arr

def hms_to_seconds(hms_arr):
    sec_arr = []
    for hms in hms_arr:
        if isinstance(hms, str) and len(hms) == 6 and hms.isdigit():
            h, m, s = int(hms[:2]), int(hms[2:4]), int(hms[4:6])
            sec_arr.append(h*3600 + m*60 + s)
        else:
            sec_arr.append(np.nan)
    return np.array(sec_arr)

def seconds_to_hms(sec):
    h = int(sec // 3600)
    m = int((sec % 3600) // 60)
    s = int(sec % 60)
    return f"{h:02d}:{m:02d}:{s:02d}"

def plot_multi_ranging(modified_dis, modified_time, swarm_dis, swarm_time):
    plt.figure(figsize=(18, 9))

    colors = ['#0072B2', '#D55E00', '#009E73']
    modified_markers = ['o', 's', 'D'] 
    modified_line_styles = ['-', '--', ':']  
    modified_labels = ['drone1-drone2 (modified)', 'drone1-drone3 (modified)', 'drone2-drone3 (modified)']
    
    swarm_colors = ["#DA0ADA", "#079DF4", '#8c564b']
    swarm_markers = ['x', '+', '*'] 
    swarm_line_styles = ['-.', ':', '--'] 
    swarm_labels = ['drone1-drone2 (swarm)', 'drone1-drone3 (swarm)', 'drone2-drone3 (swarm)']

    modified_secs = hms_to_seconds(modified_time[0])
    swarm_secs = hms_to_seconds(swarm_time[0])
    
    start_modified = np.nanmin(modified_secs)
    start_swarm = np.nanmin(swarm_secs)
    start_time = max(start_modified, start_swarm)
    
    # use start_time + 1 second as new alignment point
    aligned_time = start_time + 1
    
    modified_start_idx = np.where(modified_secs >= aligned_time)[0][0] if len(np.where(modified_secs >= aligned_time)[0]) > 0 else 0
    swarm_start_idx = np.where(swarm_secs >= aligned_time)[0][0] if len(np.where(swarm_secs >= aligned_time)[0]) > 0 else 0
    
    x_modified = np.arange(len(modified_secs) - modified_start_idx)
    for i in range(3):
        y_modified = modified_dis[i][modified_start_idx:]
        plt.plot(x_modified, y_modified, marker=modified_markers[i], color=colors[i], 
                label=modified_labels[i], linewidth=3, markersize=5)
    
    x_swarm = np.arange(len(swarm_secs) - swarm_start_idx)
    for i in range(3):
        y_swarm = swarm_dis[i][swarm_start_idx:]
        plt.plot(x_swarm, y_swarm, linestyle='--', color=swarm_colors[i], 
                marker=swarm_markers[i], label=swarm_labels[i], linewidth=1, markersize=4)

    xtick_pos = []
    xtick_labels = []
    last_sec = None
    for idx, sec in enumerate(modified_secs[modified_start_idx:]):
        if np.isnan(sec):
            continue
        if last_sec is None or sec - last_sec >= 5:
            xtick_pos.append(idx)
            xtick_labels.append(seconds_to_hms(sec))
            last_sec = sec

    plt.xticks(xtick_pos, xtick_labels, rotation=30)
    plt.xlabel(f'Time Sequence (aligned at {seconds_to_hms(aligned_time)})', fontsize=16)
    plt.ylabel('Distance (m)', fontsize=16)
    plt.title('Multi-Drones Distance Comparison (Aligned at First Common Timestamp +1s)', fontsize=18)
    plt.legend(fontsize=12)
    plt.grid(True)
    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    modified_Log_path = 'data/modified_Log.txt'
    swarm_Log_path = 'data/swarm_Log.txt'
    flight_Log_path = 'data/flight_Log.txt'

    pairs = [(1, 0), (2, 0), (1, 2)]

    modified_dis, modified_time = read_modified_data(modified_Log_path, pairs)
    modified_time = modify_realTime(modified_time, flight_Log_path)

    swarm_dis, swarm_time = read_swarm_data(swarm_Log_path, pairs)

    plot_multi_ranging(modified_dis, modified_time, swarm_dis, swarm_time)