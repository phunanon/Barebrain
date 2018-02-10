//g++ "pFI.cpp" -o pFI.elf --std=c++11 -g -pthread -O3 #-static-libstdc++
#include <iostream> //Terminal
#include <fstream> //IO
#include <string> //Strings
#include <sstream> //String streams
#include <thread> //For threads, and sleeping
typedef unsigned char byte;
typedef unsigned int uint;
const uint TAPE = 30000;
const byte LOOP = 255;

inline void clearScreen() { std::cout << "\033[2J\033[1;1H"; }

inline void flout(std::string out, std::string colour) //Flush text out to the terminal
{
    std::cout << "\033[1;" << colour << "m" << out << "\033[0m";
    fflush(stdout);
}


std::string fileStr; //file name string
std::string sizeStr; //file size string
std::string compSizeStr; //compressed file size string
uint progSize = 0; //program size
char* prog; //program array
byte* compR; //compressed register array
char* progEnd; //program endpoint
byte* tape = (byte*)calloc(TAPE, sizeof(*tape)); //memory
char** loopOpens; //open loop positions
char** loopCloses; //close loop positions
std::string stou = ""; //stdout
byte* tb = tape; //tape byte
char* pb; //program byte
byte* cb; //compressed [register] byte
bool termi = false; //terminated flag
bool stdi = false; //asking for input?
uint i; //used in loops

void runner()
{
    do //Run while program not terminated
    {
      //Interpret program byte
        switch (*pb)
        {
            case '+':
                *tb += *cb;
                break;
            case '-':
                *tb -= *cb;
                break;
            case '>':
                tb += *cb;
                break;
            case '<':
                tb -= *cb;
                break;
            case ',':
                stdi = true;
                for (i = 0; i < *cb; i++) { *tb = getchar(); }
                stdi = false;
                break;
            case '.':
                for (i = 0; i < *cb; i++) { stou += *tb; }
                break;
            case '[':
                if(!*tb) //Skip to matched ]?
                {
                    pb = loopCloses[pb - prog];
                    cb = compR + (pb - prog); //Set this from the same offset
                }
                break;
            case ']':
                if(*tb) //Loop
                {
                    pb = loopOpens[pb - prog]; //Loop back
                    cb = compR + (pb - prog); //Set this from the same offset
                }
                break;
        }
        pb++; //Increment program byte position
        cb++; //Increment compressed byte position
    } while (pb != progEnd); //Check if we have finished
    termi = true;
}


std::chrono::milliseconds timeStart;
unsigned long timeElap;
std::string strElap;
int insertPosition;
void display()
{
    clearScreen();
    std::cout << "\033[0;37m" << stou;
    fflush(stdout);
    flout("\nBrainfu*ck Int., Patrick Bowen [phunanon] 2016\n" + fileStr + ": ", "32");
    flout(sizeStr, "33");
    flout("B", "32");
    if (compSizeStr != "") { flout(" (", "32"); flout(compSizeStr, "33"); flout("B)", "32"); }
    flout("\nElap: ", "32");
    timeElap = (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - timeStart).count();
    strElap = std::to_string(timeElap);
    insertPosition = strElap.length() - 3;
    while (insertPosition > 0) { strElap.insert(insertPosition, ","); insertPosition -= 3; }
    flout(strElap, "33");
    flout("ms  Op: ", "32");
    std::stringstream ss;
    ss << std::hex << (void*)(pb - prog);
    flout(ss.str(), "33");
    flout("  Tape: ", "32");
    ss.str(std::string());
    ss << std::hex << (void*)(tb - tape);
    flout(ss.str(), "33");
    if (stdi) { flout("\nstdin: ", "33"); }
    std::this_thread::sleep_for(std::chrono::milliseconds(32)); //Framerate
}

int main(int argc, char *argv[])
{
  //Load file
    if (argc == 2)
    {
      //Load file from argument
        fileStr = argv[1];
        std::ifstream inFile(fileStr);
        if (!inFile) { flout("File was not found!\n", "31"); exit(0); }
      //Calculate file size
        std::streampos begin = inFile.tellg();
        inFile.seekg(0, std::ios::end);
        std::streampos end = inFile.tellg();
        progSize = end - begin;
      //Copy stream into char array
        prog = (char*)malloc(progSize); //(This will probably be bigger than we need, especially when compressed)
        compR = (byte*)malloc(progSize);
        inFile.seekg(0);
        char prevComm = ' ';
        uint pc = 0; //Program index count
        uint ps = 0; //Real program size
        for (uint fc = 0; fc < progSize; fc++)
        {
            inFile.get(prog[pc]);
            switch(prog[pc]) //Filter
            {
                case '+': case '-': case '>': case '<': case ',': case '.': case '[': case ']': //When it's a command
                    ps++; //Increment real program size
                    compR[pc] = 1; //We only want to run this once, thank you
                  //Do program compression
                    if (prevComm == prog[pc] && prog[pc] != '[' && prog[pc] != ']') //Is this command the same as the previous one?
                    {
                        if (compR[pc-1] != 255) //Is the register for this command not full?
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
        progSize = pc; //Set actual program size
        progEnd = prog + progSize; //Set program end pointer
      //Store loop details, in loopOpens[] and loopCloses[]
        bool err = false;
        loopOpens = (char**)calloc(progSize, sizeof(char**));
        loopCloses = (char**)calloc(progSize, sizeof(char**));
        for (pc = 0; pc < progSize; pc++)
        {
            if (prog[pc] == ']')
            {
                i = 1;
                pb = prog + pc;
                do
                {
                    pb--;
                    if (*pb == ']') { i++; }
                     else if (*pb == '[') { i--; }
                } while (i && pb);
                err = i;
                loopOpens[pc] = pb;
            } else if (prog[pc] == '[') {
                i = 1; //Inception counter
                pb = prog + pc;
                do //Find matching close ']'
                {
                    pb++;
                    if (*pb == '[') { i++; }
                     else if (*pb == ']') { i--; }
                } while (i && pb != progEnd);
                err = i;
                loopCloses[pc] = pb; //Store the matching ] for the [ to use to skip to
            }
        }
        if (err) { flout("\nThis program has unmatched brackets. Will not run.\n", "31"); exit(0); }
        pb = prog; //Reset program pointer
        cb = compR;
        sizeStr = std::to_string(progSize);
        compSizeStr = std::to_string(pc);
      //Close file
        inFile.close();
      //Begin interpretation
        timeStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        std::thread(runner).detach(); //Detach the running thread
      //Display running details + stdout
        while (!termi)
        {
            display();
        }
        display(); //Final state display
        flout("\nInterpretation complete. ", "32");
        flout(std::to_string(stou.length()), "33");
        flout("B output.\n", "32");
    } else {
        flout("No program arguments given,\nsuch as ./pFI.elf test.bf\n", "31");
    }
}
