import pandas as pd
import numpy as np
import defopt
import re

pat = re.compile(r'^\[(\d+)\]\s+@\s+step\:\s+(\d+)\s+checksum\:\s*([\d\\.eE\-\+]+)')

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
    print(checksum1)
    print(checksum2)
    nerrs = 0
    print(f'comparing 1 against 2...')
    for procId in checksum1:
        for step in checksum1[procId]:
            c1 = checksum1[procId][step]
            try:
                c2 = checksum2[procId][step]
            except:
                print(f'could not retrieve [{procId}] @ step {step} from file {output2}!!')
                c2 = -99999
            if c1 != c2:
                print(f'[{procId}] @ step {step}: {c1} != {c2}')
                nerrs += 1
    print(f'number of differences: {nerrs}')

if __name__ == '__main__':
    defopt.run(main)
