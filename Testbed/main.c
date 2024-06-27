
#include <stdio.h>

#include <solis.h>

const char* source =
"\"Hello\" + \" World\" \n";

int main(void) {

	VM vm;
	solisInitVM(&vm);

	InterpretResult result = solisInterpret(&vm, source);

	solisFreeVM(&vm);


	return 0;
}