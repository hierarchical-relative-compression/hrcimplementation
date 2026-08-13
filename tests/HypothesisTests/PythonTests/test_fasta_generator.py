"""
This file contains a hypothesis generator of FASTA files, that have the two following lines:
1) First, a line that starts with '>' and then a random string without newlines. This is the first FASTA line.
2) A random sequence of random length from the alphabet {A, C, G, T}.
"""

import JSONLayer
from hypothesis import given, strategies as st, settings, Verbosity, example

#Converts A, B, C, D into A, C, G, T
def to_dna(c):
    if c == 'A':
        return 'A'
    elif c == 'B':
        return 'C'
    elif c == 'C':
        return 'G'
    elif c == 'D':
        return 'T'
    else:
        return 'N'

def to_dna_extras(c):
    if c == 'A':
        return 'A'
    elif c == 'B':
        return 'C'
    elif c == 'C':
        return 'G'
    elif c == 'D':
        return 'T'
    elif ord(c) < 65 or ord(c) > 90:
        return 'N'
    else:
        return c

"""
Generates a FASTA file with a random initial FASTA line and a random DNA sequence.
"""
@st.composite
@example("AAAAAAACAAAAACCAAAACACAAAACCCAAACAACAAACACCAAACCACAAACCCCAACAACCAACACACAACACCCAACCACCAACCCACAACCCCCACACACCACACCCCACCACCCACCCCCCC")
def fasta_gen(draw, size_min=20):
    fasta_line=draw(st.text(min_size=1, alphabet=st.characters(codec='ascii', min_codepoint=48, max_codepoint=126, exclude_characters=['\\', '/', '`', '>']))) #Characters except space
    dna_line=draw(st.text(min_size=size_min, alphabet=st.characters(min_codepoint=65, max_codepoint=68).map(to_dna)))
    string = '>' + fasta_line + '\n' + dna_line
    return string

"""
Generates a FASTA file with a random initial FASTA line, and a random DNA sequence that
can contain other letters than "A", "C", "T" and "G".
"""
@st.composite
def fasta_gen_extras(draw, size_min=20):
    fasta_line=draw(st.text(min_size=1, alphabet=st.characters(codec='ascii', min_codepoint=48, max_codepoint=126, exclude_characters=['\\', '/', '`', '>']))) #Characters except space
    #This DNA line contains ACGT, other capital letters, and some random characters that will be converted to 'N's
    dna_line = draw(st.text(min_size=size_min, alphabet=st.characters(min_codepoint=65, max_codepoint=100).map(to_dna_extras)))
    string = '>' + fasta_line + '\n' + dna_line
    return string

"""
Generates a text file with a random DNA sequence. No FASTA line is included.
"""
@st.composite
def just_dna_gen(draw, size_min=20):
    dna_line=draw(st.text(min_size=size_min, alphabet=st.characters(min_codepoint=65, max_codepoint=68).map(to_dna)))
    return dna_line

#print("Hi")
#print(fasta_gen().example())
#print(fasta_gen().example())

