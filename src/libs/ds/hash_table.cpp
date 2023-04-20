#include "hash_table.h"
#include "../../libs/str.h"

//uint32_t SuperFastHash(const char * data, int len);
//
//int hash(char c, const int factor, const int table_count)
//{
//	//long hash = 0;
//	//hash += (long)pow(factor, len - (i + 1));
//	//hash = hash % table_count;
//	//return (int)hash;
//	return (int)c;
//}
//
//int double_hash(char c, const int table_count, const int attempt)
//{
//	//const int hash_a = hash(string, HT_PRIME_1, table_count);
//	//const int hash_b = hash(string, HT_PRIME_2, table_count);
//	//return (hash_a + (attempt * (hash_b + 1))) % table_count;
//	return (int)c;
//}

//int hash(const char* string, const int factor, const int table_count)
//{
//	long hash = 0;
//	const int len = strlen(string);
//	for (int i = 0; i < len; i++) {
//		hash += (long)pow(factor, len - (i + 1)) * string[i];
//		hash = hash % table_count;
//	}
//	return (int)hash;
//}
//
//int double_hash(const char* string, const int table_count, const int attempt)
//{
//	const int hash_a = hash(string, HT_PRIME_1, table_count);
//	const int hash_b = hash(string, HT_PRIME_2, table_count);
//	//uint32_t hash_a = SuperFastHash(string, strlen(string));
//	//uint32_t hash_b = SuperFastHash(string, strlen(string));
//	return (hash_a + (attempt * (hash_b + attempt))) % table_count;
//}
//
//
//int double_hash(int number, const int table_count, const int attempt)
//{
//	char *str = to_string(number);
//	int hash = double_hash(str, table_count, attempt);
//	free_string(str);
//	return hash;
//}

#include <stdint.h>
#include <string.h>

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

u32 fast_hash(const char *data)
{
	u32 len = (u32)strlen(data);
	u32 hash = len, tmp;
	int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= ((signed char)data[sizeof (uint16_t)]) << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += (signed char)*data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}