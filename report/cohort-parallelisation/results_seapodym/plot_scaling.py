import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

plt.rcParams.update({'font.size': 12}) # Set the global font size to 18

data = pd.read_csv('timings.csv')

legs = []
for na in data['na'].unique():
    nworkers = data[ data['na'] == na ].nw.to_numpy()
    manager_times = data[ data['na'] == na ].time_manager.to_numpy()
    plt.plot(nworkers, manager_times[0]/manager_times)
    legs.append(f'na={na}')
    
plt.plot([1, 20], [1, 20], 'k--')
legs.append('ideal')

plt.legend(legs)

for na in data['na'].unique():
    nworkers = data[ data['na'] == na ].nw.to_numpy()
    manager_times = data[ data['na'] == na ].time_manager.to_numpy()
    plt.plot(nworkers, manager_times[0]/manager_times, 'kx')
    print('-'*40)


plt.xlabel('number of workers')
plt.ylabel('speedup relative to 1 worker')
plt.title('seapodym_cohort speedup')
plt.savefig('seapodym_cohort_speedup.png')
plt.show()
