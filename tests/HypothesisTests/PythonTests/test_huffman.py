import sys
import JSONLayer
from hypothesis import given, strategies as st, settings, Verbosity, example

@settings(deadline=10000)
@given(string=st.text(min_size=50, alphabet=st.characters(codec='ascii', min_codepoint=1)))
#@example(string="""YKI@vxIdT["apZhfE>$'
#69(W<T`N&@MJV(\d'([c0;qSf
#xx'cz_""")
def test_huffman(string):
    ret = 0
    try:
        ret = JSONLayer.test_function("test_huffman", [string])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)

@settings(deadline=10000)
@given(string=st.text(min_size=1, alphabet=st.characters(codec='ascii', min_codepoint=1)), reps=st.integers(min_value=1, max_value=10))
def test_huffman_repetitive(string, reps):
    newstring = string * reps
    ret = 0
    try:
        ret = JSONLayer.test_function("test_huffman", [newstring])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)

@settings(deadline=10000)
@given(string=st.text(min_size=50, alphabet=st.characters(codec='ascii', min_codepoint=1)))
def test_huffman_api(string):
    ret = 0
    try:
        ret = JSONLayer.test_function("test_huffman_api", [string])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)

@settings(deadline=10000)
@given(string=st.text(min_size=1, alphabet=st.characters(codec='ascii', min_codepoint=1)), reps=st.integers(min_value=1, max_value=10))
def test_huffman_repetitive_api(string, reps):
    newstring = string * reps
    ret = 0
    try:
        ret = JSONLayer.test_function("test_huffman_api", [newstring])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)

@settings(deadline=10000)
@given(string=st.text(min_size=50, alphabet=st.characters(codec='ascii', min_codepoint=1)))
def test_fse_mem(string):
    ret = 0
    try:
        ret = JSONLayer.test_function("test_fse_mem", [string])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)

@settings(deadline=10000)
@given(string=st.text(min_size=1, alphabet=st.characters(codec='ascii', min_codepoint=1)), reps=st.integers(min_value=1, max_value=10))
def test_fse_repetitive(string, reps):
    newstring = string * reps
    ret = 0
    try:
        ret = JSONLayer.test_function("test_fse_mem", [newstring])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)
    