//g++ "pFC.cpp" -o pFC.elf --std=c++11 -g -pthread -O3 #-static-libstdc++
#include <iostream> //Terminal
#include <fstream> //IO
#include <string> //Strings
#include <sstream> //String streams
typedef unsigned char byte;
typedef unsigned int uint;

std::string fileStr; //file name string
std::string sizeStr; //file size string
std::string compSizeStr; //compressed file size string

char* prog; //program array
byte* compR; //compressed register array

char* progEnd; //program endpoint
uint progSize = 0; //program size
char* pb; //program byte
byte* cb; //compressed [register] byte

int main(int argc, char *argv[])
{
  //Load file
    if (argc == 2)
    {
      //Load file from argument
        fileStr = argv[1];
        std::ifstream inFile(fileStr);
        if (!inFile) { std::cout << "\033[1;31mFile was not found!\033[0m" << std::endl; exit(0); }
      //Calculate file size
        std::streampos begin = inFile.tellg();
        inFile.seekg(0, std::ios::end);
        std::streampos end = inFile.tellg();
        progSize = end - begin;
      //Copy stream into char array
        prog = (char*)malloc(progSize);
        compR = (byte*)malloc(progSize);
        inFile.seekg(0);
        char prevComm = ' ';
        uint pc = 0; //Program index count
        uint ps = 0; //Uncompressed program size
        for (uint fc = 0; fc < progSize; fc++)
        {
            inFile.get(prog[pc]);
            switch(prog[pc]) //Filter
            {
                case '+': case '-': case '>': case '<': case ',': case '.': case '[': case ']': //When it's a command (could be done quicker with a bool array)
                    ps++; //Increment uncompressed program size
                    compR[pc] = 1; //We only want to run this once, thank you
                  //Do program compression
                    if (prevComm == prog[pc] && prog[pc] != '[' && prog[pc] != ']') //Is this command the same as the previous one?
                    {
                        if (compR[pc] != 255) //Is the register for this command not full?
                        {
                            pc--; //Overwrite previous command
                            compR[pc]++; //Increment the amount of times we must run this command
                        } else {
                            compR[pc] = 1;
                        }
                    }
                    prevComm = prog[pc];
                  //Increment program count
                    pc++;
                    break;
                default: break; //When it's not, allow the next byte to overwrite
            }
        }
      //Close file
        inFile.close();
      //Init stat strings
        progSize = pc;
        compSizeStr = std::to_string(progSize);
        sizeStr = std::to_string(ps);
      //Init bfgram ints
        progEnd = prog + progSize; //Set program end pointer
        pb = prog; //Set program pointer
        cb = compR;
      //Begin compilation
        std::string source = "#include <stdio.h>\n#include <stdlib.h>\nint main()\n{\n    char* tb = (char*)malloc(30000);\n    putchar('\\n');\n";
        std::string compRStr = "";
        byte depth = 1;
        do
        {
            source += std::string(depth * 4, ' '); //Indent
            compRStr = std::to_string(*cb);
            switch (*pb)
            {
                case '+': source += "*tb += " + compRStr + ";\n"; break;
                case '-': source += "*tb -= " + compRStr + ";\n"; break;
                case '>': source += "tb += " + compRStr + ";\n"; break;
                case '<': source += "tb -= " + compRStr + ";\n"; break;
                case '[': source += "while (*tb) {\n"; depth++; break;
                case ']': source += "}\n"; depth--; break;
                case ',': source += "*tb = getchar();\n"; break;
                case '.': source += "putchar(*tb);\n"; break;
            }
            pb++; //Increment program counter
            cb++; //Increment compression register counter
        } while (pb != progEnd);
        source += "}";
        std::cout << "\033[1;32mSource conversion complete.\033[0m" << std::endl;
        std::ofstream outFile(fileStr + ".c"); //Open the stream to write
        outFile << source;
        outFile.close(); //Close the stream
    } else {
        std::cout << "\033[1;31mNo program arguments given,\nsuch as ./pFC.elf test.bf\033[0m" << std::endl;
    }
}
