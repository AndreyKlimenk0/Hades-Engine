#ifndef VAR_SERVICE_H
#define VAR_SERVICE_H

#include "../libs/str.h"
#include "../libs/structures/array.h"

#define ATTACH(service, variable) (service->attach(#variable, &variable))
#define ATTACH2(service, variable) (service.attach(#variable, &variable))

enum Value_Type {
    BOOLEAN_VALUE = 0,
    INTEGER_VALUE = 1,
    FLOAT_VALUE   = 2,
    STRING_VALUE  = 3
};

struct Rvalue {
    Rvalue();
    ~Rvalue();

    Value_Type type;
    union {
        bool boolean;
        s32 integer;
        float real;
        const char *string;
    };
};

struct Variable_Binding {
    String name;
    Rvalue rvalue;
};

struct Variable_Service {
    Variable_Service();
    ~Variable_Service();

    String namespace_name;
    Array<Variable_Binding *> bindings;
    Array<Variable_Service *> namespaces;

    void load(const char *file_name);
    void parse(const char *data);
    void shutdown();

    void attach(const char *variable_name, bool *value);
    void attach(const char *variable_name, int *value);
    void attach(const char *variable_name, float *value);
    void attach(const char *variable_name, String *string);

    void bind_boolean(const char *variable_name, const char *string);
    void bind_integer(const char *variable_name, const char *string);
    void bind_float(const char *variable_name, const char *string);
    void bind_string(const char *variable_name, const char *string);

    Variable_Binding *find_binding(const char *name);
    Variable_Service *find_namespace(const char *name);
};

#endif