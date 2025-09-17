import re
import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

data = {
    'worker': [],
    'time_init': [],
    'time_step': [],
    'time_comm': [],
}

for line in open('log.txt').readlines():
    
    m = re.match(r'^\[(\d+)\] Timings calc/step/overhead/init/comm:\s*([\d\.]+)/\s*([\d\.]+)/\s*([\d\.]+)/\s*([\d\.]+)/\s*([\d\.]+)', line)
    if m:
        worker = int(m.group(1))
        tinit = float(m.group(5))
        tstep = float(m.group(3))
        tcomm = float(m.group(6))
        data['worker'].append(worker)
        data['time_init'].append(tinit)
        data['time_step'].append(tstep)
        data['time_comm'].append(tcomm)
        
data = pd.DataFrame(data)
# remove the manager
data = data[ data.worker > 0 ]
print(data)

sns.histplot(data, x = 'time_init', color='blue')
sns.histplot(data, x = 'time_step', color='cyan')
sns.histplot(data, x = 'time_comm', color='magenta')
plt.xlabel('time s')
plt.legend(['init', 'step', 'comm'])
plt.title('Histogram of execution times across workers')
plt.savefig('seapodym_cohort_times.png')
plt.show()