#include "test.h"
#include "../libs/math/matrix.h"
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
	//float r = XMConvertToRadians(90.0f);
	//double x = cos(1.5708);
	//print("COCCCCCCCCCCCS", x);
	//Matrix3 temp = { math::cos(r), 0.0f, math::sin(r),
	//				0.0f, 1.0f, 0.0f,
	//				-math::sin(r), 0.0f, math::cos(r) };
	Vector4 z1 = Vector4(Vector3::base_x, 0.0f);
	Vector4 z2 = Vector4(Vector3::base_x, 0.0f);
	Matrix4 camera = rotate_about_y(XMConvertToRadians(-90.0f));
	Matrix4 invcamera = inverse(rotate_about_y(XMConvertToRadians(-90.0f)));
	z1 = z1 * camera;
	z2 = z2 * invcamera;
	print("Z1", &z1);
	print("Z2", &z2);
	int break_point = 0;
}

void update_test()
{
}

