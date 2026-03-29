#ifndef IMGUIV2_H
#define IMGUIV2_H

#include "../libs/number_types.h"
#include "../render/renderer.h"

namespace imgui {
	enum AxisV2 {
		X_AXISV2 = 0,
		Y_AXISV2 = 1
	};

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

	struct Size_Dimension {
		Size_Type type;
		float value = 0.0f;
		void calculate_percent_size(s32 parent_size) { value = value * (float)parent_size; };
		s32 get() { return (s32)value; };
		void set(s32 _value) { value = (float)_value; }
		void add(s32 _value) { value += (float)_value; }
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

	struct Element_ID {
		Element_ID();
		explicit Element_ID(const char *str);
		~Element_ID();

		u32 hash = 0;
		String string;

		Element_ID(const Element_ID &other);
		Element_ID &operator=(const Element_ID &other);
	};

	struct Element_Size {
		Size_Dimension width;
		Size_Dimension height;

		Size_Dimension operator[](AxisV2 axis);
	};

	struct UI_Element {
		UI_Element();
		UI_Element(Element_ID element_id, UI_Element *parent_element);
		~UI_Element();

		u32 flags;

		u32 called = 0;
		u32 children_id_counter;
		UI_Element *position_relative_element = NULL;

		/*Rect_s32 rect;*/
		Point_s32 position;
		Point_s32 prev_position;
		Element_Size size;
		Element_Size prev_size;

		//Theme
		s32 space;
		Padding padding;

		Layout layout;
		u32 rounding;
		u32 rounding_flags;
		u32 alignment_flags;

		Color background_color;

		//Content
		const char *text = NULL;

		//Context
		Element_ID id;
		UI_Element *parent_element = NULL;
		Array<UI_Element *> child_elements;

		void begin_frame();
		void add_child(UI_Element *ui_element);

		bool root_element();
		UI_Element *find_child(Element_ID element_id);
	};

	void init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d);
	void shutdown_guiv2();

	void begin_frame();
	void end_frame();

	void begin_ui_element(const char *name);
	void end_ui_element();

	void set_position(s32 x, s32 y);
	void set_position(UI_Element *ui_element, s32 x, s32 y);
	void set_size(Size_Dimension horizontal, Size_Dimension vertical);
	void set_space(s32 space);
	void set_padding(Padding padding);
	void set_layout(Layout layout);
	void set_alignment(u32 alignment_flags);
	void set_background_color(const Color &color);
	void set_rounding(u32 rounding, u32 rounding_flags = ROUND_RECT);

	void text(const char *text);

	bool ui_element_hovered();
	bool ui_element_clicked();

	UI_Element *get_ui_element();
}
#endif
