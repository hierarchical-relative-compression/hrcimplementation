from hypothesis import given, strategies as st, settings, Verbosity, HealthCheck
from test_fasta_generator import fasta_gen, fasta_gen_extras
import os.path
import itertools
import tempfile

cont = itertools.count()

def get_prefix():
    return str(next(cont))    
    
@st.composite
def hrlz_files(draw):
    n_chr = draw(st.integers(min_value=1, max_value=40))
    chr_names = [f"chr_{i}" for i in range(n_chr)]
    tempdir = tempfile.mkdtemp(prefix=get_prefix()) #Thread-safe, unique
    filenames = draw(st.lists(st.text(min_size=4, alphabet=st.characters(codec="ascii", min_codepoint=65)),
                               min_size=2, max_size=40, unique=True))
    n_files = len(filenames)
    with open(os.path.join(tempdir, "abs_files.txt"), "w") as abs_file:
        abs_file.write(str(n_files) + '\n')
        for filename in filenames:
            abs_file.write(str(os.path.join(tempdir, filename)) + '\n')
    #abs_file is done, now write the files themselves
    for filename in filenames:
        os.mkdir(os.path.join(tempdir, filename))
        for chr_name in chr_names:
            with open(os.path.join(tempdir, filename, chr_name), "w") as chr_file:
                #Write a random fasta
                fasta=draw(fasta_gen())
                chr_file.write(fasta)
    return tempdir

@settings(max_examples=10, suppress_health_check=[HealthCheck.too_slow])
@given(hrlzdir=hrlz_files())
def test_hrlz(hrlzdir):
    print(str(hrlzdir))