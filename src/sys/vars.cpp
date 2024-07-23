#include <assert.h>

#include "sys.h"
#include "vars.h"
#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"

static const String VARIABLE_FILE_EXTENSION = "variables";
                                            
void Variable_Service::load(const char *file_name)
{
    assert(file_name);

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
    
    char *data = read_entire_file(full_path_to_variable_file);
    if (data) {
        parse(data);
    }
}

void Variable_Service::parse(const char *data)
{
    assert(data);
}
