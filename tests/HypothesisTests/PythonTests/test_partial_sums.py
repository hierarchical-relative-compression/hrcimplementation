import sys
import JSONLayer
from itertools import islice
from hypothesis import given, strategies as st

"""
We are going to create simple partial sums data structure tests.
The main idea is going to be to send an initial array, followed by a sequence
of updates. Between each update, we send a query for each partial sum of the array,
to make sure that all the positions are kept correctly. (Remember that a position "i" of
the array can be found by partial_sum(i)-partial_sum(i-1)). Then, we compare
against a brute-force model of partial sums to check if it's correct. 
"""

#Brute-force partial sums
class PartialSums:

    def __init__(self, init):
        self.array = init
        pass

    def update(self, i, delta):
        if i < 0 or i >= len(self.array):
            raise Exception(f"Index {i} out of bounds for the array of Partial Sums")
        self.array[i] += delta
    
    def query(self, i):
        return sum(self.array[:i+1])
    
    def all_partial_sums(self):
        ret = [self.array[0]]
        for i, e in islice(enumerate(self.array), 1, None): #Iterate from 1
            ret.append(ret[-1] + e)
        return ret

#Strategy to generate random arrays, and random updates in that array.
#updates is a list of tuples (pos, x)
@st.composite
def arr_with_positive_updates(draw, arrs=st.lists(st.integers(min_value=0, max_value=(2**30 - 1)), min_size=1)):
    arr = draw(arrs)
    updates = draw(st.lists(
        st.tuples(
            #We chill updates a bit by only adding up to 2^31-1
            st.integers(min_value=0, max_value=len(arr)-1), st.integers(min_value=0, max_value=(2**30 - 1))
        )
    ))
    return (arr, updates)

@given(tup=arr_with_positive_updates())
def test_arr_positive_updates(tup):
    try:
        arr=tup[0]
        updates=tup[1]
        size = len(arr)
        #Our baseline model
        model = PartialSums(arr)
        #Create array of operations
        #First we query all partial sums without updates
        queries = [f"q({i})" for i in range(size)]
        #Then we send an update, and after each update, full queries
        for pos, x in updates:
            queries.append(f"u({pos},{x})")
            queries.extend([f"q({i})" for i in range(size)])
        
        #Execute
        ans = JSONLayer.test_function("test_partial_sums", [arr, queries])
        #Check first partial sums
        first_ans = ans[:size]
        assert(model.all_partial_sums() == first_ans)
        #Check for each udpate
        i = size
        for pos, x in updates:
            model.update(pos, x)
            assert(model.all_partial_sums() == ans[i:i+size])
            i+=size
    except BaseException as e:
        print(e)
        raise e



#Example
if "__name__"=="__main__":
    arr = [3, 4, 6, 2]

    ps = PartialSums(arr)

    print(ps.all_partial_sums())

    ps.update(1, 7)
    print(ps.query(1))
    print(ps.query(2))
    print(ps.all_partial_sums())
