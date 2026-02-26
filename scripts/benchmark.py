import sys
import os
from subprocess import run, PIPE
from pathlib import Path

commands = {
    "star": ["hrlz", "--rlz"],
    "opt": ["hrlz", "--opt"],
    "cluster": ["hrlz"]
}

input_file = Path(sys.argv[1])
result_dir = Path(sys.argv[1] + "_result")
result_dir.mkdir()

for name, cmd in commands.items():
    cmd_result_dir = Path(result_dir, name)
    cmd_result_dir.mkdir()
    with open(Path(cmd_result_dir, 'stdout'), 'w') as stdout, open(Path(cmd_result_dir, 'stderr'), 'w') as stderr:
        run(cmd + [f"-c {Path(cmd_result_dir, 'result')}", input_file], stdout=stdout, stderr=stderr)
