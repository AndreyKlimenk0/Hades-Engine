#include "test.h"

#include "win_local.h"
#include "win_time.h"

#include "../render/vertex.h"
#include "../libs/math/matrix.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/ds/hash_table.h"
#include "../libs/ds/linked_list.h"
#include "../libs/ds/array.h"


struct Demo {
	Demo() {};
	Demo(int id) : id(id) {}
	int id = 0;
	~Demo() { print("an object of demo struct with id {} and address is deleted", id); }
};


void test()
{
	//Linked_List<int> list;
	//list.append(11);
	//list.append(22);
	//list.append(33);
	//list.append(44);
	//list.append(55);
	//list.print();
	//
	//int *ptr = NULL;
	//START_PROFILE;
	//FOR(list, ptr) {
	//	print("linked list item", *ptr);
	//}
	//int x;
	//list.remove(33, x);
	//print("X ", x);
	//list.remove(44, x);
	//print("X ", x);
	//list.remove(11, x);
	//print("X ", x);

	//list.print();
	//int &temp = list.find(33);
	//int &one = list.find((u32)1);
	//print("temp", temp);
	//print("one", one);

	//END_PROFILE;

	//list.set_pointer_to_item(&buffer, 4);
	//print("buffer", *buffer);

	//Demo *demo = new Demo(11);
	//Demo *demo1 = new Demo(22);
	//Demo *demo2 = new Demo(33);


	//Linked_List<Demo *> ptr_list;
	//ptr_list.append(demo);
	//ptr_list.append(demo1);
	//ptr_list.append(demo2);

	//if (ptr_list.first_node->item == demo) {
	//	print("The same");
	//} else {
	//	print("diffrent");
	//}



	//Linked_List<int> int_list;
	//int_list.append(1);
	//int_list.append(2);
	//int_list.append(3);
	//int_list.append(4);


	//int *ptr = NULL;
	//FOR(int_list, ptr) {
	//	print("linked list item", *ptr);
	//}

	//Node<int> *temp = NULL;
	//int_list.remove_node(3, &temp);
	//int_list.append(temp);
	//int_list.remove_node(2, &temp);
	//int_list.append(temp);
	//int_list.remove_node(1, &temp);
	//int_list.append(temp);

	//print("--------------------------");
	//FOR(int_list, ptr) {
	//	print("linked list item", *ptr);
	//}
	//
	//print("Ptr list");
	//
	//int *int1 = new int;
	//*int1 = 11;
	//
	//int *int2 = new int;
	//*int2 = 22;
	//
	//int *int3 = new int;
	//*int3 = 33;
	//
	//int *int4 = new int;
	//*int4 = 44;
	//
	//Linked_List<int *> ptr_list;
	//ptr_list.append(int1);
	//ptr_list.append(int2);
	//ptr_list.append(int3);
	//ptr_list.append(int4);
	//
	//int *p = NULL;
	//FOR(ptr_list, p) {
	//	print(*p);
	//}

	//Node<int *> *n = NULL;
	//ptr_list.remove_node(int1, &n);
	//ptr_list.append(n);
	//ptr_list.remove_node(int3, &n);
	//ptr_list.append(n);
	//ptr_list.remove_node(int2, &n);
	//ptr_list.append(n);
	//ptr_list.remove_node(int1, &n);
	//ptr_list.append(n);
	//print("node", *n->item);

	//print("----------------------------");
	//FOR(ptr_list, p) {
	//	print(*p);
	//}

}