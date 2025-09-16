import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

plt.rcParams.update({'font.size': 12}) # Set the global font size to 18

data = pd.read_csv('timings.csv')

nworkers = data.nw
manager_times = data.time_manager

plt.plot(nworkers, manager_times[0]/manager_times)
plt.plot(nworkers, manager_times[0]/manager_times, 'ko')
plt.plot([1, 36], [1, 36], 'k--')
plt.xlabel('number of workers')
plt.ylabel('speedup relative to 1 worker')
plt.title('seapodym_cohort speedup on AMD Milan CPU (36 age groups)')
plt.savefig('seapodym_cohort_speedup.png')
plt.show()
