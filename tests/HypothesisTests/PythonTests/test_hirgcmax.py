import sys
from hypothesis import given, strategies as st, settings, Verbosity
from test_fasta_generator import fasta_gen, fasta_gen_extras
import conftest
import JSONLayer



"""
Test HiRGCMax with one FASTA file of reference and a FASTA target file
"""

"""
This is the library version. Cannot handle segfaults
@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(deadline=None)
def test_compress_decompress_hirgcmax(fasta_ref, fasta_target):
    #byte_differ = JSONLayer.test_function("test_compress_hirgcmax", [fasta_ref, fasta_target])
    try:
        raw_ref = fasta_ref.encode("ascii")
        raw_target = fasta_target.encode("ascii")
        print(f"Testing ref: {fasta_ref}, target: {fasta_target}")
        byte_differ = conftest.libhrlz.test_compress_hirgcmax(raw_ref, len(raw_ref), raw_target, len(raw_target))
    except BaseException as e:
        print(f"Error while returning string!!! string ref: {fasta_ref}, string target: {fasta_target}", file=sys.stderr)
        print(e, file=sys.stderr)
        assert False
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"
"""

"""
Test HiRGCMax with one FASTA file of reference and a FASTA target file
"""
@given(fasta_ref=fasta_gen(200), fasta_target=fasta_gen(200), ran=st.integers(min_value=1, max_value=2**63-1))
@settings(deadline=None)
def test_compress_decompress_hirgcmax(fasta_ref, fasta_target, ran):
    byte_differ = JSONLayer.test_function("test_compress_hirgcmax", [fasta_ref, fasta_target, 2*ran + 1])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"

"""
Test HiRGCMax with one FASTA file of reference and a FASTA target file,
but both contain random letters and Ns.
"""
@given(fasta_ref = fasta_gen_extras(), fasta_target=fasta_gen_extras(), ran=st.integers(min_value=1, max_value=2**63-1))
@settings(deadline=None)
def test_compress_decompress_hirgcmax_extras(fasta_ref, fasta_target, ran):
    byte_differ = JSONLayer.test_function("test_compress_hirgcmax", [fasta_ref, fasta_target, 2*ran + 1])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"
    
"""
Test HiRGCMax with one FASTA file of reference and a FASTA target file against an old implementation
"""
"""
@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen(), ran=st.integers(min_value=1, max_value=2**63-1))
@settings(deadline=None)
def test_hirgcmax_against_old(fasta_ref, fasta_target, ran):
    byte_differ = JSONLayer.test_function("test_hirgcmax_against_old", [fasta_ref, fasta_target, 2*ran + 1])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"
"""

"""
Test HiRGCMax with one FASTA file of reference and a FASTA target file,
but both contain random letters and Ns.
"""
"""
This is the library version. Cannot handle segfaults
@given(fasta_ref = fasta_gen_extras(), fasta_target=fasta_gen_extras())
@settings(deadline=None)
def test_compress_decompress_hirgcmax_extras(fasta_ref, fasta_target):
    #byte_differ = JSONLayer.test_function("test_compress_hirgcmax", [fasta_ref, fasta_target])
    try:
        raw_ref = fasta_ref.encode("ascii")
        raw_target = fasta_target.encode("ascii")
        byte_differ = conftest.libhrlz.test_compress_hirgcmax(raw_ref, len(raw_ref), raw_target, len(raw_target))
    except BaseException as e:
        print(f"Error while returning string!!! string ref: {fasta_ref}, string target: {fasta_target}", file=sys.stderr)
        print(e, file=sys.stderr)
        assert False
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"
"""
