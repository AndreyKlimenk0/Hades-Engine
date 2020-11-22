#include <string.h>
#include "string.h"

char *concatenate_c_str(const char *str1, const char *str2)
{
	int str1_len = strlen(str1);
	int str2_len = strlen(str2);
	int len = str1_len + str2_len;
	char *new_string = new char[len + 1];

	memcpy(new_string, str1, str1_len);
	memcpy(new_string + (sizeof(char) * str1_len), str2, str2_len);
	new_string[len] = '\0';

	return new_string;
}


Array<char *> *split(char *string, const char *characters)
{
	Array<char *> *temp = new Array<char *>();
	char *next = NULL;
	char *token = strtok_s(string, characters, &next);
	while (token != NULL) {
		temp->push(token);
		token = strtok_s(NULL, characters, &next);
	}
	return temp;
}