#include "test.h"
#include "../libs/math/common.h"
#include "../libs/ds/array.h"
#include "../libs/str.h"

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

void test()
{
	Point_f32 temp;
	Point_f32 point1 = { 22.4f, 10.0f, -19.2f };
	Point_f32 point2 = { -10.4f, 54.1f, -0.2f };
	float result = find_distance(&point1, &point2);
	//print("Distance !!!!!!!!", result);
}

void update_test()
{
}

