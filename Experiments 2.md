

# HiRGCMax with Ecoli

## Hierarchical compression:

Description: Compressed the whole Ecoli database with HiRGCMax, using the standard adaptive arithmetic compressor. It uses parallelism both in the stage of finding the best edge weights, but not in the stage of compressing the arborescence. There's no parallelism in the decompression either. It was checked that the decompressed output is the same as the original database.

Original size of Ecoli Database:
1.130.365.197 Bytes = 1.13 GB

Compressed size:
93.189.001 Bytes = 93.2 MB

Compression time:
· C++ timers:
compress_to_file 127.296.520.448 ns           = 2.12 min
arborescence 2.608.302 ns                             
report_result 371.826 ns  
compute_edge_weights 73.607.146.382 ns  = 1.26 min
construct_graph 54.617.129.267 ns               = 0.91 min
total 255.523.776.225 ns                               = 4.25 min
· Real time:
Started the computation at Thu Aug 15 18:37:11 2024
finished computation at Thu Aug 15 18:41:26 2024  
elapsed time: 255.524s = 4.25 min

Decompression time:
decompress 140.577.735.103 ns  = 140 s = 2.33 min
total 140.577.735.103 ns                = 140 s = 2.33 min

## Original compression (without hierarchy)

Compressed with rel114:
156.352.230 Bytes = 156 MB

Compression time:
normal_rlz 127.260.330.922 ns = 127 s = 2.12 min
total 127.260.330.922 ns           = 127 s = 2.12 min


# SARS dataset with HiRGC

## Hierarchical compression

Description: Compressed the whole Ecoli database with HiRGCMax, using the standard adaptive arithmetic compressor. It uses parallelism both in the stage of finding the best edge weights, but not in the stage of compressing the arborescence. There's no parallelism in the decompression either. It was checked that the decompressed output is the same as the original database.

Uncompressed size:
12.017.222.323 Bytes = 12 GB

Compressed size:
459.678.615 Bytes = 460 MB

Compression time:
· C++ timers:
compress_to_file 410.692.437.040 ns              = 410s = 6.8 min
arborescence 8.986.244.808 ns                       =9s
report_result 520.330.455 ns                           = 0.5 s
compute_edge_weights 282.704.865.315 ns  = 282 s   = 4.7 min
construct_graph 2.280.750.950.037 ns           = 2280 s = 38 min
total 2.983.654.827.655 ns                                = 2983 s = 50 min
· Real time:
Started the computation at Thu Aug 15 17:42:08 2024
finished computation at Thu Aug 15 18:31:55 2024
elapsed time: 2986.65s = 50 min

Decompression time:

decompress 458.065.857.596 ns  = 458 s = 7.63 min
total 458.065.857.596 ns               = 458 s = 7.63 min

## Original compression (without hierarchy)

Five randomly picked references from the 400.000 files: #272486, #135697, #179595, #204654, #139042. 

**Reference 272486:**

Compressed size:
188.342.101 Bytes = 188 MB

Compression time:
normal_rlz 135.000.216.790 ns  = 135 s = 2.25 min
total 135.000.216.790 ns            = 135 s = 2.25 min

Decompression time:
decompress 175.147.903.397 ns  = 175 s = 2.92 min
total 175147903397 ns                 = 175 s = 2.92 min

**Reference 135697:**

Compressed size:
3.056.582.778 Bytes = 3GB (lol)

Compression time:
normal_rlz 1.870.746.696.205 ns = 1870 s =  31.16 min
total 1.870.746.696.205 ns = 1870 s = 31.16 min