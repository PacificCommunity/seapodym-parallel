import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

# obtained on hpc3 11 Jun 2025
results = pd.DataFrame({
    'na' : np.array([10, 20, 50, 100, 200]),
    'nt' : np.array([20, 40, 100, 200, 400]),
    'nw' : np.array([10, 20, 50, 100, 200]),
    'time communication min' : np.array([0.301, 0.603, 1.51, 3.01, 6.02]),
    'time communication max' : np.array([0.302, 0.603, 1.51, 3.02, 6.05]),
    'time computation min' : np.array([0.00117088, 0.00319638, 0.00902281, 0.0360203, 0.0224132]),
    'time computation max' : np.array([0.00199383, 0.0050792, 0.0188001, 0.0762369, 0.419599]),
})
results['time communication mean'] = (results['time communication min'] + results['time communication max']) / 2
results['time computation mean'] = (results['time computation min'] + results['time computation max']) / 2
results['parallel efficiency'] = results['time communication mean'] / (results['time computation mean'] + results['time communication mean'])
x = results['nw']
y = results['parallel efficiency']
plt.plot(x, y)
plt.plot(x, y, 'ro')

plt.title('Parallel Efficiency vs Number of Workers')
plt.xlabel('Number of Workers = num age groups')
plt.ylabel('Parallel Efficiency')
plt.axis([0, 200, 0, 1])
plt.show()
