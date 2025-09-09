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

for f in glob.glob("resultsNw*.csv"):
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

for nd in df['num_doubles'].unique():
    for nm in df['step_milliseconds'].unique():
        df1 = df[(df['num_doubles'] == nd) & (df['step_milliseconds'] == nm)].sort_values(by='workers', ascending=True)
        x = df1['workers']
        y = df1['speedup_avg']
        yerr = df1['speedup_std']
        plt.errorbar(x, y, yerr=yerr, label=f'{nd} init dbl, {nm}ms step', marker='o', capsize=5)

plt.plot([df['workers'].min(), df['workers'].max()], [df['workers'].min(), df['workers'].max()], 'k--')

plt.title('Parallel scalability')
plt.xlabel('# workers = # age groups')
plt.ylabel('Speedup')
plt.legend(title='')
plt.savefig('speedup_vs_workers.png')
plt.show()