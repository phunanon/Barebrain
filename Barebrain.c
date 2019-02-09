#include <stdio.h>
#include <stdint.h>

#define PROG_SIZE	4096
#define TAPE_LEN	1024
#define LOOP_MAX	256

enum Symbols { INC, DEC, RIG, LEF, LOO, END, PUT, GET, ZER, EOP };

int main (int argc, char* argv[])
{
	//Check if program was supplied, and attempt to open
	FILE* file;
	if (argc != 2 || (file = fopen(argv[1], "r")) == NULL) {
		puts("Supply program filename as sole argument.");
		return 1;
	}

	//Prepare stores
	uint8_t arr_p[PROG_SIZE];	//Stores program op codes
	uint8_t arr_r[PROG_SIZE];	//Stores op code repeats
	uint16_t arr_le[PROG_SIZE];	//Store LOO-to-END offsets
	uint16_t arr_el[PROG_SIZE];	//Store END-to-LOO offsets
	uint8_t arr_t[TAPE_LEN];	//Stores tape data

	uint16_t o = 0; //Opcode iterator
	//Extract op codes and their repeat counts, with some optimisation
	{
		int8_t ch; //Char buffer
		while ((ch = getc(file)) != EOF) {

			//Filter for valid op's
			switch (ch) {
				case '+': ch = INC; break; case '-': ch = DEC; break;
				case '>': ch = RIG; break; case '<': ch = LEF; break;
				case '[': ch = LOO; break; case ']': ch = END; break;
				case '.': ch = PUT; break; case ',': ch = GET; break;
				default: continue;
			}

			//Tally repeated op (ex. loop), else prepare for unique op
			if (ch == arr_p[o] && ch != LOO && ch != END)
				++arr_r[o];
			else {
				++o;
				if (o == PROG_SIZE - 1) {
					puts("Err: Exceeds maximum program size.");
					return 1;
				}
				arr_p[o] = ch;
				arr_r[o] = 1;
			}

			//Check for [-]
			if (arr_p[o] == END && o > 2 && arr_p[o-1] == DEC && arr_p[o-2] == LOO)
				arr_p[o -= 2] = ZER;
		}
	}
	fclose(file);
	
	arr_p[o+1] = EOP;	//Append End-Of-Program
	uint8_t* p = arr_p;	//Program pointer

	//Generate loop offset heuristics
	{
		uint8_t* loops[LOOP_MAX];
		uint8_t** l = loops - 1;

		while (*(++p) != EOP) {
			if (*p == LOO)
				*(++l) = p; //Append to loop queue
			else if (*p == END) {
				uint16_t offset = p - *l;
				arr_el[p  - arr_p] = offset;	//
				arr_le[*l - arr_p] = offset;	// Store offsets
				--l;
			}
		}
		p = arr_p; //Reset
	}

	//Prepare pointers and reusables
	uint8_t* t = arr_t;		//Tape pointer
	uint8_t* r = arr_r;		//Repeat pointer
	uint16_t offset;		//Loop offset cache

	//Evaluate program
	uint8_t do_run = 1;
	while (do_run) {
		switch (*p) {
			case INC: *t += *r; break;
			case DEC: *t -= *r; break;
			case RIG:  t += *r; break;
			case LEF:  t -= *r; break;
			case LOO:
				if (*t) break;
				offset = arr_le[p - arr_p];
				p += offset;	//
				r += offset;	// Jump forward
				break;
			case END:
				if (!*t) break;
				offset = arr_el[p - arr_p];
				p -= offset;	//
				r -= offset;	// Jump backward
				break;
			case PUT: for (uint8_t i = 0; i < *r; ++i) putchar(*t);		break;
			case GET: for (uint8_t i = 0; i < *r; ++i) *t = getchar();	break;
			case ZER: *t = 0; break;
			case EOP: do_run = 0; break;
		}
		++p;
		++r;
	}
}
