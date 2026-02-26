'''
Created on Aug 11, 2023
Version 1.0, 18/07/2024

@author: max

'''

import pathlib
import subprocess
import os
import signal
import sys
import re
import threading
import errno
import time

"""
Static initialization of fields
We initialize here the path to the child (hardcoded)
"""
curr_dir = pathlib.Path(__file__).parent
curr_dir_path = pathlib.PurePath(curr_dir)
parent = curr_dir_path.parent.parent
cpp_dir = parent.parent / "build" / "tests" / "HypothesisTests"
print("CPP DIR: ", str(cpp_dir))
cpp_exec_path = str(cpp_dir / "main_pbtests")
del(curr_dir_path)
#del(parent)

lock = threading.Lock()
global reading_out_alive
global reading_err_alive
reading_out_alive=False
reading_err_alive=False

"""
Test example, where we start the children,
send some test strings, and then we soft kill the child process
"""
def start():
    '''
    Constructor
    '''
    print(curr_dir)
    print(cpp_dir)
    print(cpp_exec_path)
    start_child()
    send_test_strings()
    soft_kill(child_process)

"""
Start the child CPP process
"""
def start_child():
    #We need an extra pipe for the control messages that we receive
    #control_r and control_w are file descriptors to read and write to a pipe.
    #The parent process will only use control_r, the child will only use control_w
    global control_r, control_w
    control_r, control_w = os.pipe()
    #os.pipe() makes non-inheritable streams by default. Let's change that
    os.set_inheritable(control_r, True)
    os.set_inheritable(control_w, True)

    #Same with the stdin, stdout pipe
    global stdin_r, stdin_w
    stdin_r, stdin_w = os.pipe()
    os.set_inheritable(stdin_r, True)
    os.set_inheritable(stdin_w, True)

    global stdout_r, stdout_w
    stdout_r, stdout_w = os.pipe()
    os.set_inheritable(stdout_r, True)
    os.set_inheritable(stdout_w, True)

    global stderr_r, stderr_w
    stderr_r, stderr_w = os.pipe()
    os.set_inheritable(stderr_r, True)
    os.set_inheritable(stderr_w, True)

    pid = os.fork()
    if (pid == 0):
        #Child process
        try :
            #make the reading end of the stdin pipe our standard input
            os.dup2(stdin_r, sys.stdin.fileno())
            os.close(stdin_w)
            #make the writing end of the stdout pipe our standard output
            os.dup2(stdout_w, sys.stdout.fileno())
            os.close(stdout_r)
            #same with stderr
            os.dup2(stderr_w, sys.stderr.fileno())
            os.close(stderr_r)
            #Pass the writing end of the control pipe as a parameter to the child
            os.close(control_r)
            os.execvp(cpp_exec_path, ["main_pbtests", str(control_w)])
        except Exception as e:
            print("IOError: ", str(e))
    else:
        #Parent process
        #Close the pipes that we are not going to use
        os.close(stdin_r)
        os.close(stdout_w)
        os.close(control_w)
        os.close(stderr_w)
        #We have the file descriptors as globals to use
        global child_process_pid
        child_process_pid = pid
        #Run a thread that reads from the stdout and blocks
        global reading_out_alive
        if not reading_out_alive:
            thread = threading.Thread(target = read_stdout)
            thread.start()
            reading_out_alive = True
        global reading_err_alive
        if not reading_err_alive:
            thread2 = threading.Thread(target = read_stderr)
            thread2.start()
            reading_err_alive = True
    

def old_finish():
    send_string_print_output("END")
    if (child_process.poll() is None):
        #The subprocess is still alive
        soft_kill(child_process)

def read_stdout():
    try:
        while True:
            buf = os.read(stdout_r, 1000)
            if buf:
                print(buf.decode("ascii"))
                pass
            else:
                break
    except Exception as e:
        pass
    global reading_out_alive
    reading_out_alive=False
    

def read_stderr():
    try:
        while True:
            buf = os.read(stderr_r, 1000)
            if buf:
                print(buf.decode("ascii"))
                pass
            else:
                break
    except Exception as e:
        pass
    global reading_err_alive
    reading_err_alive=False

def finish(send_string=True):
    if send_string:
        send_string_print_output("END")
    global child_process_pid
    os.kill(child_process_pid, 0)
    #Read all output
    print("Output from finished process:")
    while True:
        buf = os.read(stdout_r, 1000)
        if buf:
            print(buf.decode("ascii"))
        else:
            break
    print("Errors from finished process: ")
    while True:
        buf = os.read(stderr_r, 1000)
        if buf:
            print(buf.decode("ascii"), file=sys.stderr)
        else:
            break

    close_stream(control_w)
    close_stream(stdin_w)
    close_stream(stdout_r)
    close_stream(stderr_r)

def close_stream(fd):
    try:
        os.close(fd)
    except OSError:
        pass

"""
If there was some error with the child CPP process, 
stop it and re-run it again
"""
def recover():
    finish(send_string=False)
    global child_process_pid
    del child_process_pid
    global control_r, control_w
    global stdin_r, stdin_w
    global stdout_r, stdout_w
    global stderr_r, stderr_w
    del control_r, control_w, stdin_r, stdin_w, stdout_r, stdout_w, stderr_r, stderr_w
    start_child()

