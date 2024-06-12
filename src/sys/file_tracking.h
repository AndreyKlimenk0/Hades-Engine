#ifndef FILE_TRACKING_SYSTEM
#define FILE_TRACKING_SYSTEM

#include <windows.h>

#include "../sys/utils.h"
#include "../libs/str.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"

struct File_Tracking_System {
	File_Tracking_System();
	~File_Tracking_System();

	struct Directory {
		u32 file_count = 0;
		Callback *callback = NULL;
		HANDLE backup_file_handler = INVALID_HANDLE_VALUE;
		s64 modifying_time = 0;
		String path_to_dir;
		OVERLAPPED overlapped;
		Array<FILE_NOTIFY_INFORMATION> file_notify_array;

		void free();
	};
	Array<Directory *> directories;

	void add_directory(const char *engine_relative_dir_path, Callback *callback);
	void update();
	void shutdown();
};
#endif
