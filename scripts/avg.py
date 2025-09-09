import pandas as pd
import defopt

def main(*, filename: str):
    """
    Average the speedup
    @param filename resultsNw*.csv file
    """
    data = pd.read_csv(filename)
    print(f'speedup = {data.speedup.mean()} +/- {data.speedup.std()}')

if __name__ == '__main__':
    defopt.run(main)
