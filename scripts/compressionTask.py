from dataclasses import dataclass
from subprocess import run 
from pathlib import Path
from typing import List

@dataclass()
class CompressionTask:
    reference: Path
    files: List[Path]
    gen_infile_path: Path
    stdout: Path
    stderr: Path

    def _gen_infile(self):
        with open(self.gen_infile_path, "w") as f:
            f.write(str(self.reference) + "\n")
            for file in self.files:
                f.write(f"{str(file)}\n")

    def execute(self):
        self._gen_infile()
        with open(self.stdout, "w") as stdout, open(self.stderr, "w") as stderr:
            run(["rlz", self.gen_infile_path], stdout=stdout, stderr=stderr)
        with open(self.stdout, "r") as stdout:
            results = stdout.readline().split(",")
            cumulativefac = [int(results[i * 3 + 1]) for i in range(len(self.files))]
            fac = [cumulativefac[0]] + [cumulativefac[i] - cumulativefac[i - 1] for i in range(1, len(self.files))]
            return self, fac
