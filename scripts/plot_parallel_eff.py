import pandas as pd
import glob
import re

data = {}
for f in glob.glob('resultsNw*.csv'):
    m = re.match(r'resultsNw(\d+).csv', f)
    nw = int(m.group(1))
    d = pd.read_csv(f)
    d['parallel_eff'] = d.speedup/d.ideal
    data[nw] = {'mean': d['parallel_eff'].mean(), 'std': d['parallel_eff'].std()}
print(data)

