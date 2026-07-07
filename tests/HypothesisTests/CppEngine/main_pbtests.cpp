/**
 * First level functions to receive and send data to the parent
 * process. Once the data is received, it is passed to json_parse
 * to perform a function call.
*/


#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "json_parse.hpp"
#include <vector>
#include <sys/stat.h>

const char *END = "END";

int get_n_lines(const std::string& message) {
    int i = 1; //There's always one line
    for (char c : message) {
        if (c == '\n') {
            i++;
        }
    }
    return i;
}

std::unique_ptr<std::string> process_line(const char *message) {
    return json_parse(message);
}

void check_file_descriptor(int fd) {
    struct stat buf;
    if (fstat(fd, &buf))
        std::cout << "Bad call" << std::endl;
    else
        std::cout << "Stat: " << buf.st_mode << std::endl;
}

int write_string_to_fd(int fd_ctrl, const std::string& string) {
    //Get number of lines of the output and write it as a 4-byte int (big-endian)
    unsigned char bytes[4];
    int n = string.size();
    bytes[0] = (n >> 24) & 0xFF;
    bytes[1] = (n >> 16) & 0xFF;
    bytes[2] = (n >> 8) & 0xFF;
    bytes[3] = n & 0xFF;
    try {
        std::cout << "About to write reply " << string << " on file descriptor " << fd_ctrl << std::endl;
        write(fd_ctrl, bytes, 4);
        write(fd_ctrl, string.c_str(), n);
        std::cout << "Finished printing!!" << std::endl;
        return 0;
    } catch (std::exception& e) {
        std::cout << "There was an IO Error!" << std::endl;
        std::cerr << "IOError: " << std::endl;
        std::cerr << e.what() << std::endl;
        std::cout << "1{\"return\": \"ERROR\"}" << std::endl;
        return 1;
    }
}

/**
 * The logic of main is a loop that proceeds as follows:
 * It receives as its only argument the file descriptor of a pipe,
 * to which it must send all control output, including the result of the tests
 * (but not necessarily any printed statements during the test).
 * To read a line, it first expects the number of bytes to read
 * as an integer printed in text. Then, immediately after, uses
 * cin.read() to read the exact number of bytes that it is expecting.
 * main uses formatted input to read the first number of bytes. This is
 * useful because it also descards any potential newline characters left in
 * the input from the previous read.
 * After reading the provided number of bytes, it passes the C-char read to process_line.
 * Process_line will return a return string in a unique pointer. To return the string through
 * the control pipe, we first inspect how many lines the string has. This is done because
 * python input basically only works line by line, and we make the job of the receiver easy.
 * Then, we write the number of lines as an integer printed in text, followed by the string itself.
 * The loop continues until we get the message "3END", in which case we print back "1END" and we 
 * terminate.
*/
int main(int argc, char *argv[]) {
    (void)argc;
    int fd_ctrl;
    try {
        fd_ctrl = std::stoi(argv[1]);
        check_file_descriptor(fd_ctrl);
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    //Communicate until we get an end of line
    int message_size;
    std::cin >> message_size;
    //We allocate just enough memory for the message
    char *buf = static_cast<char*>(malloc(message_size+1));
    //We read that amount of characters, which does not
    //include the terminating end of line
    std::cin.read(buf, message_size);
    buf[message_size] = '\0';
    //Check if it's the end of the program
    while (strcmp(END, buf) != 0) {
        //We process the line and get some output
        try {
            std::unique_ptr<std::string> output = process_line(buf);
            std::string out_m = *output;
            //Bounce back!
            //std::string out_m = buf;
            int ret = write_string_to_fd(fd_ctrl, out_m);
            if (ret != 0) { //fatal error
                free(buf);
                return 1;
            }
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            int ret = write_string_to_fd(fd_ctrl, "{\"return\": \"ERROR\"}");
            if (ret != 0) { //fatal error
                free(buf);
                return 1;
            }
        }
        //Read next input size
        std::cin >> message_size;
        if (!std::cin) {
            std::cerr << "Bad message size" << std::endl;
            free(buf);
            return 1;
        }
        //Reallocate and read
        buf = static_cast<char*>(realloc(buf, message_size+1));
        if (buf == nullptr) {
            std::cerr << "Out of memory to allocate new message!" << std::endl;
            free(buf);
            write_string_to_fd(fd_ctrl, "END");
            return 0;
        }
        std::cin.read(buf, message_size);
        buf[message_size] = '\0';
    }
    free(buf);
    //Send message back
    write_string_to_fd(fd_ctrl, "END");
    return 0;
}

