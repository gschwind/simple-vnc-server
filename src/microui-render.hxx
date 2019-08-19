/*
 * microui-render.hxx
 *
 *  Created on: 19 août 2019
 *      Author: benoit.gschwind
 */

#ifndef SRC_MICROUI_RENDER_HXX_
#define SRC_MICROUI_RENDER_HXX_

#include "microui.hxx"

#include <cairo.h>

void r_init(void);
void r_draw_rect(mu_Rect rect, mu_Color color);
void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color);
void r_draw_icon(int id, mu_Rect rect, mu_Color color);
 int r_get_text_width(const char *text, int len);
 int r_get_text_height(void);
void r_set_clip_rect(mu_Rect rect);
void r_clear(mu_Color color);
void r_present(void);

extern cairo_t * cr;
extern cairo_surface_t * surf;

#endif /* SRC_MICROUI_RENDER_HXX_ */
