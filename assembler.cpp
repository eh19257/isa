#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>

using  namespace std;

/* Function Headers */
void assemble(string filePath, string outputPath);
void parse(string line, string outputPath);


// Parse each line and append to the output file
string parse(string line){
    return "ADD r0 r0 r0";
}

// Main Assembly function
void assemble(string filePath, string outputPath){
    ifstream program(filePath);
    ofstream output(outputPath);

    string line;
    int counter = 0;

    while (!program.eof()){
        getline(program, line);
        output << parse(line) << endl;;
    }
    output << "HALT" << endl;
    
    program.close();
    output.close();
}

int main(int argc, char** argv){
    if (argc != 2){
        std::cout << "Usage: ./assemble fileName.j" << std::endl; 
    } else {
        string outputFilePath = string(argv[1]).substr(0, string(argv[1]).size() - 2);
        assemble(argv[1], outputFilePath);
    }
    return 1;
}

