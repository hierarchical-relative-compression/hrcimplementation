'''
Created on Dec 18, 2023
Version 1.0, 18/07/2024

@author: max

Commands to issue tests through Connector
'''

import Connector
import json

def single_test(test_name : str, input_data):
    try:
        ret = Connector.send_command_return_output("TEST " + test_name + " " + input_data)
        return ret
    except Exception:
        return "ERROR"

"""
Serializes in JSON format a call to a function,
sends the JSON string through the Connector, receives a JSON
object back, deserializes it, and returns the object.
Parameters:
    f_name: name of the function.
    args: list of arguments to the function
"""
def test_function(f_name, args):
    argc = len(args)
    json_obj = {"fn_name" : f_name, "args": {}}
    json_obj["args"]["argc"] = argc
    for i in range(argc):
        json_obj["args"][i] = args[i]
        #For debug:
        print("arg " + str(i) + ": " + str(args[i]))
    serialized = json.dumps(json_obj)
    #Print byte by byte
    #for c in serialized:
    #    print(c)
    output = Connector.send_string_return_output(serialized)
    if (output is None):
        return None
    deserialized = json.loads(output)
    ret = deserialized["return"]
    return ret

def test_function_with_hook(f_name, args, hook):
    argc = len(args)
    json_obj = {"fn_name" : f_name, "args": {}}
    json_obj["args"]["argc"] = argc
    for i in range(argc):
        json_obj["args"][i] = args[i]
        #For debug:
        print("arg " + str(i) + ": " + args[i])
    serialized = json.dumps(json_obj)
    #Print byte by byte
    #for c in serialized:
    #    print(c)
    output = Connector.send_string_return_output(serialized)
    if (output is None):
        return None
    deserialized = json.loads(output, object_hook=hook)
    ret = deserialized["return"]
    return ret

def start():
    Connector.start_child()

def end():
    Connector.finish()


"""
LEGACY COMMENTS AND INFO
"""
#If we write the character backslash followed by the character n, which is the way
#that python's json serializes a newline, when writing the backslash into the
#text stream to the child, we will duplicate the backslash, which is not the
#intended behaviour.
#serialized_n = serialized.replace("\\n", "\n")