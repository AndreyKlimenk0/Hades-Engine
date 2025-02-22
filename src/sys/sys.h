#ifndef SYS_H
#define SYS_H

#include <windows.h>

#include "../libs/str.h"
#include "../win32/win_console.h"
#include "../libs/number_types.h"

#define ASSERT_MSG(expr, str_msg) (assert((expr) && (str_msg)))

void report_info(const char *info_message);
void report_error(const char *error_message);
char *get_error_message_from_error_code(DWORD hr);

inline void simple_print(const char *string)
{
	append_text_to_console_buffer(string, true);
}

template <typename... Args>
void print(Args... args)
{
	char *formatted_string = format(args...);
	append_text_to_console_buffer(formatted_string, true);
	if (formatted_string) { delete formatted_string; }
}

template <typename... Args>
void print_same_line(Args... args)
{
	char *formatted_string = format(args...);
	append_text_to_console_buffer(formatted_string, false);
	if (formatted_string) { delete formatted_string; }
}

bool is_string_unique(const char *string);

template <typename... Args>
void loop_print(Args... args)
{
	char *formatted_string = format(args...);
	if (is_string_unique(formatted_string)) {
		append_text_to_console_buffer(formatted_string, true);
	}
	if (formatted_string) { delete formatted_string; }
}

template <typename... Args>
void info(Args... args)
{
	char *formatted_string = format(args...);
	report_info(formatted_string);
	if (formatted_string) { delete formatted_string; }
}

template <typename... Args>
void error(Args... args)
{
	char *formatted_string = format(args...);
	report_error(formatted_string);
	if (formatted_string) { delete formatted_string; }
}

#endif
