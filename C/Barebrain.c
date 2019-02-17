#include <stdio.h>
#include <stdint.h>

#define PROG_MAX	16384
#define TAPE_LEN	512
#define LOOP_MAX	256

enum Symbols { INC='+', DEC='-', RIG='>', LEF='<', LOO='[', END=']', PUT='.', GET=',', ZER='0', EOP=0 };

int main (int argc, char* argv[])
{
	FILE* file;
	//Check if program was supplied, and attempt to open
	if (argc != 2 || (file = fopen(argv[1], "r")) == NULL) {
		puts("Supply program filename as sole argument.");
		return 1;
	}

	uint8_t arr_p[PROG_MAX] = {EOP};//Program op code tape
	uint8_t arr_r[PROG_MAX];		//Op code repeat index
	uint16_t arr_o[PROG_MAX];		//LOO-to-END & END-to-LOO offset index
	//Extract op codes, tally repeats, generate loop offset index heuristic
	{
		uint16_t loops[LOOP_MAX];	//Loop op code stack
		uint16_t* l = loops - 1;	//Loop stack pointer
		uint16_t o = 0;				//Opcode iterator
		int8_t ch;					//Char buffer
		while ((ch = getc(file)) != EOF) {
			//Filter for valid op's
			switch (ch) {
				case INC: case DEC: case RIG: case LEF: case LOO: case END: case PUT: case GET: break;
				default: continue;
			}

			//Tally repeated op (ex. loop), else prepare for next unique op
			if (ch == arr_p[o] && ch != LOO && ch != END)
				++arr_r[o];
			else {
				if (++o == PROG_MAX - 1) {
					puts("Exceeds maximum program size.");
					return 1;
				}
				arr_p[o] = ch;
				arr_r[o] = 1;

				//Loop heuristic
				if (ch == LOO)
					*(++l) = o; //Append to loop stack
				else if (ch == END) {
					arr_o[o]  =			//
					arr_o[*l] = o - *l;	// Store offsets
					--l;
				}

				//Optimise away potential [-]
				if (ch == END && o > 2 && arr_p[o-1] == DEC && arr_p[o-2] == LOO)
					arr_p[o -= 2] = ZER;
			}
		}
	}
	fclose(file);

	uint8_t arr_t[TAPE_LEN] = {0};	//Tape data store
	uint8_t* t = arr_t;				//Tape pointer
	uint8_t* p = arr_p+1;			//Program pointer
	uint8_t* r = arr_r+1;			//Repeat pointer
	uint16_t offset;				//Loop offset cache
	//Evaluate program
	for (; *p != EOP; ++p, ++r) {
		switch (*p) {
			case INC: *t += *r; break;
			case DEC: *t -= *r; break;
			case ZER: *t  =  0; break;
			case RIG:  t += *r; break;
			case LEF:  t -= *r; break;
			case LOO:
				if (*t) break;
				offset = arr_o[p - arr_p];
				p += offset;	//
				r += offset;	// Jump forward
				break;
			case END:
				if (!*t) break;
				offset = arr_o[p - arr_p];
				p -= offset;	//
				r -= offset;	// Jump backward
				break;
			case PUT: for (uint8_t i = 0; i < *r; ++i) putchar(*t);		break;
			case GET: for (uint8_t i = 0; i < *r; ++i) *t = getchar();	break;
		}
	}
}
