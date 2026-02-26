from hypothesis import strategies as st, given
import JSONLayer
import sys


@given(s=st.text(st.characters(codec="ascii", min_codepoint=1), min_size=2))
def test_msufsort(s):
    try:
        ret = JSONLayer.test_function("test_msufsort", [s])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert ret == 0
    pass