#include "test.h"
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

#include <windows.h>
#include <stdlib.h>
#include "../sys/sys_local.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"
#include "win_types.h"

static inline void join_pahts(const String &first_path, const String &second_path, String &result) 
{
	assert(result.is_empty());

	result = first_path + "\\" + second_path;
}

char *to_string(wchar_t *unicode_string, u32 size_in_bytes)
{
	assert(size_in_bytes > 0);
	assert((size_in_bytes % sizeof(u16)) == 0);

	u32 unicode_string_len = (size_in_bytes / sizeof(u16));
	u32 new_string_len = unicode_string_len + 1;
	char *new_string = new char[new_string_len];

	size_t new_string_size_in_bytes = 0;
	errno_t result = wcstombs_s(&new_string_size_in_bytes, new_string, new_string_len, unicode_string, unicode_string_len);
	assert(result == 0);
	assert((new_string_size_in_bytes / sizeof(u8)) == new_string_len);

	return new_string;
}

struct Hot_Reload_System {
	Hot_Reload_System() {}
	~Hot_Reload_System() { shutdown(); }

	struct Directory {
		u32 file_count = 0;
		Callback *callback = NULL;
		HANDLE backup_file_handler = INVALID_HANDLE_VALUE;
		OVERLAPPED overlapped;
		Array<FILE_NOTIFY_INFORMATION> file_notify_array;

		void free();
	};
	Array<Directory *> directories;

	void add_directory(const char *engine_relative_dir_path, Callback *callback);
	void update();
	void shutdown();
};

void Hot_Reload_System::Directory::free()
{
	DELETE_PTR(callback);
	CloseHandle(overlapped.hEvent);
	CloseHandle(backup_file_handler);
	file_notify_array.clear();
	file_count = 0;
	callback = NULL;
	backup_file_handler = INVALID_HANDLE_VALUE;
}

inline Hot_Reload_System::Directory *make_directory(u32 file_count, HANDLE backup_file, Callback *callback)
{
	assert(callback);

	Hot_Reload_System::Directory *directory = new Hot_Reload_System::Directory();
	directory->callback = callback;
	directory->file_count = file_count;
	directory->backup_file_handler = backup_file;
	directory->file_notify_array.reserve(file_count);
	ZeroMemory(&directory->overlapped, sizeof(OVERLAPPED));
	directory->overlapped.hEvent = CreateEvent(NULL, true, false, NULL);

	return directory;
}

