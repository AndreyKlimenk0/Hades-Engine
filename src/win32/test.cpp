#include "test.h"
#include <string.h>

#include "win_local.h"
#include "win_time.h"

#include "../render/vertex.h"
#include "../libs/math/matrix.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"
#include "../libs/ds/array.h"
#include "../game/entity.h"


String *enum_string_to_common(const char *str)
{
	String string = str;
	Array<String> buffer;
	
	split(&string, "_", &buffer);

	String *ptr_str = NULL;
	FOR(buffer, ptr_str) {
		ptr_str->to_lower();
	}

	char d = ('a' - 'A');
	buffer[0].data[0] -= d;

	String *new_str = new String();
	FOR(buffer, ptr_str) {
		new_str->append(ptr_str);
		new_str->append(" ");
	}
	new_str->data[new_str->len - 1] = '\0';
	return new_str;
}

#define ENUM_TO_STRING(e) print("Enum {}", enum_string_to_common(#e)->to_str());

struct Demo {
	Demo() {};
	Demo(int id) : id(id) {}
	int id = 0;
	~Demo() { print("an object of demo struct with id {} and address is deleted", id); }
};


void test()
{
	ENUM_TO_STRING(ENTITY_TYPE_MUTANT);
}