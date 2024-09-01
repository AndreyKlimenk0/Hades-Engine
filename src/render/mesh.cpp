#include "mesh.h"
#include "../libs/math/functions.h"
#include "../sys/sys.h"

const char *Triangle_Mesh::get_name()
{
	assert(exclusive_or(!name.is_empty(), !file_name.is_empty()));
	return name.is_empty() ? file_name.c_str() : name.c_str();
}

String Triangle_Mesh::get_pretty_name()
{
	String result = "";
	if (!name.is_empty() && !file_name.is_empty()) {
		result.move(format("'{}' mesh from {}", name, file_name));
	} else if (exclusive_or(name.is_empty(), file_name.is_empty())) {
		result = get_name();
	}
	return result;
}
