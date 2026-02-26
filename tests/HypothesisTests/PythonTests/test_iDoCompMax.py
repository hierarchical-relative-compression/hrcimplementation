import sys
import JSONLayer
from hypothesis import given, strategies as st, settings, Verbosity, example
from test_fasta_generator import fasta_gen, just_dna_gen

#To return a triple for matches in RLZ, and tuples for substitutions and insertions
def as_match(dct):
    if "__match__" in dct:
        return (dct["pos"], dct["len"], dct["ch"])
    elif "__sub__" in dct:
        return (dct["pos"], dct["ch"])
    elif "__ins__" in dct:
        return (dct["pos"], dct["ch"])
    return dct

@settings(deadline=None)
@given(ref=st.text(min_size=1, alphabet=st.characters(codec='ascii', exclude_characters=['>'])), target=st.text(min_size=1, alphabet=st.characters(codec='ascii', exclude_characters=['>'])))
def test_idocomp_parse_rlz(ref, target):
    try:
        
        matches=JSONLayer.test_function_with_hook("test_idocomp_parse_rlz", [ref, target], as_match)
    except BaseException as e:
        print(e)
        assert False
    try:
        """We have to test two things:
        1) That the matches actually reconstruct the target exactly
        2) That the matches are the best possible, i.e. the longest match at each time.
        """
        if ref == "0" and target == "000":
            print("Hi!")
        #Test reconstruction: matches is a list of triples (pos, len, ch)
        reconstruction=""
        for pos, length, ch in matches:
            assert pos >= 0 and pos < len(ref)
            assert length >= 0 and pos + length <= len(ref)
            if (length > 0):
                reconstruction += ref[pos:pos+length]
            reconstruction += chr(ch)
        assert target == reconstruction
        #Longest match
        i=0
        for pos, length, ch in matches:
            if length > 0:
                assert ref[pos:pos+length] == target[i:i+length]
            if ((i + length) < (len(target) - 1)) and (pos + length < len(ref)):
                assert target[i+length] != ref[pos + length]
            i+=length + 1
        #best match
        i=0
        for pos, length, ch in matches:
            #Check that this match is best possible by brute-forcing the rest
            for ref_pos in range(len(ref)):
                j=0
                while (ref_pos + j < len(ref) and i+j < len(target) - 1 and ref[ref_pos+j] == target[i+j]):
                    j+=1
                assert j <= length, "Longest match found at pos " + str(ref_pos) + " for suffix " + str(i) + " of target"
            i+=length + 1
    except BaseException as e:
        print(e)
        assert False


@st.composite
def ref_with_substitutions(draw, txt=st.text(min_size=20, alphabet=st.characters(codec="ascii", exclude_characters=['>']))):
    ref = draw(txt)
    subs = draw(st.lists(
                    st.tuples(
                        st.integers(min_value=0, max_value=len(ref)-1), st.characters(codec="ascii", exclude_characters=['>'])), 
                    max_size=len(ref), unique_by=lambda t: t[0])) #The substitutions are unique by their position
    return (ref, subs)

