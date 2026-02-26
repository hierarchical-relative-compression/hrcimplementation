
# 1. GDC2 on Ecoli

## First bad experiment

**Date**: 12/03/2024
**Description**:
We first tried compressing pairs of sequences (1 target, 1 reference) with GDC2. The output was very very bad. We did a compression with a hierarchy. We used a bugged version of the code that fed the FASTA definition line to the fingerprint. The code is legacy, but can be found currently at the bottom of `CompressorGDC2.cpp` (commented out). It can probably be found working in the version of the repo of 12/03/2024. We used the full 219-sequences Ecoli database.

**Location of results**: EcoliDATA/output_219_1st/

**To reproduce**:
We used a version of the database that contained the FASTA definition line, and no line breaks. The line to run would be something like:

	hrlz abs_fasta_219.txt -c output_219_1st --compressor gdc2

REMEMBER: This was in an older version of the code.

**Results**:
The size is comprised of the size of the compressed targets, plus the root file.
To obtain the targets size, we can execute:
	`du -bc ./*.gdc2_rc
To obtain 124.059.991 Bytes.
The root file is #129, with filename 11679_4#53. The size can be found in `11679_4#53_11791_7#5.gdc2_ref`, of 1.439.193 bytes. In total,
	124.059.991 + 1.439.193 = 125.499.184 bytes (125.5 MB)


## Children experiment

**Date**: 15/3/2024
**Description**:
To improve the compression ratio of GDC2, we compress each sequence with all its targets at once. We used the full 219-sequences Ecoli database.

**Location of results**: EcoliDATA/output_219_children

**To reproduce**: 
Compress with GDC2, but change the line in `compress_dataset` to use `compress_hrlz_gdc2_only_children` instead of `compress_hrlz_gdc2_full`.

	hrlz abs_fasta_219.txt -c output_219_children --compressor gdc2

**Results**: 
The size is comprised of the size of the compressed targets, plus the root file.
To obtain the targets size, we can execute:
	`du -bc ./*.gdc2_rc
To obtain 49576919 Bytes. 
The root file is 129 again. The size can be inspected in `11679_4#53.gdc2_ref`, with ``du -b 11679_4#53.gdc2_ref``, 1439193 Bytes
So the total compressed size is 51016112 Bytes (51 MB)

## Full experiment (hrlz)

**Date**: 18/03/2024 and 19/03/2024
**Description**:
To make a fair comparison with GDC2, we compress the Ecoli database by putting the input files of GDC2 in the order that hrlz commands. The compression was done once, still with the bug that fed the FASTA line to HRLZ, and once after the fix (on 19/03/2024)

**Location of results**: First one in EcoliDATA/output_219_gdc2_full, and second one in `EcoliDATA/output_219_2nd`

**Reproduce**:
	`hrlz abs_fasta_219.txt -c output_219_2nd --compressor gdc2`
If we want to use the exact same hierarchy that was found:
	`compress_dataset output_219_2nd -o output_219_2nd --compressor gdc2

**Results:**
Total size of `output_219_gdc2_full`: 5952725 Bytes
Total size of `output_219_2nd`: 5960345 Bytes

## Star graph experiments

**Date**: 19/03/2024
**Description**: To compare GDC2 to itself, we run GDC2 with the original list of files, but put the selected root at the start. We can execute this by changing the list of files and putting the filename of the root at the top. We did this in the experiments with the Ecoli database. However, later in code we will make the parameter --rlz to do this automatically for GDC2.
We tried 5 random roots: out of 219, we chose 114, 171, 216, 34, 45.

**Location of results**: Modified lists of files: under `EcoliDATA/references/references_XX.txt`, where XX=114, 171, 216, 34, 45.
Output of compression: `EcoliDATA/output_gdc2_basic/ref_XX/`

**Reproduce**:
	`hrlz references_XX.txt -c ref_XX --compressor gdc2`
With modern fix:
	`hrlz abs_fasta_219.txt --rlz XX -c ref_XX --compressor gdc2

