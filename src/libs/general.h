#ifndef GENERAL_H
#define GENERAL_H

#include "math/vector.h"

#define DELETE(x) if (x) delete x;
#define DELETE_PTR(x) if (x) delete x, x = NULL;
#define DELETE_ARRAY(x) if (x) delete[] x;


extern Vector4 White;
extern Vector4 Black;
extern Vector4 Red;
extern Vector4 Green;
extern Vector4 Blue;
extern Vector4 Yellow;
extern Vector4 Cyan;
extern Vector4 Magenta;
extern Vector4 Silver;
extern Vector4 LightSteelBlue;
#endif