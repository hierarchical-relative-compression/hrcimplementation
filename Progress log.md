
Before 12/03/2024:
GDC2 functions of compress and delete, and compress and keep, work as the original ones. Did an experiment with the full 219 E.Coli database. The clustering works fast af on the beast because of the massive parallelism. We should try to parallelise GDC2 compression as well, even if it messes up standard output. We got 655 files out, which corresponds to 1 metadata + 3·218 edges of the arboricity, so perfect output.

Tuesday 12/03/2024:
To count the compressed size, as I discussed with Inge, we should not count the references; just the minimum necessary to store the data. If we were to be able to modify GDC2's code, this would mean only the targets should be counted, and then only one reference compressed at the top (the root). Thanks to having the metadata in text format, I can see which sequence is the root (in the first experiment, 129). I can count the .rc files to count only the targets, and then pick one .ref from the root. (Actually there's only one, since for some reason the root has only one child).

We can use the command `du` for disk usage, use the flag `-b` to count the number of bytes, and the flag `-c` to add up file sizes of the list of files we pass it. It allows for regexes. Therefore we can do:

	du -bc ./*.gdc2_rc

To count the file sizes of all the files ending with `.gdc2_rc`. The output was 124059991 bytes.

To find the root is not that easy, metadata has a very long list of filenames. But for now, we can copy them to a kate and find the line number (NOTE: THE INDEX OF THE ROOT IS 0-INDEXED, BUT KATE SHOWS LINE NUMBERS 1-INDEXED). 

The root filename is 11679_4#53. Its only target is 14, with filename 11791_7#5. The compressed root filesize (in 11679_4#53_11791_7#5.gdc2_ref) is 1439193 bytes.

Therefore the total size of the database is 
	124.059.991 + 1.439.193 = 125.499.184 bytes (125.5 MB)

Now we have to compress it with GDC2 itself.


-- Confirmed that the first element of the file list passed on to GDC2 is the reference (c1stage.cpp line 238, picks the first element from the vector of filenames).

--GDC2 was reading filenames on a fixed size buffer of 1024. Hard.

Now, we need to compare GDC2 to itself. We need to generate the file list, and choose a reference to be the first one. We have 219 sequences. We go to random.org and choose 5 random numbers from 1 to 219 both inclusive:

114	171	45	216	34

We generate the correspondent list of files by taking abs_fasta_full.txt (which is just a list of absolute paths), and make 5 copies. In each, we take the line corresponding to the reference, cut it and paste it at the start.

I compiled GDC2 raw on the machine with CMake (I forgot what change it was on the makefile, it is on the work computer). Then, started running the reference_114.txt list of files with GDC2. It turns out that it is slow, but that may be because it is actually compressing well.

It compressed hella well. Its file size is:
5951578 Bytes (5.9 MB)


13/3/24
I refactored the Compressors. Jesus Christ. Never try to blend in your parts with others' parts, the design is just not the same. Just do a new branch.
I used object polymorphism to redirect the API calls to compressors. But the objects do not do anything themselves.

I refactored hrlz to pass the arboricity as a parameter.

I made GDC2 compress all the children on the hierarchy together.

I didn't do the decompression yet, but my gut feeling is that the compression is working.

ECC does not publish its code for GDC2, so we can only speculate about how they did it, or maybe send an email to the creators. They only publish their working code with HiRGC.

From reading GDC2's paper, we can see that it uses ALL sequences that it compresses as a reference for the next sequences in the second phase. In the first phase, it's just a normal RLZ with the main reference. However, it does the second phase in order: a sequence only has available as reference the sequences that have been processed before. Therefore, the order in which it processes the sequences matters, and we can change that with our compression scheme.

I did the initial compression and decompression methods of HiRGC. However, now I have to change them so that they use the arboricity (and the new interface methods). This should not be a big problem. However, HiRGC segfaults on the fasta files of our database.

I conjecture that it segfaults because the fasta files of our database only have one field in the first fasta line, but HiRGC expects more, and it segfaults when it doesn't find them. The best solution is to actually change HiRGC to ignore these fields (just set them to empty strings gosh, is it that difficult?). We will have to, once again, merge the code of HiRGC into our project. At this point, I could be forking those git repositories and uploading my changes. It would be beneficial. Something to do over the weekend. 

15/3/2024

I ran GDC2 on all the children. It seems that it worked fine. The size of all targets compressed, found by `du -bc ./*.gdc2_rc`, is:

49576919 Bytes

The root is again sequence 129 (0-indexed). Its filename is `11679_4#53`, its reference file is of course `11679_4#53.gdc2_ref`, and its file size, found by `du -b 11679_4#53.gdc2_ref` is:

1439193 Bytes

So the total compressed size is:

51016112 Bytes (51 MB)


18/3/24

There's an enormously weird bug in GDC2's decompression of children. I added an executable of the GDC2 code from the project so to run the same code standalone. When decompressing the reference 11658_5#41 after all the previous ones in the hierarchy, it crashes on a free() call inside qsmodel.cpp. qsmodel is called in c1stage decompressor, it creates a qsmodel on the stack, uses a library as interface, computes all, and then frees the objects of qsmodel. If we call it standalone, only on 11658_5#41, it decompresses fine. It's probably a memory corruption from before, like an index out of bounds write. valgrind on decompress would be greatly useful. 

However, we may not need to fix this bug if it doesn't appear on the compression of the full database.

Full compression ongoing.

19/03/24

Realised that HiRGC's code is a piece of crap when it comes to memory management. It crashes because it needs a FASTA file where the DNA is split in lines of at most 1024 characters (terrible). It also creates all the datastructures to hold the data as a max size array of something like 2^24. Terrible code. 

It is so bad that I decided to fit the data to the program rather than to fix the program. I made yet another script to decompress the fasta and keep it with the line breaks.

I found a bug: I was using the fasta line when computing the fingerprints with GDC2. I never put the code in hrlz to ignore the first fasta line lol. I fixed that bug. Now, our code should work with whatever valid FASTA input (I wish all compressors did this lol). By the way, it is slower because we do have to read the file byte by byte. Otherwise, we don't know how much memory do we have to allocate in bulk for a read command. However, it seems that in the optimized release version, the difference is not that noticeable (it is in debug though).

We should be able to optimize this by first creating the huge buffer, and then reading until a newline with ifstream.getline(), which reads directly into a C-style buffer. This should avoid having to check ourselves for whitespace, and makes more sense.

Because of this, I should re-do the experiment of compressing the whole database with GDC2.

I made all the compressions of GDC2 with itself. Just stored the file size really, it's all that matters to us, and we know that basic GDC2 should (should) be correct (we know there's a bug in the decompression, but that aside).

I reworked the code of CompressorHiRGC. I should really make HiRGC work with paths, I don't think it will be that hard (it's just a few lines of using the path library).

It seems that the compression with HiRGC is working after much debugging. It was using some C library functions to read the arguments, that use globals that stay on after one invocation. I cleared that global (optind). The decompression isn't working yet, there seems to be a segfault somewhere.

I added submodules to the project, in particular, I added a submodule for HiRGC. It seems that git support for submodules is a bit capped due to the main repo being in gitlab and the other one in github, and having different logins. The workflow is now to maintain each one independently for now. The advantage is that we keep track of the changes in the projects. I plan to do this too for GDC2, another day (maybe when I get to fix the decompressor).

20/03/24

I went down the rabbit hole of text and binary input in C++, and concluded that the FASTA file format is just bad. Bad because it is a text format, and bad because it can have (or not have) newlines wherever. It is too flexible. This forces us to use formatted input. So, I did formatted input in our project. I also created the test that compares FASTA files together, but only the relevant characters of the sequence.



3/22/24

I made the code for running hirgc as a star graph, and confirmed that it does actually improve the compression, at least on the reference 114 significantly (151MB to 87MB). Will have to compress it with the other references.

I talked to Inge about the problem with the file format, and we agreed on the following: If it is easy to make programs work with the binary read, we should do it. 

My approach: We will preprocess the FASTA files to NOT have newlines. However, this is still a valid FASTA format. Every program that we use should be able to eat this. If there is a program that does not, I will change it so that it does. Hopefully, the change won't take too much time (it did take time on hirgc, holy shit that program was crap). 

Seems that HiRGC has an issue if there are no line breaks at all, in the decompression. Just pure buggy ass code. I've put a line break in the first character, but this might be bad. We might want to put a line break at the end of the file if it is not there.

There's also a weird order of decompression now.

We might want to revert to the last commit. I'm not committing the changes made this evening.


01/04/2024

In the last meeting, we agreed to modify programs so that they use binary reads. The file format that I will use to solve the FASTA problem will be a file with the first FASTA definition line, then a line break, and then all the DNA data WITHOUT LINE BREAKS. This will be enforced in the data by a pre-processing, and will allow programs to read all the DNA in one go.

I fixed some bugs related to changing the directory when using HiRGC, by setting to canonical paths the paths to the output and input directories inside the code for hrlz and file. This is needed if we execute the executable with a relative path from somewhere else.
The fix for HiRGC included a solution inspecting the directory of the executable THAT ONLY WORKS ON LINUX, so our code is not platform-independent. It's fine for now, I think.

The first database that was compressed with HiRGC had a size of:

87533838 Bytes, more or less 87.5 MB.

It was checked that, after decompression, the data was equivalent.

03/04/2024

Before the meeting of 22/03/2024 (before easter), I made a computational test of running HRLZ with a star-like hierarchy. I chose root 144, same as the first root that I used for GDC2's test. I forgot which command, more specifically, which list of sequences I used for the test. Because, for GDC2, I had to manually edit the list of sequences, but for HiRGC and the others it's not necessary.

I tested that the output from the RLZ was correct with this command executed at /home/mpelo/compression/HRLZ/tests (at beast.compute.dtu.dk):

	python compare_file.py ../../DATA/abs_fasta_219.txt ../../DATA/fasta_sequences/ ../../DATA/output_hirgc_decomp_rlz_144/


15/04/2024

I made a bunch of sanity changes to testPipe.cpp for SCCG because it wasn't working on the cluster. The first one is to execute "which java" and record the output, so that we know where java is installed (it wasn't the hardcoded /usr/bin/java in the server cuz I had to install it locally, manually). The second one is to execute "canonical_path("proc/self/exe")" to know the location of the binary, and use it as the classpath for java. This is necessary if we run the executable from a different directory than the one the executable is in (otherwise the cwd changes and Java doesn't find the class). Furthermore, I had to specify the ".7z" extension in the compress_and_delete method cuz it wasn't finding the file to delete. Annoying that I had to put it in two different places.

I ran the first experiment with SCCG, compressing Ecoli with the same hierarchy than hirgc because I was having bugs and computing the hierarchy was taking forever with SCCG. (I think in general it is quite slow).


16/12/24

# BIG UPDATE

### The beast cluster changes to two versions of HiRGC the 16/12/24 (last work week December 2024) 

### HiRGC has a flag to use the Arithmetic Compressor, but will always output a file with extension .7z even if it is not a 7z file so to not break existing code

A refactor could help the above nonsense, but the aim is to not have to refactor the code because we will only use our own code (HiRGCMax) in the end, after the performance update.