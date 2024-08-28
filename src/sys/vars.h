#ifndef VAR_SERVICE_H
#define VAR_SERVICE_H

#include "../libs/str.h"
#include "../libs/structures/hash_table.h"

enum Value_Type {
    BOOLEAN_VALUE = 0,
    INTEGER_VALUE = 1,
    FLOAT_VALUE   = 2,
    STRING_VALUE  = 3
};

struct Rvalue {
    Rvalue();
    ~Rvalue();
    Rvalue(const Rvalue &other);

    Value_Type type;
    union {
        bool boolean;
        s32 integer;
        float real;
        const char *string;
    };
    Rvalue &operator=(const Rvalue &other);

    void free();
};

struct Variable_Directory {
    Variable_Directory(const char *name);
    ~Variable_Directory();

    String name;
    Hash_Table<String, Rvalue> variables;

    void attach(const char *variable_name, bool *value);
    void attach(const char *variable_name, int *value);
    void attach(const char *variable_name, float *value);
    void attach(const char *variable_name, String &string);

    void bind_boolean(const char *variable_name, const char *string);
    void bind_integer(const char *variable_name, const char *string);
    void bind_float(const char *variable_name, const char *string);
    void bind_string(const char *variable_name, const char *string);
};

struct Variable_Service {
    Variable_Service();
    ~Variable_Service();

    Hash_Table<String, Variable_Directory *> variable_directories;

    void load(const char *file_name);
    void parse(const char *data);
    void shutdown();

    Variable_Directory *get_variable_directory(const char *name);
};

#endif