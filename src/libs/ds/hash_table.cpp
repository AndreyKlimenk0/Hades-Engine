#include "hash_table.h"


int hash(const char* string, const int factor, const int table_count)
{
	long hash = 0;
	const int len = strlen(string);
	for (int i = 0; i < len; i++) {
		hash += (long)pow(factor, len - (i + 1)) * string[i];
		hash = hash % table_count;
	}
	return (int)hash;
}

int double_hash(const char* string, const int table_count, const int attempt)
{
	const int hash_a = hash(string, HT_PRIME_1, table_count);
	const int hash_b = hash(string, HT_PRIME_2, table_count);
	return (hash_a + (attempt * (hash_b + 1))) % table_count;
}
