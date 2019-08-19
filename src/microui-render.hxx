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
#include <pango/pango.h>
#include <pango/pangocairo.h>

namespace svs {

struct render {

    cairo_t * cr;
    cairo_surface_t * surf;

    PangoFontDescription * pango_font;
    PangoFontMap * pango_font_map;
    PangoContext * pango_context;

    render(void * data, cairo_format_t format, int width, int height, int stride)
    {
        surf = cairo_image_surface_create_for_data((unsigned char*)data, format, width, height, stride);
        cr = cairo_create(surf);

        pango_font = pango_font_description_from_string("Arial");
        pango_font_map = pango_cairo_font_map_get_default();
        pango_context = pango_font_map_create_context(pango_font_map);
    }

    ~render()
    {
        cairo_destroy(cr);
        cairo_surface_destroy(surf);
    }

    void init(void);
    void draw_rect(mu_Rect rect, mu_Color color);
    void draw_text(const char *text, mu_Vec2 pos, mu_Color color);
    void draw_icon(int id, mu_Rect rect, mu_Color color);
     int get_text_width(const char *text, int len);
     int get_text_height(void);
    void set_clip_rect(mu_Rect rect);
    void clear(mu_Color color);
    void present(void);

    void dorender(mu_Context * ctx);

};

}
#endif /* SRC_MICROUI_RENDER_HXX_ */
