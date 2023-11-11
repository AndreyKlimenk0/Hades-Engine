#include "test.h"
#include "../libs/ds/array.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../win32/win_local.h"
#include "../libs/math/common.h"


 //A jittering sampling filter is a 2d array with N * N size. The filter holds random generated offets
 //Each sampled texel from shadow map in a pixel shader is mapped into one filter.

static float generate_random_offset()
{
	return ((float)rand() / (float)RAND_MAX) - 0.5f; 	//Generate random offset between -0.5 and 0.5.
}

static void make_jittering_sampling_filters(u32 tile_size, u32 filter_size)
{
	Array<Vector2> jittered_samples;
	jittered_samples.reserve(math::pow2(tile_size) * math::pow2(filter_size));

	u32 index = 0;
	for (u32 tile_row = 0; tile_row < tile_size; tile_row++) {
		for (u32 tile_col = 0; tile_col < tile_size; tile_col++) {
			for (u32 filter_row = 0; filter_row < filter_size; filter_row++) {
				for (u32 filter_col = 0; filter_col < filter_size; filter_col++) {

					float v = ((float)filter_row + 0.5f + generate_random_offset()) / filter_size;
					float u = ((float)filter_col + 0.5f + generate_random_offset()) / filter_size;

					jittered_samples[index].x = math::sqrt(v) * math::cos(2 * PI * u);
					jittered_samples[index].y = math::sqrt(v) * math::sin(2 * PI * u);
					print("({}, {}) -> ({}, {})", u, v, jittered_samples[index].x, jittered_samples[index].y);
					index++;
				}
			}
			print("---------------------------------------");
		}
	}

	//for (u32 i = 0; i < (math::pow2(tile_size) * math::pow2(filter_size)); i++) {
	//	print(&jittered_samples[i]);
	//}

	//u32 n = 2;
	//for (u32 i = 0; i < n; i++) {
	//	for (u32 j = 0; j < n; j++) {
	//		print("first square index = ", ((i * n) + j));
	//		for (u32 x = 0; x < n; x++) {
	//			for (u32 y = 0; y < n; y++) {
	//				print("second square index = ", ((x * n) + y));
	//				print("index =", (((i * n) + j) * (n * n)) + ((x * n) + y));
	//			}
	//		}
	//	}
	//}
}

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
	//print("--------------Start of a test function---------------");
	//make_jittering_sampling_filters(4, 4);
	//print("--------------End of a test function---------------");
}
