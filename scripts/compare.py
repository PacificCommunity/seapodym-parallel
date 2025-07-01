import pandas as pd
import numpy as np
import defopt

def load(output):
    res = {}
    for line in open(output).readlines():
        m = re.match(pat, line)
        if m:
            procId, step, checksum = int(m.group(1)), int(m.group(2)), float(m.group(3))
            if procId not in res:
                res[procId] = {}
            res[procId][step] = checksum
    return res


def main(*, output1: str, output2: str):
    """
    Compare the checksums of two output files
    @param output1: first output file
    @param output2: second output file
    """
    checksum1 = load(output1)
    checksum2 = load(output2)
    print(f'comparing 1 against 2...')
    for procId in checksum1:
        for step in checksum1[procId]:
            c1 = checksum1[procId][step]
            c2 = checksum2[procId][step]
            if c1 != c2:
                print(f'[{procId}] @ step {step}: {c1} != {c2}')

if __name__ == '__main__':
    defopt.run(main)
