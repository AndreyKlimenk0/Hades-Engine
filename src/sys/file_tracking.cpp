#include <assert.h>

#include "file_tracking.h"
#include "../libs/os/file.h"
#include "../libs/os/path.h"

static inline void join_pahts(const String &first_path, const String &second_path, String &result)
{
	assert(result.is_empty());

	result = first_path + "\\" + second_path;
}

static inline char *to_string(wchar_t *unicode_string, u32 size_in_bytes)
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

static inline File_Tracking_System::Directory *make_directory(u32 file_count, const char *full_path_to_dir, HANDLE backup_file, Callback *callback)
{
	assert(callback);
	assert(file_count != 0);

	if (file_count == 1) {
		// ReadDirectoryChangesW works only with buffer more than size of FILE_NOTIFY_INFORMATION 
		// because of it I should add extra memory space.
		file_count += 1;
	}
	File_Tracking_System::Directory *directory = new File_Tracking_System::Directory();
	directory->callback = callback;
	directory->file_count = file_count;
	directory->path_to_dir = full_path_to_dir;
	directory->backup_file_handler = backup_file;
	directory->file_notify_array.reserve(file_count);
	ZeroMemory(&directory->overlapped, sizeof(OVERLAPPED));
	directory->overlapped.hEvent = CreateEvent(NULL, true, false, NULL);

	return directory;
}

void File_Tracking_System::Directory::free()
{
	DELETE_PTR(callback);
	CloseHandle(overlapped.hEvent);
	CloseHandle(backup_file_handler);
	file_notify_array.clear();
	path_to_dir.free();
	file_count = 0;
	callback = NULL;
	backup_file_handler = INVALID_HANDLE_VALUE;
}

File_Tracking_System::File_Tracking_System()
{
}

File_Tracking_System::~File_Tracking_System()
{
	shutdown();
}

void File_Tracking_System::add_directory(const char *engine_relative_dir_path, Callback *callback)
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
				Directory *directory = make_directory(file_count, full_path, backup_file, callback);
				DWORD size_buffer = 0;
				if (ReadDirectoryChangesW(directory->backup_file_handler, (void *)directory->file_notify_array.items, directory->file_notify_array.get_size(), false, FILE_NOTIFY_CHANGE_LAST_WRITE, &size_buffer, &directory->overlapped, NULL)) {
					directories.push(directory);
					print("File_Tracking_System::add_directory: The directory {} was successfully appended for tracking changes in files.", &full_path);
				} else {
					directory->free();
					DELETE_PTR(directory);
					error_message = get_error_message_from_error_code(GetLastError());
					print("File_Tracking_System::add_directory: Async call of ReadDirectoryChangesW failed for directory {}. {}", full_path, error_message);
				}
			} else {
				error_message = get_error_message_from_error_code(GetLastError());
				print("File_Tracking_System::add_directory: Failed to create a backup file for tracking directory changes. {}", error_message);
			}
		} else {
			print("File_Tracking_System::add_directory: The directory {} doesn't contain files. There is no sense to append it.", full_path);
		}
	} else {
		print("File_Tracking_System::add_directory: The directory {} doesn't exist. it can't be appended for tracking changes in files.", full_path);
	}
	free_string(error_message);
}

void File_Tracking_System::update()
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
							print("File_Tracking_System::update: {} was modified.", file_name);
							directory->callback->call((void *)file_name);
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
						loop_print("File_Tracking_System::update: A new async call for reading directory changes was not queued. {}", error_message);
						free_string(error_message);
					}
				}
			} else {
				assert(ERROR_IO_INCOMPLETE == GetLastError());
			}
		}
	}
}

void File_Tracking_System::shutdown()
{
	Directory *directory = NULL;
	For(directories, directory) {
		directory->free();
		DELETE_PTR(directory);
	}
}
