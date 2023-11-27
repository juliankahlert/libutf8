// SPDX-License-Identifier: MIT

#include <stdio.h>

#include "utf8.h"

int main(int argc, char *argv[])
{
	char *str = argv[1];
	printf("%s => %d\n", str, utf8_check(str));
	printf("%s => %d\n", str, utf8_ncheck(str, 5));
	printf("%zd\n", utf8_count(str));
	printf("%zd\n", utf8_ncount(str, 2));

	utf8_stripinval(str);
	printf("%s\n", str);

	utf8_downcase(str);
	printf("downcase: %s\n", str);

	printf("%d\n", utf8_term(str));
	printf("%d\n", utf8_nterm(str, 5));

	return 0;
}

