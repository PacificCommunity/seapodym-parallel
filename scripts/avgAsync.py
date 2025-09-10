import re
import defopt
import numpy

pat = r'^\s*Rank\s*[^0]\s*checksum\s*\=\s*[\.\de\+\-]+\s*time\:\s*([\.\de\+\-]+)'
patAsync = r'^\s*Async\s+Rank\s*[^0]\s*checksum\s*\=\s*[\.\de\+\-]+\s*time\:\s*([\.\de\+\-]+)'
times = []
def main(resfile: str, *, extract: str=''):
    p = pat
    if extract.lower() == 'async':
        p = patAsync
        
    for line in open(resfile).readlines():
        
        m = re.match(pat, line)
        if m:
            times.append( float(m.group(1)) )


    print(numpy.mean(times))

if __name__ == '__main__':
    defopt.run(main)
        
    
    