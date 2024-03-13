#include "test.h"
#include "../libs/math/matrix.h"
#include "../libs/ds/array.h"
#include "../libs/str.h"
#include "../libs/os/file.h"

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
}

void update_test()
{
}

