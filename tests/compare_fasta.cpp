#include <iostream>
#include <string>
#include <fstream>

std::size_t get_length(char *path) {
    //We open the file and get its size in bytes
    std::ifstream file;
    file.exceptions(std::ifstream::badbit);
    file.open(path, std::ifstream::binary);
    file.seekg(0, file.end);
    std::size_t length = file.tellg();
    file.close();
    return length;
}

void skip_fasta_line(std::ifstream& file) {
    char c = file.peek();
    if (c == '>') {
        //We have to ignore the first line
        std::string fasta_line;
        std::getline(file, fasta_line);
    }
}

int main(int argc, char *argv[]) {
    std::ifstream file1; 
    std::ifstream file2;
    file1.exceptions(std::ifstream::badbit);
    file2.exceptions(std::ifstream::badbit);
    file1.open(argv[1]);
    file2.open(argv[2]);
    //We skip the first line if it is a FASTA definition line
    skip_fasta_line(file1);
    skip_fasta_line(file2);
    //Read char by char, and ignore whitespace and newlines
    std::size_t i = 0;
    char c1 = 0;
    char c2 = 0;
    while (file1 && file2) {
        if (c1 != c2) {
            std::cout << "File 1 and 2 differ on sequence character " << i-1 << std::endl;
            return -1;
        }
        i++;
        file1 >> c1; //skipping whitespace
        file2 >> c2;
    }
    if (!file1) {
        if (!file2) {
            //All good
            return 0;
        } else {
            std::cout << "file 1 ended at byte " << i << std::endl;
        }
    } else {
        //File 1 didn't end but file 2 did!
        std::cout << "file 2 ended at byte " << i << std::endl;
    }
    return -1;
}


