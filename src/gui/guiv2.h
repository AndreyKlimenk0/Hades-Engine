#ifndef IMGUIV2_H
#define IMGUIV2_H

#include "../libs/number_types.h"
#include "../render/renderer.h"

namespace imgui {
	enum Layout {
		ROW_LAYOUT = 0,
		COLUMN_LAYOUT,
	};

	const u32 ALIGNMENT_TOP    = 0x1; // Top to bottom.
	const u32 ALIGNMENT_BOTTOM = 0x2; // Bottom to top.
	const u32 ALIGNMENT_LEFT   = 0x4; // Left to right.
	const u32 ALIGNMENT_RIGHT  = 0x8; // Right to left
	const u32 ALIGNMENT_HORIZONTAL_CENTER = 0x10;
	const u32 ALIGNMENT_VERTICAL_CENTER   = 0x20;
	const u32 ALIGNMENT_CENTER = 0x40;

	enum Size_Type {
		SIZE_TYPE_FIXED,
		SIZE_TYPE_FILLED,
		SIZE_TYPE_PERCENT,
		SIZE_TYPE_GROW,
	};

	//struct Size {
	//	static Size_Dimension filled();
	//	static Size_Dimension fixed(u32 value);
	//	static Size_Dimension percent(u32 value);
	//};

	struct Padding {
		Padding() {};
		explicit Padding(s32 padding) : left(padding), top(padding), right(padding), bottom(padding) {}
		Padding(s32 left, s32 top, s32 right, s32 bottom) : left(left), top(top), right(right), bottom(bottom) {}
		~Padding() {};

		s32 left = 0;
		s32 top = 0;
		s32 right = 0;
		s32 bottom = 0;
	};

	struct Size_Dimension {
		Size_Type type;
		float value = 0.0f;
		void calculate_percent_size(s32 parent_size) { value = value * (float)parent_size; };
		s32 get() { return (s32)value; };
		void set(s32 _value) { value = (float)_value; }
	};

	inline Size_Dimension filled_size()
	{
		return { .type = SIZE_TYPE_FILLED };
	}

	inline Size_Dimension fixed_size(u32 value)
	{
		return { .type = SIZE_TYPE_FIXED, .value = (float)value };
	}

	inline Size_Dimension percent_size(float value)
	{
		return { .type = SIZE_TYPE_PERCENT, .value = value };
	}

	inline Size_Dimension grow_size()
	{
		return { .type = SIZE_TYPE_GROW };
	}

	void init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d);
	void shutdown_guiv2();

	void begin_frame();
	void end_frame();

	void begin_ui_element(const char *name);
	void end_ui_element();

	void set_position(s32 x, s32 y);
	void set_size(Size_Dimension horizontal, Size_Dimension vertical);
	void set_padding(Padding padding);
	void set_layout(Layout layout);
	void set_alignment(u32 alignment_flags);
	void set_background_color(const Color &color);
}
#endif
