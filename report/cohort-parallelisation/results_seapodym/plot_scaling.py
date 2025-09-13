import matplotlib.pyplot as plt
import numpy as np

plt.rcParams.update({'font.size': 12}) # Set the global font size to 18

nprocs = np.array([2, 4, 7, 10, 19, 37])
nworkers = nprocs - 1
manager_times = np.array([59.6544, 18.0884, 7.21377, 5.128, 2.89823, 1.98355])

plt.plot(nworkers, manager_times[0]/manager_times)
plt.plot([1, 36], [1, 36], 'k--')
plt.xlabel('number of workers')
plt.ylabel('speedup relative to 1 worker')
plt.title('seapodym_cohort speedup on AMD Milan CPU (36 age groups)')
plt.savefig('seapodym_cohort_speedup.png')
plt.show()
