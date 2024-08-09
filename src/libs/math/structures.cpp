#include "structures.h"

Ray::Ray()
{
}

Ray::Ray(const Vector3 &_origin, const Vector3 &_direction)
{
	origin = _origin;
	len = length(_direction);
	direction = normalize(_direction);
}
