#include <stdio.h>
#include <stdint.h>

#define INC 0x0
#define DEC 0x1
#define RIG 0x2
#define LEF 0x3
#define LOO 0x4
#define END 0x5
#define PUT 0x6
#define GET 0x7
#define ZER 0x8
#define TAPE_LEN 30000
#define PROG_SIZE 30000

int main (int argc, char *argv[])
{
	//Check if program was supplied, and attempt to open
	FILE *fp;
	if (argc != 2 || (fp = fopen(argv[1], "r")) == NULL) {
		puts("Err: Supply program filename as sole argument.");
		return 1;
	}
	
	//Prepare stores
	uint8_t arr_p[PROG_SIZE];	//Stores program op codes
	uint8_t arr_r[PROG_SIZE];	//Stores op code repeats
	uint16_t arr_lc[PROG_SIZE];	//Stores offset to close op pointer at open op
	uint16_t arr_lo[PROG_SIZE];	//Stores offset to open op pointer at close op
	uint8_t arr_t[TAPE_LEN];	//Stores tape data
	
	//Extract op codes and their repeat counts, with some optimisation
	uint16_t o = 0; //Opcode iterator
	{
		uint16_t a = 0; //Input iterator
		int8_t ch; //Character buffer
		while ((ch = getc(fp)) != EOF) {

			//Filter for valid op's
			switch (ch) {
				case '+': ch = INC; break; case '-': ch = DEC; break;
				case '>': ch = RIG; break; case '<': ch = LEF; break;
				case '[': ch = LOO; break; case ']': ch = END; break;
				case '.': ch = PUT; break; case ',': ch = GET; break;
				default: continue;
			}
			
			//Check if op is repeated (but not for loop ops)
			if (ch == arr_p[o] && ch != LOO && ch != END) ++arr_r[o];
			else {
				++o;
				if (o == PROG_SIZE) {
					puts("Err: Maximum program size exceeded.");
					return 1;
				}
				arr_p[o] = ch;
				arr_r[o] = 1;
			}
			
			//Check for [-]
			if (o > 2 && arr_p[o] == END && arr_p[o-1] == DEC && arr_p[o-2] == LOO) {
				o -= 2;
				arr_p[o] = ZER;
			}
		}
		fclose(fp);
	}
	
	uint8_t *p = arr_p;				//Program pointer
	uint8_t *p_end = arr_p + o + 1;	//Program end pointer

	//Generate loop offset heuristics
	{
		uint8_t inception = 0;
		uint8_t *inspect;
		uint8_t loopErr = 0;
		do {

			if (*p == END) {
				inception = 1;
				inspect = p;
				do {
					--inspect;
					if (*inspect == END) ++inception;
					else if (*inspect == LOO) --inception;
				} while (inception && inspect != arr_p);
				arr_lo[p - arr_p] = p - inspect; //Store offset
			} else if (*p == LOO) {
				inception = 1;
				inspect = p;
				do {
					++inspect;
					if (*inspect == LOO) ++inception;
					else if (*inspect == END) --inception;
				} while (inception && inspect != p_end);
				arr_lc[p - arr_p] = inspect - p;	//Store offset
			}

			if (inception) {
				puts("Err: Unmatched loops.");
				return 1;
			}

		} while (++p != p_end);
		p = arr_p; //Reset
	}

	//Prepare pointers and reusables
	uint8_t *t = arr_t;		//Tape pointer
	uint8_t *r = arr_r;		//Repeat pointer
	uint16_t offset;		//Stores loop offset

	//Begin evaluating program
	do {
		switch (*p) {
			case INC: *t += *r;	break;
			case DEC: *t -= *r;	break;
			case RIG: t += *r;	break;
			case LEF: t -= *r;	break;
			case LOO:
				if (*t) break;
				offset = arr_lc[p - arr_p];
				p += offset;	//
				r += offset;	// Jump forward
				break;
			case END:
				if (!*t) break;
				offset = arr_lo[p - arr_p];
				p -= offset;	//
				r -= offset;	// Jump backward
				break;
			case PUT: for (uint8_t i = 0; i < *r; ++i) putchar(*t);		break;
			case GET: for (uint8_t i = 0; i < *r; ++i) *t = getchar();	break;
			case ZER: *t = 0; break;
			break;
		}
		++p;
		++r;
	} while (p != p_end);

	return 0;
}
