from collections import deque
import sys
#import numpy as np

f = open(sys.argv[-1])

d = deque()
dt = deque()
curt = 0.0
count = 0

for l in f:
    a = l.split()
    if len(a) != 2:
        continue
    if l.startswith('U '):
        count += 1
        # dt.append(curt)
        # d.append(int(l.split(' ')[-1])y)
        curt += int(l.split(' ')[-1]) / 1e6
        if count % 100 == 1 and d:
            print count, sum(d) / len(d)
    elif l.startswith('Q '):
        dt.append(curt)
        d.append(int(l.split(' ')[-1]))
        curt += d[-1] / 1e6
    while dt and d and curt - dt[0] > 50.0:
        d.popleft()
        dt.popleft()


#print sum(du) / len(du)
#print sum(dq) / len(dq)

#print sum(dq)
#print sum(du)
# print np.std(du)
# print np.std(dq)
