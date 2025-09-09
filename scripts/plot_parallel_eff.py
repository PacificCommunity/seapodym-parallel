import pandas as pd
import glob
import re

data = {
        'branch': [],
        'numWorkers': [],
        'milliseconds': [],
        'numData': [],
        'parallelEff_avg': [],
        'parallelEff_std': [],
        }
for f in glob.glob('*/resultsNw*_nm*_nd*.csv'):
    print(f)
    m = re.match(r'(\w+)\/resultsNw(\d+)_nm(\d+)_nd(\d+)\.csv', f)
    print(m)
    branch = m.group(1)
    nw = int(m.group(2))
    nm = int(m.group(3))
    nd = int(m.group(4))
    d = pd.read_csv(f)
    parallel_eff = d.speedup/d.ideal
    data['branch'].append(branch)
    data['numWorkers'].append(nw)
    data['milliseconds'].append(nm)
    data['numData'].append(nd)
    data['parallelEff_avg'].append(parallel_eff.mean())
    data['parallelEff_std'].append(parallel_eff.std())

data = pd.DataFrame(data)
print(data)