void Hot_Reload_System::add_directory(const char *engine_relative_dir_path, Callback *callback)
{
	assert(callback);
	assert(engine_relative_dir_path);

	String full_path;
	join_pahts(get_base_path(), engine_relative_dir_path, full_path);

	char *error_message = NULL;
	if (directory_exists(full_path)) {
		u32 file_count = get_file_count_in_dir(full_path);
		if (file_count > 0) {
			HANDLE backup_file = CreateFile(full_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
			if (backup_file != INVALID_HANDLE_VALUE) {
				Directory *directory = make_directory(file_count, backup_file, callback);
				DWORD size_buffer = 0;
				if (ReadDirectoryChangesW(directory->backup_file_handler, (void *)directory->file_notify_array.items, directory->file_notify_array.get_size(), false, FILE_NOTIFY_CHANGE_LAST_WRITE, &size_buffer, &directory->overlapped, NULL)) {
					directories.push(directory);
					print("Hot_Reload_System::add_directory: The directory {} was successfully appended for tracking changes in files.", &full_path);
				} else {
					directory->free();
					DELETE_PTR(directory);
					error_message = get_error_message_from_error_code(GetLastError());
					print("Hot_Reload_System::add_directory: Async call of ReadDirectoryChangesW failed for directory {}. {}.", full_path, error_message);
				}
			} else {
				error_message = get_error_message_from_error_code(GetLastError());
				print("Hot_Reload_System::add_directory: Failed to create a backup file for tracking directory changes. {}.", error_message);
			}
		} else {
			print("Hot_Reload_System::add_directory: The directory {} doesn't contain files. There is no sense to append it.", full_path);
		}
	} else {
		print("Hot_Reload_System::add_directory: The directory {} doesn't exist. it can't be appended for tracking changes in files.", full_path);
	}
	free_string(error_message);
}

void Hot_Reload_System::update()
{
	Directory *directory = NULL;
	For(directories, directory) {
		if (HasOverlappedIoCompleted(&directory->overlapped)) {
			DWORD received_bytes = 0;
			if (GetOverlappedResult(directory->backup_file_handler, &directory->overlapped, &received_bytes, false)) {
				ResetEvent(directory->overlapped.hEvent);
				if (received_bytes > 0) {
					FILE_NOTIFY_INFORMATION *file_notify_info = directory->file_notify_array.items;
					for (u32 i = 0; i < directory->file_notify_array.count; i++) {
						if (file_notify_info->Action == FILE_ACTION_MODIFIED) {
							char *file_name = to_string(file_notify_info->FileName, file_notify_info->FileNameLength);
							directory->callback->call((void *)file_name);
							print("Hot_Reload_System::update: {} was modified.", file_name);
							free_string(file_name);
						}
						if (file_notify_info->NextEntryOffset == 0) {
							break;
						}
						file_notify_info += file_notify_info->NextEntryOffset;
					}
				}
				if (!ReadDirectoryChangesW(directory->backup_file_handler, (void *)directory->file_notify_array.items, directory->file_notify_array.get_size(), false, FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, &directory->overlapped, NULL)) {
					DWORD error_code = GetLastError();
					if (error_code != ERROR_IO_PENDING) {
						char *error_message = get_error_message_from_error_code(error_code);
						loop_print("Hot_Reload_System::update: A new async call for reading directory changes was not queued. {}", error_message);
						free_string(error_message);
					}
				}
			} else {
				assert(ERROR_IO_INCOMPLETE == GetLastError());
			}
		}
	}
}

void Hot_Reload_System::shutdown()
{
	Directory *directory = NULL;
	For(directories, directory) {
		directory->free();
		DELETE_PTR(directory);
	}
}


void temp_routine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
	print("temp routine.");
}

void init_hot_reloading();
void update();

OVERLAPPED overlapped_info;
FILE_NOTIFY_INFORMATION *file_notify_information = NULL;
HANDLE directory_notify_backup = INVALID_HANDLE_VALUE;
const u32 FILE_NUMBER = 13;
const u32 TEMP_SIZE = sizeof(FILE_NOTIFY_INFORMATION) * FILE_NUMBER;

