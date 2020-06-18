#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define PROG_MAX  16384
#define TAPE_LEN  512
#define LOOP_MAX  256

int main (int argc, char* argv[])
{
  FILE* file;
  //Check if program was supplied, and attempt to open
  if (argc != 2 || (file = fopen(argv[1], "r")) == NULL)
    puts("Supply program filename as sole argument."),
    exit(1);

  uint8_t  arr_p[PROG_MAX] = {0}; //Program op code tape
  uint8_t  arr_r[PROG_MAX];       //Op code repeat index
  uint16_t arr_o[PROG_MAX];       //START-to-END & END-to-START offset index
  //Extract op codes, tally repeats, generate loop offset index heuristic
  {
    uint16_t  loops[LOOP_MAX];    //Loop op code stack
    uint16_t* l = loops - 1;      //Loop stack pointer
    uint16_t  o = 0;              //Opcode iterator
    int8_t    ch;                 //Char buffer
    while ((ch = getc(file)) != EOF) {
      //Tally repeated op (ex. loop), else prepare for next unique op
      if (ch == arr_p[o] && ch != '[' && ch != ']') {
        ++arr_r[o];
        continue;
      }
      if (++o == PROG_MAX - 1)
        puts("Exceeds maximum program size."),
        exit(1);
      arr_p[o] = ch;
      arr_r[o] = 1;

      //Loop heuristic
      if (ch == '[')
        *(++l) = o;          //Append to loop stack
      else if (ch == ']') {
        arr_o[o]  =          //
        arr_o[*l] = o - *l;  // Store offsets
        --l;
      }

      //Optimise away potential [-]
      if (ch == ']' && o > 2 && arr_p[o - 1] == '-' && arr_p[o - 2] == '[')
        arr_p[o -= 2] = 255;
    }
  }
  fclose(file);

  uint8_t  arr_t[TAPE_LEN] = {0}; //Tape data store
  uint8_t* t = arr_t;             //Tape pointer
  uint8_t* p = arr_p + 1;         //Program pointer
  uint8_t* r = arr_r + 1;         //Repeat pointer
  uint16_t offset;                //Loop offset cache
  //Evaluate program
  for (; *p; ++p, ++r) {
    switch (*p) {
      case '+': *t += *r; break;
      case '-': *t -= *r; break;
      case 255: *t  =  0; break;
      case '>':  t += *r; break;
      case '<':  t -= *r; break;
      case '[':
        if (*t) break;
        offset = arr_o[p - arr_p];
        p += offset;  //
        r += offset;  // Jump forward
        break;
      case ']':
        if (!*t) break;
        offset = arr_o[p - arr_p];
        p -= offset;  //
        r -= offset;  // Jump backward
        break;
      case '.': for (uint8_t i = 0; i < *r; ++i) putchar(*t);    break;
      case ',': for (uint8_t i = 0; i < *r; ++i) *t = getchar(); break;
    }
  }
}
