#ifndef GENERAL_H
#define GENERAL_H

#include <stdlib.h>


#define DELETE(x) if (x) delete x;
#define DELETE_PTR(x) if (x) delete x, x = NULL;
#define DELETE_ARRAY(x) if (x) delete[] x;

#endif