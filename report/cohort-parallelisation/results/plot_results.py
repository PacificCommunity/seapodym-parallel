import pandas as pd
import re
import glob
import seaborn as sns
import matplotlib.pyplot as plt

data = {
    'step_milliseconds': [],
    'num_doubles': [],
    'workers': [],
    'speedup_avg': [],
    'speedup_std': [],
}

for f in glob.glob("nm*_nd*/resultsNw*.csv"):
    print(f)
    workers = int(re.search(r'Nw(\d+)', f).group(1))
    num_doubles = int(re.search(r'nm(\d+)_nd(\d+)', f).group(2))
    step_milliseconds = int(re.search(r'nm(\d+)_nd(\d+)', f).group(1))
    print(f'workers: {workers}, num_doubles: {num_doubles}, step_milliseconds: {step_milliseconds}')
    
    df = pd.read_csv(f)
    data['speedup_avg'].append(df['speedup'].mean())
    data['speedup_std'].append(df['speedup'].std())
    data['step_milliseconds'].append(step_milliseconds)
    data['num_doubles'].append(num_doubles)
    data['workers'].append(workers)
    
df = pd.DataFrame(data)
print(df)

#sns.set_theme(style="whitegrid")
plt.figure(figsize=(10, 6))

#sns.lineplot(data=df, x='workers', y='speedup_avg', hue='num_doubles', marker='o', err_style="bars", ci='sd')
df1 = df[(df['num_doubles'] == 10000) & (df['step_milliseconds'] == 100)].sort_values(by='workers', ascending=True)
x = df1['workers']
y = df1['speedup_avg']
yerr = df1['speedup_std']
plt.errorbar(x, y, yerr=yerr, label='10000 doubles/100ms per step', marker='o', capsize=5)

df2 = df[(df['num_doubles'] == 15000) & (df['step_milliseconds'] == 10)].sort_values(by='workers', ascending=True)
x = df2['workers']
y = df2['speedup_avg']
yerr = df2['speedup_std']
plt.errorbar(x, y, yerr=yerr, label='15000 doubles/10ms per step', marker='x', capsize=5)

plt.title('Speedup vs Number of Workers')
plt.xlabel('Number of Workers')
plt.ylabel('Speedup')
plt.legend(title='Number of Doubles')
plt.savefig('speedup_vs_workers.png')
plt.show()