void add_hot_reloading_directory(const char *engine_relative_dir_path) 
{
	assert(engine_relative_dir_path);

	String full_path;
	join_pahts(get_base_path(), engine_relative_dir_path, full_path);

	file_notify_information = new FILE_NOTIFY_INFORMATION[FILE_NUMBER];

	OVERLAPPED overlapped_info;
	ZeroMemory(&overlapped_info, sizeof(OVERLAPPED));
	overlapped_info.hEvent = CreateEvent(NULL, true, false, NULL);
	if ((overlapped_info.hEvent == INVALID_HANDLE_VALUE) || (overlapped_info.hEvent == NULL)) {
		return;
	}

	if (directory_exists(full_path)) {
		directory_notify_backup = CreateFile(full_path.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
		if (directory_notify_backup != INVALID_HANDLE_VALUE) {
			DWORD size_buffer = sizeof(FILE_NOTIFY_INFORMATION);
			if (ReadDirectoryChangesW(directory_notify_backup, (void *)file_notify_information, TEMP_SIZE, false, FILE_NOTIFY_CHANGE_LAST_WRITE, &size_buffer, &overlapped_info, NULL)) {
				print("add_hot_reloading_directory: hot reloading OK");
				DWORD ssize;
				if (GetOverlappedResult(directory_notify_backup, &overlapped_info, &ssize, false)) {
					int xx = 0;
				} else {
					DWORD e = GetLastError();
					if (e == ERROR_IO_PENDING) {
						int xx = 0;
					} else if (e == ERROR_IO_INCOMPLETE) {
						int ooo = 123124;
					}
				}
			} else {
				print("add_hot_reloading_directory: Failed to read directory changes.");
			}
		} else {
			print("add_hot_reloading_directory: Faield to get access to a backup file for hot reloading directory {}.", engine_relative_dir_path);
		}
	} else {
		print("add_hot_reloading_directory: The directory {} can't be hot reloading because it does't exist.", engine_relative_dir_path);
	}

}

static Hot_Reload_System hot_reload_system;

struct Test_Struct {
	void reload(void *args)
	{
		char *file_name = (char *)args;
		print("reload", file_name);
	}
};
static Test_Struct test_struct;

void test()
{
	//add_hot_reloading_directory("hlsl");

	Callback *callback = make_member_callback<Test_Struct>(&test_struct, &Test_Struct::reload);

	hot_reload_system.add_directory("hlsl", callback);
	hot_reload_system.add_directory("temp_dir", callback);
}


void update_test()
{
	hot_reload_system.update();

	//static bool io_incomplete = false;
	//DWORD size_buffer = 0;
	//if (!io_incomplete) {
	//	if (ReadDirectoryChangesW(directory_notify_backup, (void *)file_notify_information, TEMP_SIZE, false, FILE_NOTIFY_CHANGE_LAST_WRITE, &size_buffer, &overlapped_info, NULL)) {
	//		print("add_hot_reloading_directory: hot reloading OK");
	//	}
	//}
	//DWORD ssize = 0;
	//if (HasOverlappedIoCompleted(&overlapped_info)) {
	//	int y = 0;
	//}
	//if (GetOverlappedResult(directory_notify_backup, &overlapped_info, &ssize, false)) {
	//	int i = 0;
	//	if (ssize > 0) {
	//		int v = 0;
	//		io_incomplete = false;
	//		//file_notify_information[0].FileName;
	//		//DWORD result = file_notify_information[0].FileNameLength / sizeof(s16);
	//		//u32 ss = sizeof(s16);
	//		//ZeroMemory(file_notify_information, sizeof(TEMP_SIZE));
	//		//u32 outputSize = (file_notify_information[0].FileNameLength / sizeof(u16)) + 1; //FileName is a unicode string. FileNameLength stores size of a unicode string in bytes without the terminating null character.
	//		//char *string = new char[outputSize];
	//		//size_t returnted_size = 0;
	//		//wcstombs_s(&returnted_size, string, outputSize, file_notify_information[0].FileName, outputSize - 1);
	//		//int vwv = 2;
	//		//char *s = to_string(file_notify_information[0].FileName, file_notify_information[0].FileNameLength);

	//		for (u32 i = 0; i < FILE_NUMBER; i++) {
	//			FILE_NOTIFY_INFORMATION *ptr = file_notify_information;
	//			char *s = to_string(ptr->FileName, ptr->FileNameLength);
	//			print("File {} was changed.", s);
	//			free_string(s);
	//			if (ptr->NextEntryOffset == 0) {
	//				break;
	//			}
	//			ptr += ptr->NextEntryOffset;
	//		}

	//		u32 index = 0;
	//		FILE_NOTIFY_INFORMATION *ptr = file_notify_information;
	//		while (index < FILE_NUMBER) {
	//			file_notify_information[index].NextEntryOffset;
	//			if ((*ptr).NextEntryOffset == 0) {
	//				break;
	//			}
	//			ptr += (*ptr).NextEntryOffset;
	//		}

	//	}
	//} else {
	//	if (GetLastError() == ERROR_IO_PENDING) {
	//		int xx = 0;
	//	} else if (GetLastError() == ERROR_IO_INCOMPLETE) {
	//		static int ttt = 0;
	//		io_incomplete = true;
	//	}
	//}
}

