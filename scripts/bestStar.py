from collections import defaultdict

n, m = map(int, input().split())

d = defaultdict(lambda: 0)
for _ in range(m):
    r, s, w = input().split()
    w = int(w)
    d[r] += w
print(d)
best = min(d, key=d.get)
print(best)
print(d[best])
