
# Ecoli Dataset


I think all of these results were made with the 2log_2 n threshold.

### HiRGC:

| Dataset             | Method       | Root ID | Compressed size | Compression time |
| :------------------ | :----------- | :------ | --------------: | ---------------: |
| ./abs_fasta_219.txt | Hierarchical | 129     |        88.87 MB |         4.23 min |

| compress_to_file | arborescence | compute_edge_weights | construct_graph |
| ---------------: | -----------: | -------------------: | --------------: |
|         2.12 min |       2.6 ms |             1.23 min |         54.61 s |


### iDoComp:

| Dataset             | Method       | Root ID | Compressed size | Compression time |
| :------------------ | :----------- | :------ | --------------: | ---------------: |
| ./abs_fasta_219.txt | Hierarchical | 129     |          308 MB |         8.29 min |

| compress_to_file | arborescence | compute_edge_weights | Printing graph | construct_graph |
| ---------------: | -----------: | -------------------: | -------------: | --------------: |
|            3 min |       859 us |             4.31 min |         658 us |          58.2 s |


# SARS Dataset

Using 2log_2 n threshold:

### HiRGC:

| Dataset              | Method       | Root ID | Compressed size | Compression time |
| :------------------- | :----------- | :------ | --------------: | ---------------: |
| ./abs_fasta_400k.txt | Hierarchical | 17898   |       438.38 MB |        49.77 min |


| compress_to_file | arborescence | compute_edge_weights | construct_graph |
| ---------------: | -----------: | -------------------: | --------------: |
| 6.84 min | 8.99 s | 4.71 min | 38.01 min |

### iDoComp:

| Dataset                  | Method       | Root ID | Compressed size | Compression time |
| :----------------------- | :----------- | :------ | --------------: | ---------------: |
| ./abs_sequences_400k.txt | Hierarchical | 266606  |          139 MB |           44 min |

| compress_to_file | arborescence | compute_edge_weights | Printing graph | construct_graph |
| ---------------: | -----------: | -------------------: | -------------: | --------------: |
|          1.2 min |       8.41 s |             4.63 min |         2.12 s |          38 min |


Using 2sqrt n threshold:

### HiRGC:

| Dataset                  | Method       | Root ID | Compressed size | Compression time |
| :----------------------- | :----------- | :------ | --------------: | ---------------: |
| ./abs_sequences_400k.txt | Hierarchical | 316303  |          175 MB |           6.82 h |

| compress_to_file | arborescence | compute_edge_weights | Printing graph | construct_graph |
| ---------------: | -----------: | -------------------: | -------------: | --------------: |
|         4.52 min |     2.87 min |               6.47 h |         35.1 s |          12 min |

### iDoComp:

| Dataset                  | Method       | Root ID | Compressed size | Compression time |
| :----------------------- | :----------- | :------ | --------------: | ---------------: |
| ./abs_sequences_400k.txt | Hierarchical | 367285  |          104 MB |           2.68 h |

| compress_to_file | arborescence | compute_edge_weights | Printing graph | construct_graph |
| ---------------: | -----------: | -------------------: | -------------: | --------------: |
|         1.61 min |     3.13 min |               2.37 h |         35.7 s |        12.2 min |





| Dataset            | Full Size | HiRGC Hierarchy, log | HiRGC Hierarchy, sqrt | HiRGC, Reference (star graph) | iDoComp Hierarchy, log | iDoComp Hierarchy, sqrt | iDoComp, Reference (star graph) |
| :----------------- | :-------- | -------------------: | --------------------: | ----------------------------: | ---------------------: | ----------------------: | ------------------------------- |
| Ecoli (full), size | 1.1 GB    |             88.87 MB |                       |  Best of 0,5% random:  156 MB |                308 MB* |                         |                                 |
| SARS (full), size  | 12 GB     |            438.38 MB |                175 MB |   Best of 0,5% random: 188 MB |                 139 MB |                  104 MB |                                 |



| Dataset            | HiRGC Hierarchy, log | HiRGC Hierarchy, sqrt | HiRGC, Reference (star graph)  | iDoComp Hierarchy, log | iDoComp Hierarchy, sqrt | iDoComp, Reference (star graph) |
| :----------------- | -------------------: | --------------------: | -----------------------------: | ---------------------: | ----------------------: | ------------------------------- |
| Ecoli (full), time |             4.23 min |                       | Best of 0,5% random: 2.12 min* |              8.29 min* |                         |                                 |
| SARS (full), time  |            49.77 min |                6.82 h | Best of 0,5% random: 2.25 min* |                 38 min |                  2.68 h |                                 |
