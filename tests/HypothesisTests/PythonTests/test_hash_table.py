import sys
import JSONLayer
from hypothesis import given, strategies as st

@given(arr=st.lists(st.integers(), min_size=1))
def test_adj_list(arr):
    try:
        ans = JSONLayer.test_function("test_adj_list", [arr])
        if ans == None:
            assert False
        if ans == "ERROR":
            assert False
        assert int(ans) == -1
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False

@given(arr=st.lists(st.integers(), min_size=1), param=st.integers(min_value=1, max_value=2**63-1))
def test_hash_table(arr, param):
    try:
        ans = JSONLayer.test_function("test_agg_multimap", [arr, 2*param + 1])
        if ans == None:
            assert False
        if ans == "ERROR":
            assert False
        assert int(ans) == -1
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False