**Results**:
Ref 114
3509    archive.gdc2_desc
4599188 archive.gdc2_rc
1348881 archive.gdc2_ref
5951578 total

Ref171
3498    archive.gdc2_desc
4622668 archive.gdc2_rc
1351582 archive.gdc2_ref
5977748 total

Ref 216
3511    archive.gdc2_desc
4609654 archive.gdc2_rc
1340110 archive.gdc2_ref
5953275 total

Ref34
3495    archive.gdc2_desc
4550477 archive.gdc2_rc
1413109 archive.gdc2_ref
5967081 total

Ref45
3499    archive.gdc2_desc
4536057 archive.gdc2_rc
1413109 archive.gdc2_ref
5952665 total


# 2. HiRGC on Ecoli

## HRLZ with HiRGC

**Date**: 1/04/2024

**Description**: Compression of Ecoli database with HiRGC. Pair compression. Full database. There are two compressions: the first one we lost the hierarchy, the second one we have it in full. 

**Location of the results**: `EcoliDATA/output_hirgc_219_1` and `EcoliDATA/output_hirgc_219_2`

**Reproduce**: Similar test:
	`hrlz abs_fasta_219.txt -c output_hirgc_219_1 --compressor hirgc`
With the same hierarchy (only for the second one cuz the first one was lost):
	`compress_hierarchy output_hirgc_219_2 -o output_hirgc_219_2 --compressor hirgc`

**Results**:
For compression 1:
87533838        total

For compression 2:
91.488.302        total

## HiRGC but star graph

**Date**: 22/03/2024

**Description**: Compression of Ecoli database with HiRGC but not with hierarchy, but with a chosen relative reference for all targets. Same references chosen for all tests: 114, 171, 216, 34, 45 .

**Location of the results**: `EcoliDATA/output_hirgc_rlz/output_hirgc_rlz_XX`

**Reproduce**: `hrlz abs_fasta_219.txt --rlz XX -c output_hirgc_rlz_XX --compressor hirgc`

**Results**:

Reference 114: 151234766 Bytes total

# 3. SCCG on Ecoli

**Date**: 15/04/2024

**Description**: Compression of the Ecoli database with SCCG. We used the same hierarchy as in the second compression of HiRGC, just for the sake of speed. 

**Location of the results**: `EcoliDATA/output_sccg`

**Reproduce**: `compress_hierarchy EcoliDATA/output_sccg/ -o folder --compressor sccg`

**Results:** targets: 86031868 Bytes, root: 5445012 Bytes, total: 91476880 Bytes

**Date**: 16/04/2024

**Description**: Compression of the Ecoli database with SCCG, fresh hierarchy. The problem with the previous experiment is that we weren't using SCCG to select the best edges for the arboricity, we were using the previously selected ones with another compressor. 

**Location of the results:** `EcoliDATA/output_sccg_hrlz`

**Reproduce**: `compress_hierarchy EcoliDATA/output_sccg_hrlz/ -o folder --compressor sccg`

**Results**: targets: 85705976 Bytes, root: 5445012 Bytes, total: 91150988 Bytes


# Total Ecoli

Original size:

1.131.797.325 Bytes (1.13 GB)

RLZ, hierarchical:

472.126.849 Bytes (472 MB)

HiRGC, hierarchical:

91.488.302 Bytes (91.48 MB)

SCCG, hierarchical:

91.154.298 Bytes (91.15 MB)

# SARS

Original size:

12.017.275.625 Bytes (12.02 GB)

Suffix arrays:

48.068.889.292 Bytes (48.07 GB)

Time for construction of suffix arrays (at 12 cores):
1m37s
## HiRGC, hierarchical on SARS

Compressed size: 

526.425.065 Bytes (526.43 MB)
(metadata 12MB, too big, gotta fix that)

## RLZ, hierarchical on SARS

446.604.077 Bytes (446.60 MB)