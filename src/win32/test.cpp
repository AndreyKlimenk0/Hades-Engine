#include "test.h"
#include "../libs/math/matrix.h"
#include "../libs/math/vector.h"
#include "../libs/ds/array.h"
#include "../libs/str.h"
#include "../render/render_api.h"
#include "../libs/os/file.h"
#include "../libs/key_binding.h"

typedef void (*Job_Function)(void *data);

struct Job {
	void *data = NULL;
	Job_Function *function = NULL;
};

struct Parallel_Job_List {
	Array<Job> jobs;

	void add_job(Job_Function *job_function, void *data);
	void run();
	void wait();
};


struct Parallel_Job_Manager {
	Array<Parallel_Job_List> parallel_jobs;

	void init();
};

float calculate_s(float a, float b, float c)
{
	return (a + b + c) / 2.0f;
}

float calculate_a(float a, float b, float c, float s)
{
	return math::sqrt(s * (s - a) * (s - b) * (s - c));
}

float calculate_h(float a, float base)
{
	return (a * 2.0f) / base;
}

bool intersection_test(float radius, const Vector2 &start_point, const Vector2 &end_point, const Vector2 &point)
{
	if ((start_point.x > point.x) || (point.x > end_point.x)) {
		return false;
	}

	auto A = start_point;
	auto B = end_point;
	auto C = point;

	auto a = get_length(&(A - B));
	auto b = get_length(&(C - B));
	auto c = get_length(&(A - C));

	auto base = math::max(a, math::max(b, c));

	auto s = calculate_s(a, b, c);

	auto area = calculate_a(a, b, c, s);
	auto h = calculate_h(area, base);

	return h <= radius;
}

//#define FIND_BY_FIELD(array, field, object, result) \
//  do {                                              \
//	auto func = [&]() -> decltype(array.items) { \
//		for (u32 i = 0; i < array.count; i++) { \
//			if (array[i].field == object) { \
//				return &array[i]; \
//			} \
//		} \
//		return NULL; \
//	}; \
//	result = func(); \
//  } while (0)
//
//struct Person {
//	String name;
//	int age;
//};

void test()
{
	//Array<Person> temp;
	//temp.push({ "lazel", 22 });
	//temp.push({ "wendy", 21 });

	//String x = "wendy";
	//Person *p = NULL;
	//FIND_BY_FIELD(temp, name, x, p);
	//int i = 0;
}

void update_test()
{
}
