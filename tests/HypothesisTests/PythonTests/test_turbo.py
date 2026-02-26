import sys
import JSONLayer
from hypothesis import given, strategies as st, settings, Verbosity, example

@settings(deadline=10000)
@given(string=st.text(min_size=50, alphabet=st.characters(codec='ascii', min_codepoint=1)))
@example(string="""YKI@vxIdT["apZhfE>$'
69(W<T`N&@MJV(\d'([c0;qSf
xx'cz_""")
def test_turbo(string):
    ret = 0
    try:
        ret = JSONLayer.test_function("test_turbo", [string, len(string), "-Fc"])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)