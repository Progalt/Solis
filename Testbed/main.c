
#include <stdio.h>

#include <solis.h>

const char* source =
"var x = 25 + 5;\n"
"var y = x + 2;";

int main(void) {

	VM vm;
	solisInitVM(&vm);

	InterpretResult result = solisInterpret(&vm, source);

	solisFreeVM(&vm);


	return 0;
}