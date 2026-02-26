"""
Created 18/07/2024
Version 1.0, 18/07/2024

Fixture that starts the Connector and tears it down at closing time.
"""

import pytest
import JSONLayer
import ctypes
import sys

build_path = "../../../build/"

@pytest.fixture(autouse=True, scope='session')
def setup_tests():
    print("Setting up tests!")
    #Setup of the connector:
    JSONLayer.start()
    #Set up the library
    #set_types()
    #We're not using the library for now
    yield
    #Teardown:
    print("Tearing down tests!")
    JSONLayer.end()


def set_types():
    global libhrlz
    libhrlz = ctypes.CDLL(build_path + "tests/HypothesisTests/CppTests/libHRLZTests.so")
    #Set types for the functions
    libhrlz.compress_decompress_arith.restype = ctypes.c_int
    libhrlz.compress_decompress_arith.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong]

    libhrlz.test_compress_hirgcmax.restype = ctypes.c_longlong
    libhrlz.test_compress_hirgcmax.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_ulonglong]

    libhrlz.test_idocomp_compress_decompress.restype = ctypes.c_longlong
    libhrlz.test_idocomp_compress_decompress.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_ulonglong]

    #libhrlz.test_rlz_compress_decompress.restype = ctypes.c_longlong
    #libhrlz.test_rlz_compress_decompress.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_ulonglong]

    libhrlz.test_idocomp_parse_rlz.restype = ctypes.c_ulonglong #pointer to char
    libhrlz.test_idocomp_parse_rlz.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_ulonglong]

    libhrlz.test_idocompmax_parse_subs.restype = ctypes.c_ulonglong
    libhrlz.test_idocompmax_parse_subs.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_ulonglong]
    
    libhrlz.test_idocompmax_insertions_decomp.restype = ctypes.c_ulonglong
    libhrlz.test_idocompmax_insertions_decomp.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong, ctypes.c_char_p, ctypes.c_ulonglong]

    libhrlz.test_partial_sums.restype = ctypes.c_ulonglong
    libhrlz.test_partial_sums.argtypes = [ctypes.c_char_p, ctypes.c_ulonglong]

    libhrlz.free_ptr.argtypes = [ctypes.c_char_p]
    print("Finished setting up the library!")

"""
Makes a python string from the given returned string, and frees
the original C string
"""
def obtain_string(pointer: ctypes.c_ulonglong):
    global libhrlz
    try:
        ret_char_ptr = ctypes.cast(pointer, ctypes.c_char_p)
        ret_str = str(ret_char_ptr.value)
        #Free
        libhrlz.free_ptr(ctypes.ret_char_ptr)
        return ret_str
    except BaseException as e:
        print(f"Error while returning string!!!", file=sys.stderr)
        print(e, file=sys.stderr)
        assert False
