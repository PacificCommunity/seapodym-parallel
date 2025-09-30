import re
import glob

import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime


pat_beg = re.compile(r'\[([^\]]+)\]\s*\[(\d+)\]\s*\[info\]\s*Running task (\d+) from step (\d+) to (\d+)')
pat_end = re.compile(r'\[([^\]]+)\]\s*\[(\d+)\]\s*\[info\]\s*Finished task')

data = []
for logfile in glob.glob('log_worker*.txt'):
                   
    in_task  = False
    task = None
    time_beg = None
    workerId = None
    
    for line in open(logfile).readlines():
        
        if in_task:
            m = re.match(pat_beg, line)
            if m: 
                time_end = m.group(1)
                data.append({'Worker': workerId, 'Task': task, "Start": time_beg, "End": time_end})
                in_task = False
                task = None
                time_beg = None
                workerId = None
            
        if not in_task:
            m = re.match(pat_beg, line)
            if m:
                time_beg = m.group(1)
                workerId = int(m.group(2))
                task = int(m.group(3))
                in_task = True
            
print(data)


# Convert time strings to datetime objects
for task in data:
    task["Start"] = datetime.strptime(task["Start"], "%Y-%m-%d %H:%M:%S.%f")
    task["End"] = datetime.strptime(task["End"], "%Y-%m-%d %H:%M:%S.%f")

# Map workers to y-axis positions
workers = sorted(set(task["Worker"] for task in data))
worker_pos = {worker: i for i, worker in enumerate(workers)}

# Create the plot
fig, ax = plt.subplots(figsize=(10, 6))

# Plot each task
for task in data:
    start = mdates.date2num(task["Start"])
    end = mdates.date2num(task["End"])
    duration = end - start
    ax.barh(worker_pos[task["Worker"]], duration, left=start, height=0.4, label=task["Task"])

# Format axes
ax.set_yticks(list(worker_pos.values()))
ax.set_yticklabels(workers)
ax.set_xlabel("Time")
ax.set_ylabel("Worker")
ax.set_title("Gantt Chart of Task Execution per Worker")
ax.xaxis_date()
ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))

# Remove duplicate legend entries
handles, labels = ax.get_legend_handles_labels()
unique = dict(zip(labels, handles))
#ax.legend(unique.values(), unique.keys(), title="Tasks", loc="upper right")

plt.tight_layout()
plt.show()

