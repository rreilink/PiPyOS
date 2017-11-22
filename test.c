#include <string.h>
#include <stdio.h>


int main(void) {
	char a[200];
	const char *b="test";
	strcpy(a, b);
	printf("a=%s b=%s\n", a, b);
	return 0;

}

