#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "sys.h"
#include "vars.h"
#include "utils.h"
#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/structures/array.h"

static const String VARIABLE_FILE_EXTENSION = "variables";

static char *eat_spaces(char *string)
{
    if (string_null_or_empty(string)) {
        return NULL;
    }
    char *s = string;
    while (isspace(*s)) {
        s++;
    }
    return s;
}

static char *get_next_world(char **line)
{
    if (string_null_or_empty(*line)) {
        return NULL;
    }
    *line = eat_spaces(*line);

    char *start = *line;
    char *temp = *line;
    bool result = false;
    while (*temp && (*temp != ' ') && (*temp != '\r') && (*temp != '\n')) {
        temp++;
    }
    if (*temp) {
        char *end = temp + 1;
        if (*temp == '\r') {
            if (*end == '\n') {
                end++;
            }
        }
        result = true;
        *temp = '\0';
        *line = end;
    }
    return start;
}

static bool compare_start(const char *string, const char *substring)
{
    assert(string);
    assert(substring);

    u32 string_len = (u32)strlen(string);
    u32 substring_len = (u32)strlen(substring);

    if (substring_len > string_len) {
        return false;
    }
    for (u32 i = 0; i < substring_len; i++) {
        if (string[i] != substring[i]) {
            return false;
        }
    }
    return true;
}

static bool string_contain_boolean(const char *string)
{
    if (string_null_or_empty(string)) {
        return false;
    }
    if ((strcmp(string, "true") && strcmp(string, "false"))) {
        return false;
    }
    return true;
}

static bool string_contain_integer(const char *string)
{
    assert(string);
    u32 len = (u32)strlen(string);
    for (u32 i = 0; i < len; i++) {
        if (!isdigit(string[i])) {
            return false;
        }
    }
    return true;
}

static bool string_contain_real(const char *string)
{
    assert(string);
    u32 len = (u32)strlen(string);
    if (len < 3) {
        return false;
    }
    if ((string[0] == '.') || (string[len - 1] == '.')) {
        return false;
    }
    for (u32 i = 0; i < len; i++) {
        if (!isdigit(string[i]) && (string[i] != '.')) {
            return false;
        }
    }
    return true;
}

static bool string_contain_string(const char *string)
{
    assert(string);
    u32 len = (u32)strlen(string);
    if (len < 3) {
        return false;
    }
    if ((string[0] != '"') || (string[len - 1] != '"')) {
        return false;
    }
    for (u32 i = 1; i < (len - 1); i++) {
        if ((string[i] == '"') || (string[i] == '\'')) {
            return false;
        }
    }
    return true;
}

static bool validate_world(const char *word)
{
    if (string_null_or_empty(word)) {
        return false;
    }
    u32 len = (u32)strlen(word);
    for (u32 i = 0; i < len; i++) {
        if (!isalpha(word[i]) && (word[i] != '_')) {
            return false;
        }
    }
    return true;
}

static void report_error(u32 line_number, const char *line, const char *error_message)
{
    print("Error in line {}: {}", line_number, error_message);
    print("->", line);
}

static bool parse_variable_directory_name(const char *line, String &directory_name)
{
    u32 line_len = (u32)strlen(line);
    for (u32 i = 2; i < line_len; i++) {
        if (!isalpha(line[i]) && (line[i] != '_')) {
            String temp;
            temp.copy(line, 2, line_len);
            return false;
        }
    }
    directory_name.copy(line, 2, line_len);
    return true;
}

static bool string_empty(char *string)
{
    assert(string);
    return string[0] == '\0';
}

static char *copy_string(const char *string, u32 start, u32 end)
{
    assert(end > start);

    char *new_string = NULL;
    u32 chunk_len = end - start;
    if ((chunk_len > 0) && (chunk_len <= strlen(string))) {
        const char *ptr = string;
        ptr += start;

        new_string = new char[chunk_len + 1];
        memcpy(new_string, ptr, sizeof(char) * chunk_len);
        new_string[chunk_len] = '\0';
    }
    return new_string;
}

Rvalue::Rvalue()
{
    memset(this, 0, sizeof(Rvalue));
}

Rvalue::~Rvalue()
{
    free();
}

Rvalue::Rvalue(const Rvalue &other)
{
    *this = other;
}

Rvalue &Rvalue::operator=(const Rvalue &other)
{
    if (this != &other) {
        if (type == STRING_VALUE) {
            DELETE_ARRAY(string);
        }
        type = other.type;
        if (type == STRING_VALUE) {
            string = copy_string(other.string, 0, (u32)strlen(other.string));
        } else {
            integer = other.integer;
        }
    }
    return *this;
}

void Rvalue::free()
{
    if (type == STRING_VALUE) {
        DELETE_ARRAY(string);
    }
}

Variable_Directory::Variable_Directory(const char *name) : name(name)
{
}

Variable_Directory::~Variable_Directory()
{
    variables.clear();
}