"""
Sends some test strings to the child
""" 
def send_test_strings():
    send_string_print_output("Hola amigo")
        
    send_string_print_output("Segundo hola")
    
    send_string_print_output("A test for escaping \\n sequences")
    
    send_string_print_output("A test for sending \nmultiline strings")
    
    send_string_print_output("Tercer hola")
        
    send_string_print_output("END")

"""
Function that takes a string and sends it to the child.
It sends first the number of characters that will be written,
and then sends the full message. Prints the output to stdout.
"""
def send_string_print_output(input_string):
    output = send_string_return_output(input_string)
    print(output)

"""
Function that takes a string and sends it to the child.
It sends first the number of characters that will be written,
and then sends the full message. Expects an output that has
a 4-byte integer of the number of characters to read, plus the
characters to read.
...
"""
def send_string_return_output(input_string):
    with lock:
        raw_str = input_string.encode("ascii")
        size = len(raw_str)
        byt = None
        try:
            #For debug:
            #time.sleep(0.25)
            #print("Sending message through thread ", str(threading.current_thread().ident))
            #Write size
            i = os.write(stdin_w, str(size).encode("ascii"))
            #Write the message
            i = os.write(stdin_w, raw_str)
            byt = os.read(control_r, 4)
        except IOError as e:
            if e.errno == errno.EPIPE:
                print("Pipe IO Error on the child, returning None", file=sys.stderr)
            else:
                print("Generic IO Error on the child, returning None", file=sys.stderr)
            recover()
            return None
        except BaseException as e:
            print(e, file=sys.stderr)
            return None
        if (len(byt) == 0):
            print("Empty bytes object!!! assuming error", file=sys.stderr)
            recover()
            return None
        #From the 4 bytes in big endian we have to find the number of characters to read
        n_chars = (byt[0] << 24) | (byt[1] << 16) | (byt[2] << 8) | byt[3]
        try:
            output_b = os.read(control_r, n_chars)
            output = output_b.decode("ascii")
            return output
        except IOError as e:
            if e.errno == errno.EPIPE:
                print("Pipe IO Error on the child, returning None", file=sys.stderr)
            else:
                print("IO Error when reading message, returning None", file=sys.stderr)
            recover()
            return None

"""
Sends the kill signal to the child process, and collects any output
on both standard output and error that it may have sent
"""
def soft_kill(process):
    process.kill()
    output, err = process.communicate()
    print(f"Output from killed child process: {output}")
    print(f"Errors from killed child process: {err}", file=sys.stderr)

"""
Hard kills the process through the OS, and collects any output on both
standard output and error that it may have sent.
"""
def hard_kill(process):
    os.killpg(process.pid, signal.SIGINT)
    output, err = process.communicate()
    print(f"Output from hard killed child process: {output}")
    print(f"Errors from hard killed child process: {err}", file=sys.stderr)




"""
LEGACY CODE THAT MAY BE USEFUL LATER
"""


#Uses the "communicate" function of subprocess. Problem: The child process
#will be killed afterwards.
def communicate_finish(child_process):
    try:
        output = child_process.communicate("Gracias amigo", timeout=5)[0]
        print(f"Output from process: {output}")
        if child_process.returncode == 0:
            print("Child finished, return code 0 (success)")
        else:
            print("Child finished, return code 1 (failure)")
    except subprocess.TimeoutExpired:
        print("Timeout for communication expired, killing child process")
        hard_kill(child_process)

"""
Function that takes a string and sends it to the child.
It sends first the number of characters that will be written,
and then sends the full message. Expects an output like:
n_linesline1
line2
...
"""
def old_send_string_return_output(input_string):
    with lock:
        raw_str = input_string.encode("ascii")
        size = len(raw_str)
        try:
            #time.sleep(1)
            print("Sending message through thread ", str(threading.current_thread().ident))
            #Write size
            i = child_process.stdin.write(str(size).encode("ascii"))
            #Write the message
            i = child_process.stdin.write(raw_str) #Before it was input_string
            #Terminate
            #i = child_process.stdin.write("\n")
            #We no longer use line buffering
            #The returned message tells us how many lines are there
            #o = child_process.stdout.readline()
            o = read_end.readline()
        except IOError as e:
            if e.errno == errno.EPIPE:
                print("Pipe IO Error on the child, returning None", file=sys.stderr)
            else:
                print("Generic IO Error on the child, returning None", file=sys.stderr)
            recover()
            return None
        if o == '':
            print("Output empty!!! assuming error", file=sys.stderr)
            recover()
            return None
        output = o.decode("ascii")
        if (output is None):
            print("ERROR: OUTPUT IS EMTPY!", file=sys.stderr)
            recover()
            return None
        if (re.match(r'^(\d+)', output) == None):
            print("OUTPUT DID NOT SPECIFY NUMBER OF LINES, ASSUMING WRONG", file=sys.stderr)
            print("output: ", output, file=sys.stderr)
            recover()
            return None
        n_lines = int(re.search(r'^(\d+)', output).group())
        digits = len(str(n_lines))
        n_lines = n_lines - 1; #One line has already been read
        while (n_lines > 0) :
            #output += child_process.stdout.readline()
            output += read_end.readline()
            n_lines -= 1
        output = output[digits:] #Trim the number of lines
        return output

"""
Old start child
"""
def start_child_old():
    global child_process
    child_process = subprocess.Popen(
        [cpp_exec_path, str(control_r), str(control_w)],
        shell=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,  #Creates pipe between parent and child
        text=False,              #Data in the pipe will be in binary format
        bufsize=0)              #Flush after every read/write!!!
