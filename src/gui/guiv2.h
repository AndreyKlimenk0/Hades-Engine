#ifndef IMGUIV2_H
#define IMGUIV2_H

#include "../libs/number_types.h"
#include "../render/renderer.h"

namespace imgui {
	enum Layout {
		ROW_LAYOUT = 0,
		COLUMN_LAYOUT,
	};

	const u32 ALIGNMENT_TOP    = 0x1;
	const u32 ALIGNMENT_BOTTOM = 0x2;
	const u32 ALIGNMENT_LEFT   = 0x4;
	const u32 ALIGNMENT_RIGHT  = 0x8;
	const u32 ALIGNMENT_HORIZONTAL_CENTER = 0x10;
	const u32 ALIGNMENT_VERTICAL_CENTER   = 0x20;
	const u32 ALIGNMENT_CENTER = 0x40;

	enum Size_Type {
		SIZE_TYPE_FILLED,
		SIZE_TYPE_FIXED,
		SIZE_TYPE_PERCENT,
	};

	//struct Size {
	//	static Size_Dimension filled();
	//	static Size_Dimension fixed(u32 value);
	//	static Size_Dimension percent(u32 value);
	//};

	struct Size_Dimension {
		Size_Type type;
		union {
			u32 fixed;
			float percent;
		};
	};

	inline Size_Dimension filled_size()
	{
		return { .type = SIZE_TYPE_FILLED, .fixed = 0 };
	}

	inline Size_Dimension fixed_size(u32 value)
	{
		return { .type = SIZE_TYPE_FIXED, .fixed = value };
	}

	inline Size_Dimension percent_size(u32 value)
	{
		return { .type = SIZE_TYPE_FIXED, .fixed = value };
	}

	void init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d);
	void shutdown_guiv2();

	void begin_frame();
	void end_frame();

	void begin_ui_element(const char *name);
	void end_ui_element();

	void set_position(s32 x, s32 y);
	void set_size(Size_Dimension horizontal, Size_Dimension vertical);
	void set_layout(Layout layout);
	void set_alignment(u32 alignment_flags);
	void set_background_color(const Color &color);
}
#endif
