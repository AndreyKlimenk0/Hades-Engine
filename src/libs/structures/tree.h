#ifndef TREE_H
#define TREE_H

#include <stdlib.h>

#include "array.h"
#include "../number_types.h"
#include "../../sys/sys.h"

template <typename T>
struct Ordered_Tree {
	bool set_default_data = false;
	T default_data;

	struct Node {
		Node *parent_node = NULL;
		T data;
		Array<Node *> children;

		bool has_children()
		{
			return !children.is_empty();
		}
	};

	Node *root_node = NULL;

	void free();
	void print();
	void print_node(u32 level, Array<Ordered_Tree<T>::Node *> &level_nodes);
	void setup_default_data(const T &data);
	void delete_node(Ordered_Tree<T>::Node **node);
	void delete_child_nodes(Ordered_Tree<T>::Node **node);
	
	void walk_and_update_nodes(Ordered_Tree<T>::Node *parent_node, void (*update)(T *data, void *args), void *args);

	Ordered_Tree<T>::Node *find_node(const T &data);
	Ordered_Tree<T>::Node *find_node(Ordered_Tree<T>::Node *node, const T &data);
	
	Ordered_Tree<T>::Node *create_root_node(const T &data);
	Ordered_Tree<T>::Node *create_child_node(Ordered_Tree<T>::Node *parent_node, const T &data);
	Ordered_Tree<T>::Node *create_child_node(Ordered_Tree<T>::Node *parent_node, const T &data, u32 index);
	
	Ordered_Tree<T>::Node *get_child_node(Ordered_Tree::Node *parent_node, u32 index);
	Ordered_Tree<T>::Node *get_or_create_child_node(Ordered_Tree::Node *parent_node, u32 index);
};

template <typename T>
void Ordered_Tree<T>::setup_default_data(const T &data)
{
	set_default_data = true;
	default_data = data;
}

template<typename T>
void Ordered_Tree<T>::delete_node(Ordered_Tree<T>::Node **node)
{
	if ((node == NULL) || (*node == NULL)) {
		return;
	}
	Node *_node = *node;
	Node *parent_node = _node->parent_node;
	if (parent_node) {
		for (u32 i = 0; i < parent_node->children.count; i++) {
			if (parent_node->children[i] == _node) {
				parent_node->children.remove(i);
				break;
			}
		}
	}
	delete_child_nodes(node);
}

template<typename T>
void Ordered_Tree<T>::delete_child_nodes(Ordered_Tree<T>::Node **node)
{
	if ((node == NULL) || (*node == NULL)) {
		return;
	}

	Node *_node = *node;
	if (_node->has_children()) {
		Node *child = NULL;
		For(_node->children, child) {
			delete_node(&child);
		}
		_node->children.clear();
	}
	DELETE_PTR(_node);
	*node = NULL;
}

template<typename T>
inline void Ordered_Tree<T>::walk_and_update_nodes(Ordered_Tree<T>::Node *parent_node, void (*update)(T *data, void *args), void *args)
{
	update(&parent_node->data, args);
	for (u32 i = 0; i < parent_node->children.count; i++) {
		walk_and_update_nodes(parent_node->children[i], update, args);
	}
}

template<typename T>
Ordered_Tree<T>::Node *Ordered_Tree<T>::find_node(const T &data)
{
	Node *node = NULL;
	if (root_node) {
		node = find_node(root_node, data);
	}
	return node;
}

template<typename T>
Ordered_Tree<T>::Node *Ordered_Tree<T>::find_node(Ordered_Tree<T>::Node *node, const T &data)
{
	if (node->data == const_cast<T &>(data)) {
		return node;
	} else {
		if (node->has_children()) {
			Node *child = NULL;
			For(node->children, child) {
				Node *node = find_node(child, data);
				if (node) {
					return node;
				}
			}
		}
	}
	return NULL;
}

template<typename T>
Ordered_Tree<T>::Node *Ordered_Tree<T>::create_root_node(const T &data)
{
	if (root_node) {
		root_node->data = data;
	} else {
		root_node = new Node();
		root_node->data = data;
	}
	return root_node;
}

template<typename T>
Ordered_Tree<T>::Node *Ordered_Tree<T>::create_child_node(Ordered_Tree<T>::Node *parent_node, const T &data)
{
	Node *node = new Node();
	node->data = data;
	node->parent_node = parent_node;
	parent_node->children.push(node);
	return node;
}

template<typename T>
inline Ordered_Tree<T>::Node *Ordered_Tree<T>::create_child_node(Ordered_Tree<T>::Node *parent_node, const T &data, u32 index)
{
	if (index >= parent_node->children.count) {
		s32 new_child_count = (index + 1) - parent_node->children.count;
		for (s64 i = 0; i < ((s64)new_child_count - 1); i++) {
			parent_node->children.push(NULL);
		}
		Node *node = new Node();
		node->parent_node = parent_node;
		node->data = data;
		parent_node->children.push(node);
		return parent_node->children[index];
	}
	if (!parent_node->children[index]) {
		Node *node = new Node();
		node->parent_node = parent_node;
		parent_node->children[index] = node;
	}
	parent_node->children[index]->data = data;
	return parent_node->children[index];
}

template<typename T>
inline Ordered_Tree<T>::Node *Ordered_Tree<T>::get_child_node(Ordered_Tree::Node *parent_node, u32 index)
{
	if (index < parent_node->children.count) {
		return parent_node->children[index];
	}
	return NULL;
}

template <typename T>
Ordered_Tree<T>::Node *Ordered_Tree<T>::get_or_create_child_node(Ordered_Tree::Node *parent_node, u32 index)
{
	if (index >= parent_node->children.count) {
		u32 new_child_count = (index + 1) - parent_node->children.count;
		for (u32 i = 0; i < new_child_count; i++) {
			Node *node = new Node();
			node->parent_node = parent_node;
			if (set_default_data) {
				node->data = default_data;
			}
			parent_node->children.push(node);
		}
	}
	return parent_node->children[index];
}

template<typename T>
void Ordered_Tree<T>::free()
{
	if (root_node) {
		delete_node(&root_node);
	}
}

template <typename T>
void Ordered_Tree<T>::print()
{
	if (root_node) {
		Array<Node *> only_root_node;
		only_root_node.push(root_node);
		print_node(0, only_root_node);
		print_same_line("\n");
	}
}

template<typename T>
void Ordered_Tree<T>::print_node(u32 level, Array<Ordered_Tree<T>::Node *> &level_nodes)
{
	if (level > 0) {
		print_same_line("\n");
	}
	print_same_line("Level[{}]", level);
	Array<Node *> next_level_nodes;
	for (u32 i = 0; i < level_nodes.count; i++) {
		Node *node = level_nodes[i];
		print_same_line(" Child(idx = {}, data = {})", i, node->data);
		for (u32 j = 0; j < node->children.count; j++) {
			next_level_nodes.push(node->children[j]);
		}
	}
	if (!next_level_nodes.is_empty()) {
		print_node(level + 1, next_level_nodes);
	}
}
#endif
