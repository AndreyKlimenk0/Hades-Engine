#ifndef UI_STORAGE
#define UI_STORAGE

#include "render_api\render.h"
#include "../libs/number_types.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"

struct UI_Draw_Command {
    u32 vertex_buffer_offset;
    u32 index_buffer_offset;
    u32 index_count;
    Texture *texture = NULL;
    Rect_u32 clip_rect;
};

struct UI_Storage {
    Buffer *vertex_buffer = NULL;
    Buffer *index_buffer = NULL;
    Buffer *ui_data_buffer = NULL;
    Texture *font_texture = NULL;
    Render_Device *render_device = NULL;

    Array<UI_Draw_Command> draw_commands;

    void init(Render_Device *_render_device);
    void upload_ui();
    void upload_font();

    void prepare_for_rendering();
};
#endif