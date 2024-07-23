#include "test.h"
#include "../libs/str.h"
#include "../sys/sys.h"

void test()
{
	const char *null_str = NULL;
	const char *empty_str = "";
	const char *almost_empty = " a";
	print(string_null_or_empty(null_str));
	print(string_null_or_empty(empty_str));
	print(string_null_or_empty(almost_empty));
}

void update_test()
{
}
