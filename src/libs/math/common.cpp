#include "common.h"

static int compare_rects_u32(const void *first_rect, const void *second_rect)
{
	assert(first_rect);
	assert(second_rect);

	const Rect_u32 *first = static_cast<const Rect_u32 *>(first_rect);
	const Rect_u32 *second = static_cast<const Rect_u32 *>(second_rect);

	return first->height > second->height;
}

void pack_rects_in_rect(Rect_u32 *main_rect, Array<Rect_u32 *> &rects)
{
	assert(main_rect);

	qsort(rects.items, rects.count, sizeof(rects[0]), compare_rects_u32);

	u32 x_pos = 0;
	u32 y_pos = 0;
	u32 large = 0;

	Rect_u32 *rect = NULL;
	For(rects, rect) {

		if ((rect->width + x_pos) > main_rect->width) {
			y_pos += large;
			x_pos = 0;
			large = 0;
		}

		if ((y_pos + rect->height) > main_rect->height) {
			break;
		}

		rect->x = x_pos;
		rect->y = y_pos;

		x_pos += rect->width;

		if (rect->height > large) {
			large = rect->height;
		}
	}
}
