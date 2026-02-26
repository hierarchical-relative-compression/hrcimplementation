import sys
import os
from multiprocessing.dummy import Pool
from subprocess import run, PIPE
from pathlib import Path

def format_edges(edges):
    out = []
    for (r,s,w) in edges:
        out.append(f"{r} {s} {w}")
    return out

def gen_input(n, root, edges):
    input = "1\n"
    input += f"{n} {len(edges)} {root}\n"
    input += "\n".join(format_edges(edges))
    return input

def run_arbor(input):
    stdout = run(["arborescence"], input=input.encode(), capture_output=True).stdout.decode("ascii")
    line = stdout.split("\n")[0]
    return int(line.split(" ")[0])

name_to_num = dict()
num_to_name = dict()
results = dict()
edges = []

num = 0
    
def get_num(v):
    global num
    if v not in name_to_num:
        name_to_num[v] = num
        num_to_name[num] = v
        num += 1
    return name_to_num[v]

def get_name(vnum):
    global num
    return num_to_name[vnum]


m = int(input())
for _ in range(m):
    r, s, w = input().split()
    r_num = get_num(r)
    s_num = get_num(s)
    w = int(w)
    if r_num == s_num:
        continue
    edges.append((r_num, s_num, w))


p = Pool(12)

resultslist = p.map(lambda x: (x[0], run_arbor(gen_input(num, x[1], edges))), name_to_num.items())

results = dict()
for (k, v) in resultslist:
    results[k] = v
    print(f"{k} {v}")

best = min(results, key=results.get)
print(f"best: {best} {results[best]}")
