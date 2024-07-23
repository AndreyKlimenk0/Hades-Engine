#ifndef VAR_SERVICE_H
#define VAR_SERVICE_H

struct Variable_Service {
    void load(const char *file_name);
    void parse(const char *data);
};

#endif