void Variable_Directory::attach(const char *variable_name, bool *value)
{
    assert(variable_name);
    assert(value);

    Rvalue rvalue;
    if (variables.get(variable_name, rvalue)) {
        if (rvalue.type == BOOLEAN_VALUE) {
            *value = rvalue.boolean;
        } else {
            print("Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("Error: A value can not be attached to '{}' because binding '{} = bool' does not exist.", variable_name, variable_name);
    }
}

void Variable_Directory::attach(const char *variable_name, int *value)
{
    assert(variable_name);
    assert(value);

    Rvalue rvalue;
    if (variables.get(variable_name, rvalue)) {
        if (rvalue.type == INTEGER_VALUE) {
            *value = rvalue.integer;
        } else {
            print("Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("Error: A value can not be attached to '{}' because binding '{} = integer' does not exist.", variable_name, variable_name);
    }
}

void Variable_Directory::attach(const char *variable_name, float *value)
{
    assert(variable_name);
    assert(value);

    Rvalue rvalue;
    if (variables.get(variable_name, rvalue)) {
        if (rvalue.type == FLOAT_VALUE) {
            *value = rvalue.real;
        } else {
            print("Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("Error: A value can not be attached to '{}' because binding '{} = float' does not exist.", variable_name, variable_name);
    }
}

void Variable_Directory::attach(const char *variable_name, String &string)
{
    assert(variable_name);

    Rvalue rvalue;
    if (variables.get(variable_name, rvalue)) {
        if (rvalue.type == STRING_VALUE) {
            string = rvalue.string;
        } else {
            print("Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("Error: A value can not be attached to '{}' because binding '{} = string' does not exist.", variable_name, variable_name);
    }
}

void Variable_Directory::bind_boolean(const char *variable_name, const char *string)
{
    Rvalue value;
    value.type = BOOLEAN_VALUE;
    value.boolean = !strcmp(string, "true") ? true : false;
    variables.set(variable_name, value);
}

void Variable_Directory::bind_integer(const char *variable_name, const char *string)
{
    Rvalue value;
    value.type = INTEGER_VALUE;
    value.real = (float)atoi(string);
    variables.set(variable_name, value);
}

void Variable_Directory::bind_float(const char *variable_name, const char *string)
{
    Rvalue value;
    value.type = FLOAT_VALUE;
    value.real = (float)atof(string);
    variables.set(variable_name, value);
}

void Variable_Directory::bind_string(const char *variable_name, const char *string)
{
    Rvalue value;
    value.type = STRING_VALUE;
    value.string = copy_string(string, 1, (u32)strlen(string) - 1);
    variables.set(variable_name, value);
}

Variable_Service::Variable_Service()
{
}

Variable_Service::~Variable_Service()
{
    shutdown();
}

void Variable_Service::load(const char *file_name)
{
    assert(file_name);

    print("Start to load variables from '{}'.", file_name);

    String file_extension;
    if (!(extract_file_extension(file_name, file_extension) || (file_extension == VARIABLE_FILE_EXTENSION))) {
        print("Variable_Service::load: The variable file name '{}' is wrong. A variable file name must have format 'some_file_name.variables'", file_name);
        return;
    }

    String full_path_to_variable_file = join_paths(get_full_path_to_data_directory(), file_name);
    if (!file_exists(full_path_to_variable_file)) {
        print("Variable_Service::load: File {} doesn't exist in the data directory.", file_name);
        return;
    }

    char *data = read_entire_file(full_path_to_variable_file, "rb");
    if (data) {
        parse(data);
        free_string(data);
    }
}

typedef void (Variable_Directory::*Bind_Method)(const char *, const char *);
typedef bool (*Match_Function)(const char *string);

static const Pair<Match_Function, Bind_Method> binding_table[] = {
    { &string_contain_boolean, &Variable_Directory::bind_boolean },
    { &string_contain_integer, &Variable_Directory::bind_integer },
    { &string_contain_real,    &Variable_Directory::bind_float },
    { &string_contain_string,  &Variable_Directory::bind_string },
};

void Variable_Service::parse(const char *data)
{
    assert(data);
    u32 line_number = 0;
    char *text = (char *)data;
    Variable_Directory *variable_directory = NULL;

    while (true) {
        char *line = get_next_line(&text);
        if (!line) {
            break;
        } else if (compare_start(line, "//") || string_empty(line)) {
            continue;
        } else if (compare_start(line, ":/")) {
            String directory_name;
            if (parse_variable_directory_name(line, directory_name)) {
                variable_directory = new Variable_Directory(directory_name);
                variable_directories.set(directory_name, variable_directory);
            } else {
                report_error(line_number, line, "Not valid a directory name. The directory name must contain only [a-zA-Z_] symbols.");
            }
        } else {
            if (!variable_directory) {
                report_error(line_number, line, "Can not bind a variable. A variable directory was not specified above.");
                continue;
            }
            char *origin_line = line;
            char *name = get_next_world(&line);
            char *value = get_next_world(&line);
            if (!(name && value && validate_world(name))) {
                report_error(line_number, origin_line, "A line is an unparseable.");
                continue;
            }
            bool result = false;
            for (u32 i = 0; i < ARRAY_SIZE(binding_table); i++) {
                if (binding_table[i].first(value)) {
                    Bind_Method method = binding_table[i].second;
                    CALL_METHOD(*variable_directory, method, name, value);
                    result = true;
                    break;
                }
            }
            if (!result) {
                report_error(line_number, origin_line, "Can not process value type.");
            }
        }
        line_number++;
    }
}

void Variable_Service::shutdown()
{
    for (u32 i = 0; i < variable_directories.count; i++) {
        DELETE_PTR(variable_directories.get_value(i));
    }
    variable_directories.clear();
}

Variable_Directory *Variable_Service::get_variable_directory(const char *name)
{
    Variable_Directory *temp = NULL;
    variable_directories.get(name, &temp);
    return temp;
}