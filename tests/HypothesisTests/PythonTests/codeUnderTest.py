'''
Created on Aug 10, 2023

@author: max
'''

def encode(input_string):
    count = 1
    prev = ""
    lst = []
    if input_string == "":
        return []
    for character in input_string:
        if character != prev:
            if prev:
                entry = (prev, count)
                lst.append(entry)
            count = 1
            prev = character
        else:
            count += 1
    entry = (character, count)
    lst.append(entry)
    return lst


def decode(lst):
    q = ""
    for character, count in lst:
        q += character * count
    return q