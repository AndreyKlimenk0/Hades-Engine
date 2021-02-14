#include "test.h"

#include "../libs/ds/array.h"
#include "../render/vertex.h"
#include "../render/mesh.h"
#include "../libs/math/matrix.h"
#include "../libs/fbx_loader.h"
#include "../libs/ds/queue.h"
#include "../libs/ds/string.h"

#define FOR(data_struct, item_buffer) for (int _i; _i < data_struct.items ? item_buffer = data_struct[_i]:; _i++)

void test()
{
	Matrix4 test1 = Matrix4(Vector4(1, 2, 3, 4), Vector4(5, 6, 7, 8), Vector4(9, 10, 11, 12), Vector4(13, 14, 15, 16));
	Matrix4 test2 = Matrix4(Vector4(11, 22, 33, 44), Vector4(55, 66, 77, 88), Vector4(11, 22, 33, 44), Vector4(55, 66, 77, 88));
	Matrix4 test3 = Matrix4(Vector4(1, 0, 0, 1), Vector4(0, 2, 1, 2), Vector4(2, 1, 0, 1), Vector4(2, 0, 1, 4));
	//test3.inverse();
	//Matrix4 result = test1 * test2;
	//print_mat(result);

	//char *r = format("Andrey {}, Age {}, City {}", "Klimenko", 12, "kharkiv");
	Array<char> temp;
	temp.push('a');
	temp.push('b');
	temp.push('b');
	temp.push('c');

	char s;
	FOR(temp, s) {
		s += 1;
	}
}