@st.composite
def ref_with_few_substitutions(draw, txt=st.text(min_size=20, alphabet=st.characters(codec="ascii", exclude_characters=['>']))):
    ref = draw(txt)
    subs = draw(st.lists(
                    st.tuples(
                        st.integers(min_value=0, max_value=(len(ref)-1 // 2)), st.characters(codec="ascii", exclude_characters=['>'])), 
                    max_size=len(ref), unique_by=lambda t: t[0])) #The substitutions are unique by their position
    return (ref, subs)

@st.composite
def ref_target_with_insertions_subs(draw, txt=st.text(min_size=20, alphabet=st.characters(codec="ascii", min_codepoint=33, max_codepoint=126, exclude_characters=['>']))):
    ref = draw(txt)
    insertions = draw(st.lists(
        st.tuples(st.integers(min_value=0, max_value=(len(ref)-1)), st.characters(codec="ascii", min_codepoint=33, max_codepoint=126, exclude_characters=['>'])),
        max_size=len(ref), unique_by=lambda t: t[0]
    ))
    ref_ins_l = list(ref)
    for pos, ch in insertions:
        ref_ins_l.insert(pos, ch)
    #Do substitutions too
    subs = draw(st.lists(
                    st.tuples(
                        st.integers(min_value=0, max_value=(len(ref_ins_l)-1)), st.characters(codec="ascii", min_codepoint=33, max_codepoint=126, exclude_characters=['>'])), 
                    max_size=len(ref_ins_l) // 2, unique_by=lambda t: t[0])) #The substitutions are unique by their position
    for pos, ch in subs:
        ref_ins_l[pos] = ch
    target = "".join(ref_ins_l)
    return (ref, target)

@settings(deadline=None)
@given(tup=ref_with_substitutions())
def test_idocompmax_substitutions(tup):
    #We are going to randomly apply substitutions to the reference
    ref=tup[0]
    target=list(ref) #list of characters that we can modify
    for pos, ch in tup[1]:
        target[pos] = ch
    target_str = "".join(target)
    subtest_max(ref, target_str)

@settings(deadline=None)
@given(ref=just_dna_gen(), target=just_dna_gen())
def test_idocomp_max_substitutions_randos(ref, target):
    subtest_max(ref, target)

def subtest_max(ref, target_str):
    matches = None
    subs = None
    try:
        obj = JSONLayer.test_function_with_hook("test_idocompmax_parse_subs", [ref, target_str], as_match)
        if obj is None:
            print("Subtest_max: error, JSON returned None")
            assert False
        matches = obj["matches"]
        subs = obj["subs"]
    except BaseException as e:
        print(e)
        assert False
    #Reconstruct
    try:
        reconstruction=[]
        for pos, length, ch in matches:
            assert pos >= 0 and pos < len(ref)
            assert length >= 0 and pos + length <= len(ref)
            if (length > 0):
                reconstruction.extend(ref[pos:pos+length])
            reconstruction.append(chr(ch))
        for pos, ch in subs:
            assert pos < len(target_str)
            reconstruction[pos] = chr(ch)
        reconstruction_str = "".join(reconstruction)
        assert target_str == reconstruction_str
    except AssertionError as assertion_error:
        print(assertion_error)
        raise assertion_error
    except BaseException as e:
        print(e)
        assert False

@settings(deadline=10000)
@given(string=st.text(min_size=50, alphabet=st.characters(codec='ascii', min_codepoint=0)))
@example(string="""YKI@vxIdT["apZhfE>$'
69(W<T`N&@MJV(\d'([c0;qSf
xx'cz_""")
def test_arith(string):
    ret = 0
    try:
        ret = JSONLayer.test_function("test_arith", [string])
    except BaseException as e:
        print(e, file=sys.stderr)
        assert False
    assert ret is not None
    assert -1 == int(ret)


@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(deadline=None)
def test_idocompmax_ins(fasta_ref, fasta_target):
    byte_differ = JSONLayer.test_function("test_idocompmax_insertions_decomp", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"

@given(fasta_ref=fasta_gen(), fasta_target=fasta_gen())
@settings(deadline=None)
def test_idocompmax_ins(fasta_ref, fasta_target):
    byte_differ = JSONLayer.test_function("test_idocompmax_insertions_decomp", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"

@given(tup=ref_target_with_insertions_subs())
@settings(deadline=None)
def test_idocompmax_ins_2(tup):
    fasta_ref, fasta_target = tup
    byte_differ = JSONLayer.test_function("test_idocompmax_insertions_decomp", [fasta_ref, fasta_target])
    assert byte_differ is not None, "Error on the test"
    int_byte_differ = int(byte_differ)
    assert -1 == int_byte_differ, "Fasta files differ!!!"