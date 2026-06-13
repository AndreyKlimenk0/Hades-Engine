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
		SIZE_TYPE_FIT,     // The element takes into account size of child elements, spacing, padding and adapts its own size accordingly.
		SIZE_TYPE_PERCENT,
		SIZE_TYPE_GROW,    // The element takes into account the size of the parent element and other child elements and occupies the remaining available space.
	};

	struct Size_Dimension {
		Size_Type type;
		float value = 0.0f;
		void calculate_percent_size(s32 parent_size) { value = value * (float)parent_size; };
		s32 get() { return (s32)value; };
		void set(s32 _value) { value = (float)_value; }
		void add(s32 _value) { value += (float)_value; }
		void sub(s32 _value) { value -= (float)_value; }
	};

	inline Size_Dimension fit_size()
	{
		return { .type = SIZE_TYPE_FIT };
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

		s32 operator[](AxisV2 axis);
	};

	inline Padding operator+(const Padding &first, const Padding &second)
	{
		return Padding(first.left + second.left, first.top + second.top, first.right + second.right, first.bottom + second.bottom);
	}

	inline Padding operator-(const Padding &first, const Padding &second)
	{
		return Padding(first.left - second.left, first.top - second.top, first.right - second.right, first.bottom - second.bottom);
	}

	struct Element_ID {
		Element_ID();
		explicit Element_ID(const char *str);
		~Element_ID();

		u32 hash = 0;
		String string;

		Element_ID(const Element_ID &other);
		Element_ID &operator=(const Element_ID &other);

		void reset();
	};

	inline bool operator==(const Element_ID &first_id, const Element_ID &second_id)
	{
#ifdef _DEBUG
		return (first_id.hash == second_id.hash) && (first_id.string == second_id.string);
#else
		return first_id.hash == second_id.hash;
#endif
	}

	inline bool operator!=(const Element_ID &first_id, const Element_ID &second_id)
	{
		return !(first_id.hash == second_id.hash);
	}

	struct Element_Size {
		Size_Dimension width;
		Size_Dimension height;

		Size_Dimension &operator[](AxisV2 axis);
	};

	struct UI_Element {
		UI_Element();
		UI_Element(Element_ID element_id, UI_Element *parent_element);
		~UI_Element();

		u32 flags;

		u32 called = 0;
		u32 children_id_counter;
		u32 outlining_thikness;

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
		Color outlining_color;

		//Content
		String text;

		//Context
		Element_ID id;
		UI_Element *parent_element = NULL;
		Array<UI_Element *> child_elements;

		void begin_frame();
		void add_child(UI_Element *ui_element);

		bool root_element();
		UI_Element *find_child(Element_ID element_id);

		Rect_s32 get_rect()
		{
			return { prev_position.x, prev_position.y, prev_size.width.get(), prev_size.height.get() };
		}
	};

	void init_guiv2(u32 window_width, u32 window_height, const char *font_name, u32 font_size, Render_2D *render_2d);
	void shutdown_guiv2();

	void begin_frame();
	void end_frame();

	void begin_ui_element(const char *name);
	void end_ui_element();

	void set_absolute_position_x(s32 x);
	void set_absolute_position_y(s32 y);
	void set_absolute_position(s32 x, s32 y);
	void set_relative_position_x(s32 x);
	void set_relative_position_y(s32 y);
	void set_relative_position(s32 x, s32 y);
	
	void set_size(Size_Dimension horizontal, Size_Dimension vertical);
	void set_space(s32 space);
	void set_padding(Padding padding);
	void set_layout(Layout layout);
	void set_alignment(u32 alignment_flags);
	void set_background_color(const Color &color);
	void set_rounding(u32 rounding, u32 rounding_flags = ROUND_RECT);
	void set_outlining(u32 thikness, const Color &color);

	void text(const char *text);
	void text(const char *ui_element_name, const char *text);

	bool ui_element_hovered();
	bool ui_element_clicked();
	bool ui_element_double_clicked();

	Rect_s32 ui_element_rect();

	UI_Element *get_ui_element();
}
#endif
