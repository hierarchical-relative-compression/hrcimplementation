@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(verbosity=Verbosity.verbose)
def test_compress_decompress_idocomp(fasta_ref, fasta_target):
    byte_differ = JSONLayer.test_function("test_idocomp_compress_decompress", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"

@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(deadline=1000)
def test_compress_decompress_sccg(fasta_ref, fasta_target):
    byte_differ = JSONLayer.test_function("test_compress_sccg", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"

@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(deadline=None)
def test_compress_decompress_gdc2(fasta_ref, fasta_target):
    byte_differ = JSONLayer.test_function("test_compress_gdc2", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"
