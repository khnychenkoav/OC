import subprocess
import re

thread_counts = [1, 2, 4, 8, 16, 32]

results = []

exe = 'batchers_sort.exe'

for threads in thread_counts:
    print(f'Running with {threads} threads...')
    process = subprocess.Popen([exe, str(threads)], stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    stdout, stderr = process.communicate()
    match = re.search(r'Sorting time: ([\d\.]+) seconds', stdout)
    if match:
        time = float(match.group(1))
        results.append((threads, time))
        print(f'Sorting time: {time} seconds')
    else:
        print('Error parsing output')

with open('results1.txt', 'w') as f:
    for threads, time in results:
        f.write(f'{threads}\t{time}\n')

print('Test results saved in results1.txt')
