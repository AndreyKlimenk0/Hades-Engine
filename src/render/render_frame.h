#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <DirectXMath.h>

#include "base.h"
#include "mesh.h"

#include "../libs/ds/array.h"
#include "../libs/os/camera.h"

#include "../game/entity.h"


void draw_entities(Entity_Manager *entity_manager, Matrix4 &view, Free_Camera *camera);

#endif