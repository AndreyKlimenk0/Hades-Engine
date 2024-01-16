#ifndef HADES_STACK
#define HADES_STACK

#include "array.h"
#include "../../win32/win_types.h"

template< typename T>
struct Stack {
	Array<T> stack;

	void push(const T &item);
	void pop();
	bool is_empty();
	u32 count();
	T &top();
};

template <typename T>
inline void Stack<T>::push(const T &item)
{
	stack.push(item);
}

template<typename T>
inline void Stack<T>::pop()
{
	stack.pop();
}

template<typename T>
inline bool Stack<T>::is_empty()
{
	return stack.is_empty();
}

template<typename T>
inline u32 Stack<T>::count()
{
	return stack.count;
}

template<typename T>
inline T & Stack<T>::top()
{
	assert(!is_empty());
	return stack.get_last();
}
#endif