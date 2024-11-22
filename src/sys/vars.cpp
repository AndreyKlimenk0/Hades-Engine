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
    if (type == STRING_VALUE) {
        free_string(string);
    }
}

void Variable_Service::attach(const char *variable_name, bool *value)
{
    assert(variable_name);
    assert(value);

    Variable_Binding *binding = find_binding(variable_name);
    if (binding) {
        if (binding->rvalue.type == BOOLEAN_VALUE) {
            *value = binding->rvalue.boolean;
        } else {
            print("[variable service] Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("[variable service] Error: A value can not be attached to '{}' because binding '{} = bool' does not exist.", variable_name, variable_name);
    }
}

void Variable_Service::attach(const char *variable_name, int *value)
{
    assert(variable_name);
    assert(value);

    Variable_Binding *binding = find_binding(variable_name);
    if (binding) {
        if (binding->rvalue.type == INTEGER_VALUE) {
            *value = binding->rvalue.integer;
        } else {
            print("[variable service] Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("[variable service] Error: A value can not be attached to '{}' because binding '{} = integer' does not exist.", variable_name, variable_name);
    }
}

void Variable_Service::attach(const char *variable_name, float *value)
{
    assert(variable_name);
    assert(value);

    Variable_Binding *binding = find_binding(variable_name);
    if (binding) {
        if (binding->rvalue.type == FLOAT_VALUE) {
            *value = binding->rvalue.real;
        } else {
            print("[variable service] Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("[variable service] Error: A value can not be attached to '{}' because binding '{} = float' does not exist.", variable_name, variable_name);
    }
}

void Variable_Service::attach(const char *variable_name, String *string)
{
    assert(variable_name);
    assert(string);

    Variable_Binding *binding = find_binding(variable_name);
    if (binding) {
        if (binding->rvalue.type == STRING_VALUE) {
            *string = binding->rvalue.string;
        } else {
            print("[variable service] Error: A value can not be attached to '{}'. A passed variable type doesn't not match with a binding value type.", variable_name);
        }
    } else {
        print("[variable service] Error: A value can not be attached to '{}' because binding '{} = string' does not exist.", variable_name, variable_name);
    }
}

void Variable_Service::bind_boolean(const char *variable_name, const char *string)
{
    assert(variable_name);
    assert(string);

    Variable_Binding *binding = new Variable_Binding();
    binding->name = variable_name;
    binding->rvalue.type = BOOLEAN_VALUE;
    binding->rvalue.boolean = !strcmp(string, "true") ? true : false;
    bindings.push(binding);
}

void Variable_Service::bind_integer(const char *variable_name, const char *string)
{
    assert(variable_name);
    assert(string);

    Variable_Binding *binding = new Variable_Binding();
    binding->name = variable_name;
    binding->rvalue.type = INTEGER_VALUE;
    binding->rvalue.integer = atoi(string);
    bindings.push(binding);
}

void Variable_Service::bind_float(const char *variable_name, const char *string)
{
    assert(variable_name);
    assert(string);

    Variable_Binding *binding = new Variable_Binding();
    binding->name = variable_name;
    binding->rvalue.type = FLOAT_VALUE;
    binding->rvalue.real = (float)atof(string);
    bindings.push(binding);
}

void Variable_Service::bind_string(const char *variable_name, const char *string)
{
    assert(variable_name);
    assert(string);
    assert(strlen(string) > 0);

    Variable_Binding *binding = new Variable_Binding();
    binding->name = variable_name;
    binding->rvalue.type = STRING_VALUE;
    binding->rvalue.string = copy_string(string, 1, (u32)strlen(string) - 1);
    bindings.push(binding);
}

Variable_Binding *Variable_Service::find_binding(const char *name)
{
    assert(name);

    for (u32 i = 0; i < bindings.count; i++) {
        if (bindings[i]->name == name) {
            return bindings[i];
        }
    }
    return NULL;
}

Variable_Service *Variable_Service::find_namespace(const char *name)
{
    assert(name);

    for (u32 i = 0; i < namespaces.count; i++) {
        if (namespaces[i]->namespace_name == name) {
            return namespaces[i];
        }
    }
    return this;
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

    print("[var service] Info: Loading variables from '{}'.", file_name);

    String file_extension;
    if (!(extract_file_extension(file_name, file_extension) || (file_extension == VARIABLE_FILE_EXTENSION))) {
        print("[variable service] Error: The variable file name '{}' is wrong. A variable file name must have format 'some_file_name.variables'", file_name);
        return;
    }

    String full_path_to_variable_file = join_paths(get_full_path_to_data_directory(), file_name);
    if (!file_exists(full_path_to_variable_file)) {
        print("[variable service] Error: File {} doesn't exist in the data directory.", file_name);
        return;
    }

    char *data = read_entire_file(full_path_to_variable_file, "rb");
    if (data) {
        parse(data);
        free_string(data);
    }
}

typedef void (Variable_Service:: *Bind_Method)(const char *, const char *);
typedef bool (*Match_Function)(const char *string);

static const Pair<Match_Function, Bind_Method> binding_table[] = {
    { &string_contain_boolean, &Variable_Service::bind_boolean },
    { &string_contain_integer, &Variable_Service::bind_integer },
    { &string_contain_real,    &Variable_Service::bind_float },
    { &string_contain_string,  &Variable_Service::bind_string },
};

void Variable_Service::parse(const char *data)
{
    assert(data);
    namespace_name = "base_namespace";

    u32 line_number = 0;
    char *text = (char *)data;
    Variable_Service *service = this;

    while (true) {
        char *line = get_next_line(&text);
        if (!line) {
            break;
        } else if (compare_start(line, "#") || string_empty(line)) {
            continue;
        } else if (compare_start(line, ":/")) {
            String namespace_name;
            if (parse_variable_directory_name(line, namespace_name)) {
                service = new Variable_Service();
                service->namespace_name = namespace_name;
                namespaces.push(service);
            } else {
                report_error(line_number, line, "Not valid a directory name. The directory name must contain only [a-zA-Z_] symbols.");
            }
        } else {
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
                    CALL_METHOD(*service, method, name, value);
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
    Variable_Service *variable_namespace = NULL;
    For(namespaces, variable_namespace) {
        variable_namespace->shutdown();
    }
    For(namespaces, variable_namespace) {
        DELETE_PTR(variable_namespace);
    }
    Variable_Binding *binding = NULL;
    For(bindings, binding) {
        DELETE_PTR(binding);
    }
    namespaces.clear();
    bindings.clear();
}
