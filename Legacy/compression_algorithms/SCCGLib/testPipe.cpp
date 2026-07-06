//Popen simulation
//Credit and source: https://stackoverflow.com/questions/6171552/popen-simultaneous-read-and-write

//Adapted to C++ by Máximo Pérez López

//POSIX API
//Part of the OS libraries and not C, so we need to put .h
#include <unistd.h>
#include <sys/wait.h>
//To send and handle signals to processes
#include <sys/prctl.h>
#include <signal.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <array>
#include <memory>
#include <stdexcept>
#include <filesystem>

//There should be a definition, from CMake, of the macro THIS_DIR_STR, that contains
//the path to the directory containing this file (and the java files) as a string.

namespace MrMCMax {

    void end_java();
    void open_java();

    /*
    * The first file descriptor is for a process that reads from the pipe, and the second is
    * for a process that writes to the pipe. The pipes are unidirectional, so we need two for
    * a communication both ways. See man pipe
    */
    //PARENT => CHILD
    int outpipefd[2];
    //CHILD => PARENT
    int inpipefd[2];

    int status;

    FILE *input_stream;

    pid_t pid = 0;

    void open_java() {
        //Get directory where the java class is
        //std::string project_sources = std::filesystem::canonical("/proc/self/exe").parent_path().string();
        std::string project_sources = THIS_DIR_STR;
        const char *cp = project_sources.c_str();
        std::cout << "Java class path: " << cp << std::endl;
        //Clear the variables just in case
        outpipefd[0] = 0;
        outpipefd[1] = 0;
        inpipefd[0] = 0;
        inpipefd[1] = 0;
        //Create the pipes
        pipe(outpipefd);
        pipe(inpipefd);

        //Fork to create a child
        pid = fork();
        if (pid == 0) {
            //This code will be executed by the child

            //This call to dup2 makes the reading end of the parent's outpipe be the standard input of this process
            dup2(outpipefd[0], STDIN_FILENO);
            //This call to dup2 makes the writing end of the parent's inpipe be the standard output of this process,
            //same with the standard error
            dup2(inpipefd[1], STDOUT_FILENO);
            dup2(inpipefd[1], STDOUT_FILENO);

            //Ask the kernel to deliver SIGTERM in case the parent dies (StackOverflow says it's good practice)
            prctl(PR_SET_PDEATHSIG, SIGTERM);

            //Close unused pipe ends for this process
            //Child only reads from outpipe, we close writing to outpipe
            close(outpipefd[1]);
            //Child only writes to inpipe, we close reading from inpipe
            close(inpipefd[0]);

            //Execute our java process
            //execlp will search in the PATH variable for the java program
            execlp("java", "java", "-cp", cp, "SCCGConnector", (char *)0 );

            //It should not return here. If it does, something went wrong in the exec
            std::cerr << "Exec went wrong" << std::endl;
            exit(1);
        } else {
            //This code will be executed by the parent

            //Close unused pipes for this process: parent only writes to outpipe
            close(outpipefd[0]);
            //parent only reads from inpipe
            close(inpipefd[1]);

            //Max: create FILE* so that we can stream and use fgets to read line by line
            input_stream = fdopen(inpipefd[0], "r");
            if (input_stream == NULL) {
                std::cout << "Something went wrong with the file stream" << std::endl;
                exit(1);
            } else {
                std::cout << "Java bridge made" << std::endl;
            }
        }
    }

    void check_alive() {
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            // Child still alive
        } else if (result == -1) {
            std::cerr << "Child java process had an error" << std::endl;
            std::exit(1);
        } else {
            std::cerr << "Child java process exited before expected" << std::endl;
            std::exit(1);
        }
    }

    /**
    * THE LINE SHOULD ALWAYS END IN A NEWLINE CHARACTER
    */
    void write_line_print_output(char *line) {
        //Return message, to signal that the child has stopped outputting and is accepting
        //input again
	    std::cout << "Calling java: " << line << std::endl;
        char *ret_msg = "RETURN";
        int len_return = strlen(ret_msg);
        char buf[512];

        //Now we can write to the process.
        write(outpipefd[1], line, strlen(line));

        check_alive();
        //This reads up to a newline
        fgets(buf, 512, input_stream);

        std::cout << buf;
        //Max: fix maximum of lines
        int max_lines = 100;
        int i = 0;
        while (strncmp(buf, ret_msg, len_return) != 0 && i < max_lines) {
	    //std::cout << "Another line in buffer" << std::endl;
            std::cout << buf;
            fgets(buf, 512, input_stream);
            max_lines++;
        }
        if (i == max_lines) {
            std::cerr << "Too much output on SCCG!!!" << std::endl;
            end_java();
            std::exit(1);
        }
        std::cout << std::flush; //Just in case
    }

    void end_java() {
        char msg2[] = "END";
        write(outpipefd[1], msg2, strlen(msg2));

        //We have found RETURN. return
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    }

} //close namespace

using namespace MrMCMax;

int main_test(int argc, char *argv[]) {
    char msg[] = "./SCCG/data/wuhan.fasta ./SCCG/data/random.fasta ./SCCG/data/results\n";
    open_java();
    write_line_print_output(msg);
    end_java();
    return 0;
}

/** LEGACY CODE THAT MAY BE USEFUL LATER */
/**
 * https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
 * Added the trimming of the newline so that it doesn't mess up with the call
 * see string.erase and string.find_first_of
 * Not needed anymore
 */
std::string& exec_which_java_one_line() {
    char cmd[] = "which java";
    std::array<char, 128> buffer;
    static std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    //Now, trim from the newline to the end
    std::size_t pos_newline = result.find_first_of("\r\n");
    if (pos_newline != std::string::npos) {
        result.erase(pos_newline);
    }
    return result;
}