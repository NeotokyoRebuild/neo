#include "cbase.h"
#include "inttostr.h"

#include <stdio.h>
#include <stdlib.h>

void inttostr(char *str, const int strSize, const int srcInt)
{
#ifdef _WIN32
	itoa(srcInt, str, strSize);
#else
	snprintf(str, strSize, "%d", srcInt);
#endif
}
