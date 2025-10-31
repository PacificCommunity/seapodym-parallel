import re
import glob

import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from datetime import datetime
import pandas as pd


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
            m = re.match(pat_end, line)
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
print(f"Total tasks recorded: {len(data)}")

ptrn = re.compile(r'\[([^\]]+)\]\s*\[manager_(\d+)\]\s*\[info\]\s*Worker (\d+) - Task (\d+): step (\d+) done')
steps = []
for logfile in glob.glob('log_manager*.txt'):
    for line in open(logfile).readlines():
        m = re.match(ptrn, line)
        if m:
            t = m.group(1)
            workerId = m.group(3)
            task = m.group(4)
            step = m.group(5)
            steps.append({'Worker': workerId, 'Task': task, "Step": step, "Time": t})
print(f"Total steps recorded: {len(steps)}")
         
    

# sort the data by task
data.sort(key=lambda x: x["Task"])
steps.sort(key=lambda x: x["Task"])


# Convert time strings to datetime objects
for task in data:
    task["Start"] = datetime.strptime(task["Start"], "%Y-%m-%d %H:%M:%S.%f")
    task["End"] = datetime.strptime(task["End"], "%Y-%m-%d %H:%M:%S.%f")

steps = pd.DataFrame(steps).astype({"Worker":int, "Task":int, "Step":int})
steps = steps.set_index("Task")
steps["Time"] = pd.to_datetime(steps["Time"])



# Determine the earliest start time
min_start = min(task["Start"] for task in data)

# Map workers to y-axis positions
workers = sorted(set(task["Worker"] for task in data))
worker_pos = {worker: i for i, worker in enumerate(workers)}

# Create the plot
fig, ax = plt.subplots(figsize=(10, 6))

# Plot each task
for task in data:
    start_sec = (task["Start"] - min_start).total_seconds()
    end_sec = (task["End"] - min_start).total_seconds()
    duration = end_sec - start_sec
    ax.barh(worker_pos[task["Worker"]], duration, left=start_sec, height=1, label=task["Task"])
    ax.text(start_sec + duration/2, worker_pos[task["Worker"]], str(task["Task"]),
            va='center', ha='center', color="black", fontsize=8, fontweight='bold')
    
    subset = steps.loc[task["Task"]]
    if isinstance(subset, pd.Series):
        subset = subset.to_frame().T
    for _, row in subset.iterrows():
        step = row["Step"]
        time = (row["Time"] - min_start).total_seconds()
        ax.vlines(time, 
                  worker_pos[task["Worker"]] - 0.5, worker_pos[task["Worker"]] + 0.5,
                  colors='black', linestyles='--', linewidth=1)
        
    ax.plot([start_sec, start_sec], [0 - 0.5, task["Worker"] - 0.5,], '--')

# Format axes
ax.set_yticks(list(worker_pos.values()))
ax.set_yticklabels(workers)
ax.set_xlabel("Seconds")
ax.set_ylabel("Worker")
ax.set_title("Gantt Chart of Task Execution per Worker")
#ax.xaxis_date()
#ax.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M:%S"))

# Remove duplicate legend entries
#handles, labels = ax.get_legend_handles_labels()
#unique = dict(zip(labels, handles))
#ax.legend(unique.values(), unique.keys(), title="Tasks", loc="upper right")

# Use integer ticks for seconds
ax.xaxis.set_major_locator(ticker.MaxNLocator(integer=True))

plt.tight_layout()
plt.savefig('gantt.png')
plt.show()

