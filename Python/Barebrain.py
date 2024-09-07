import sys
import getch
import collections

if len(sys.argv) != 2:
	print("Supply program filename as sole argument.")
	exit()
file = open(sys.argv[1], 'r')

arr_p = [''] #Program op code tape
arr_r = [0]  #Op code repeat index
arr_o = [0]  #LOO-to-END & END-to-LOO offset index
p = 0        #Program pointer

#Extract op codes from file, tally repeats, generate loop offset index heuristic
loop_stack = collections.deque()
while True:
	char = file.read(1)
	if not char: break

	if char not in ('>', '<', '+', '-', '.', ',', '[', ']'): continue

	#Tally repeats, else accept unique op
	if char == arr_p[p] and char not in ('[', ']', '.', ','):
		arr_r[p] += 1
	else:
		p += 1
		prev_char = char
		arr_p.append(char)
		arr_r.append(1)
		arr_o.append(0)

		#Loop heuristic
		if char == '[': loop_stack.append(p)
		elif char == ']':
			loop_open = loop_stack.pop()
			arr_o[loop_open] = arr_o[p] = p - loop_open

		#Optimise away potential [-]
		if arr_p[-3:] == list("[-]"):
			p -= 2
			del arr_p[-2:]
			arr_p[p] = '0'

#Evaluate program
p = 1
arr_t = [0]*10000 #Tape
t = 0             #Tape pointer

while p < len(arr_p):
	op = arr_p[p]
	if   op == '+': arr_t[t] += arr_r[p]
	elif op == '-': arr_t[t] -= arr_r[p]
	elif op == '0': arr_t[t] = 0
	elif op == '>': t += arr_r[p]
	elif op == '<': t -= arr_r[p]
	elif op == '.': print(chr(arr_t[t]), end='', flush=True)
	elif op == ',': arr_t[t] = ord(getch.getch())
	elif op == '[':
		if arr_t[t] == 0:
			p += arr_o[p]
	elif op == ']':
		if arr_t[t] != 0:
			p -= arr_o[p]
	p += 1
