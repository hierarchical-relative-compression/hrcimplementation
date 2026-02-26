import sys
import JSONLayer
from hypothesis import given, strategies as st, settings, Verbosity, example
from test_fasta_generator import fasta_gen, just_dna_gen

@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(verbosity=Verbosity.verbose)
def test_rlz_compress_decompress(fasta_ref, fasta_target):
    byte_differ = JSONLayer.test_function("test_rlz_compress_decompress", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"