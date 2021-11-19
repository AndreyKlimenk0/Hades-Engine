#include <stdio.h>
#include "test.h"
#include "../sys/sys_local.h"

#include "../libs/spng.h"
#include "../libs/os/file.h"
#include <string.h>
#include <stdlib.h>
#include "../game/world.h"
#include <tuple>

#define TO_STRING(x) (#x)
#define ADD_INFO(struct_info, member) struct_info.add_info(#member, offsetof(decltype(struct_info.type), member))


template <typename T>
struct Struct_Info {	
	T type;
	Hash_Table<String, u32> member_offsets;

	void add_info(const char *member_name, u32 offset);
	void fill_struct(u32 *struct_address, Void_Dict *dict);
	T *make_struct(Void_Dict *dict);
};

template <typename T>
void Struct_Info<T>::add_info(const char *member_name, u32 offset)
{
	member_offsets.set(member_name, offset);
}

template<typename T>
void Struct_Info<T>::fill_struct(u32 *struct_address, Void_Dict *dict)
{
		for (int i = 0; i < dict->storage.count; i++) {
		Hash_Node<String, Data_Ptr> *node = dict->storage.get_node(i);
		String name = node->key;
		Data_Ptr *data_ptr = &node->value;

		u32 _offset;
		if (member_offsets.get(name, _offset)) {
			u32 *member_address = (u32 *)((u32)struct_address + _offset);
			memcpy((void *)member_address, data_ptr->ptr, data_ptr->size);
		}
	}
}

template <typename T>
T *Struct_Info<T>::make_struct(Void_Dict *dict)
{
	T *new_struct = new T;

	for (int i = 0; i < dict->storage.count; i++) {
		Hash_Node<String, Data_Ptr> *node = dict->storage.get_node(i);
		String name = node->key;
		Data_Ptr *data_ptr = &node->value;

		u32 _offset;
		if (member_offsets.get(name, _offset)) {
			u32 *member_address = (u32 *)((u32)new_struct + _offset);
			memcpy((void *)member_address, data_ptr->ptr, data_ptr->size);
		}
	}
	return new_struct;
}

// Use with this auto &var_name
#define GET_STRUCT_INFO(type_name) (*get_struct_info(type_name))

enum Func_Arg_Pass_By {
	FUNC_ARG_PASS_BY_VALUE,
	FUNC_ARG_PASS_BY_PTR,
	FUNC_ARG_PASS_BY_REFERENCE
};

struct Func_Arg_Info {
	String type;
	String name;
	Func_Arg_Pass_By pass_by;
};

struct Function_Info {
	Function_Info(const char *func_name, const char *return_value_type) : func_name(func_name), return_value_type(return_value_type) {}
	
	String func_name;
	String return_value_type;
	Array<Func_Arg_Info> function_args;

	void add_function_arg(const char *type, const char *name, Func_Arg_Pass_By arg_pass_by = FUNC_ARG_PASS_BY_VALUE);
};

void Function_Info::add_function_arg(const char *type, const char *name, Func_Arg_Pass_By pass_by)
{
	Func_Arg_Info func_arg_info;
	func_arg_info.type = type;
	func_arg_info.name = name;
	func_arg_info.pass_by = pass_by;
	function_args.push(func_arg_info);
}

struct Make_Point_LIght_Func {
	Make_Point_LIght_Func();

	Vector3 position;
	Vector3 color;
	float radius;

	Hash_Table<String, u32> member_offsets;
};

Make_Point_LIght_Func::Make_Point_LIght_Func()
{
	
}

void make_function_info(Function_Info *func_info)
{
	
}


struct Test {
	int i;
	float f;
	Vector3 v;
};

void test()
{
	String temp = "123";
	temp.insert(0, 'c');
	temp.print();
	//temp.insert(0, 'c')

}