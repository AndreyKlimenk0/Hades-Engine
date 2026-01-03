#include "base_structs.h"
#include "../../sys/utils.h"

Clear_Value::Clear_Value() : type(CLEAR_VALUE_UNKNOWN)
{
}

Clear_Value::Clear_Value(Color &_color)
{
    type = CLEAR_VALUE_COLOR;
    color = _color;
}

Clear_Value::Clear_Value(float _depth, u8 _stencil)
{
    type = CLEAR_VALUE_DEPTH_STENCIL;
    depth = _depth;
    stencil = _stencil;
}

Clear_Value::~Clear_Value()
{
}

bool Clear_Value::depth_stencil_set()
{
    return type == CLEAR_VALUE_DEPTH_STENCIL;
}

u64 Buffer_Desc::size()
{
    return count * stride;
}

Viewport::Viewport()
{
}

Viewport::Viewport(const Size_f32 &size)
{
    width = size.width;
    height = size.height;
}

Viewport::~Viewport()
{
}

void Viewport::reset()
{
    memset((void *)this, 0, sizeof(Viewport));
}

Subresource_Footprint::Subresource_Footprint()
{
}

Subresource_Footprint::~Subresource_Footprint()
{
}

Resource_Footprint::Resource_Footprint()
{
}

Resource_Footprint::~Resource_Footprint()
{
}

Blending_Desc::Blending_Desc()
{
    enable = false;
    src = BLEND_ONE;
    dest = BLEND_ZERO;
    blend_op = BLEND_OP_ADD;
    src_alpha = BLEND_ONE;
    dest_alpha = BLEND_ZERO;
    blend_op_alpha = BLEND_OP_ADD;
}

Blending_Desc::~Blending_Desc()
{
}

Rasterization_Desc::Rasterization_Desc()
{
    fill_type = FILL_TYPE_SOLID;
    cull_type = CULL_TYPE_BACK;
}

Rasterization_Desc::~Rasterization_Desc()
{
}

Depth_Stencil_Desc::Depth_Stencil_Desc()
{
    enable_depth_test = true;
    enable_stencil_test = false;
    stencil_read_mask = 0xff;
    stencil_write_mask = 0xff;
    depth_write = DEPTH_WRITE_ALL;
    depth_compare_func = COMPARISON_LESS;
    stencil_compare_func = COMPARISON_ALWAYS;
    stencil_failed = STENCIL_OP_KEEP;
    stencil_passed_depth_failed = STENCIL_OP_KEEP;
    stencil_and_depth_passed = STENCIL_OP_KEEP;
}

void Graphics_Pipeline_Desc::add_render_target(DXGI_FORMAT format)
{
    render_targets_formats.push(format);
}

Depth_Stencil_Desc::~Depth_Stencil_Desc()
{
}