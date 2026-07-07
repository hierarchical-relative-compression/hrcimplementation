//Exec simulation
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

int main(int argc, char *argv[]) {
    execl("/usr/bin/java", "java", "SCCGConnector", (char *)0 );
    exit(1);
}
