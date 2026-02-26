
# SARS database

It has 400.000 vertices. 2log n = 36 (floored). With linear growth we have:


| function             | output        | ratio edges/vertices | time                          |
| -------------------- | ------------- | -------------------- | ----------------------------- |
| $2\log n * 1.1 * it$ | test_1_1.txt  | 25                   | Between 1h and 2h but unknown |
| $2\log n * it^2$     | test_quad.txt | 27,23                | real 80m, user 2458m, sys 8m  |

Now, we run the cutoff after each round, and not after each choosing of representatives, which was after 10 rounds.

| function         | output                        | ratio edges/vertices | time                         |
| ---------------- | ----------------------------- | -------------------- | ---------------------------- |
| $2\log n * it^2$ | test_quad_round.txt           | 33,58                | real 41m, user 1253m, sys 6m |
| $2\log n * it^2$ | output_dir_rlz/log_output.txt | 35,5                 | real 40m                     |
