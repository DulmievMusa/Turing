#include<stdio.h>
#include<stdlib.h>
#include<string.h>

typedef struct _rule {
	char read;
	char q[64];
	char write;
	char nq[64];
	char dir;
	struct _rule* next;
} rule;

rule* machine = NULL;
char state[64];

int read_machine(FILE* f) {
	rule r;
	int empty = 1;
	while (fscanf(f, "%63[^:]:'%c'=>%63[^:]:'%c'%c ",
			r.q, &r.read, r.nq, &r.write, &r.dir) == 5) {
		r.dir = (r.dir=='<'?-1:1);
		if (empty) { strcpy(state, r.q); empty = 0; } 
		rule* nr = malloc(sizeof(rule));
		if (!nr) return -fprintf(stderr, "Error while allocating rule for machine\n");
		*nr = r;
		nr->next = machine;
		machine = nr;
	}
	return 0;
}

char* tape;
size_t tape_cap;
#define POS2IDX(p)((p)<0?-(p)*2+1:(p)*2)
#define POS2TAPE(p) (tape[POS2IDX(p)])

int dump_tape(int pos) {
	printf("State: %s\npos:%d\n", state, pos);
	for ( int i = pos-10; i < pos+20; i++ )
		putchar(POS2IDX(i)<tape_cap?POS2TAPE(i):'~');
	putchar('\n');
	for ( int i = pos-10; i < pos; i++ ) putchar(' ');
	putchar('^');
	putchar('\n');
	return 0;
}

int interpret(int pos, int step) {
	if ( POS2IDX(pos) >= tape_cap) {
		tape = realloc(tape, tape_cap *= 2);
		memset(tape+POS2IDX(pos), ' ', tape_cap/2);
	}
	dump_tape(pos);
	for(rule* r = machine; r; r=r->next) {
		if ( !strcmp(state, r->q) && POS2TAPE(pos) == r->read ) {
			POS2TAPE(pos) = r->write;
			strcpy(state, r->nq);
			return interpret(pos+r->dir, step+1);
		}
	}
	return !fprintf(stderr, "Machine stopped at step %d\n", step);
}

int read_tape(FILE* f, int* ret) {
	int c;
	int pos = 0;
	tape = malloc(tape_cap = 1024);
	memset(tape, ' ', tape_cap);
	if (!tape) return -fprintf(stderr, "Cannot allocate space for tape\n");
	while((c = fgetc(f)) != '\n') { POS2TAPE(pos) = c; pos++;}
	*ret = 0;
	while((c = fgetc(f)) != '^' && c != EOF) (*ret)++;
	return 0;
}

void free_machine() {
	if ( !machine ) return;
	rule* n = machine->next;
	free(machine);
	machine = n;
	free_machine();
}

int main(int argc, char* argv[]) {
	int pos;
	int err = 0;
	if ( argc < 3 )
		return -fprintf(stdout, "Usage: %s <prog> <tape>\n", argv[0]);
	
	FILE* fprog = fopen(argv[1], "r");
	if ( !fprog ) return -fprintf(stderr, "Cannot open prog file %s\n", argv[1]);
	if (read_machine(fprog)) goto close_fprog;
	FILE* ftape = fopen(argv[2], "r");
	if ( !ftape ) goto clean_prog;
	if (read_tape(ftape, &pos) ) goto close_tape;
	interpret(pos, 0);
clean_tape:
	free(tape);
close_tape:
	fclose(ftape);
clean_prog:
	free_machine();
close_fprog:
	fclose(fprog);
	return err;
}
