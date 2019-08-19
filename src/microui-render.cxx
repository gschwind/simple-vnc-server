/*
 * microui-render.cxx
 *
 *  Created on: 19 août 2019
 *      Author: benoit.gschwind
 */

#include "microui-render.hxx"

namespace svs {

void render::draw_rect(mu_Rect rect, mu_Color color)
{
//    cairo_save(cr);
    cairo_set_source_rgba(cr, color.r/255.0, color.g/255.0, color.b/255.0, color.a/255.0);
    cairo_rectangle(cr, rect.x, rect.y, rect.w, rect.h);
    cairo_fill(cr);
//    cairo_restore(cr);
}

void render::draw_text(const char *text, mu_Vec2 pos, mu_Color color)
{
//    printf("draw text %s\n", text);

    cairo_save(cr);

    cairo_translate(cr, pos.x, pos.y-get_text_height()/2.0);

    cairo_set_line_width(cr, 3.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

    {
        PangoLayout * pango_layout = pango_layout_new(pango_context);
        pango_layout_set_font_description(pango_layout, pango_font);
        pango_cairo_update_layout(cr, pango_layout);
        pango_layout_set_text(pango_layout, text, -1);
//        pango_layout_set_width(pango_layout, 1000 * PANGO_SCALE);
//        pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
//        pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
        pango_cairo_layout_path(cr, pango_layout);
        g_object_unref(pango_layout);
    }

//    cairo_stroke_preserve(cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgba(cr, color.r/255.0, color.g/255.0, color.b/255.0, color.a/255.0);
    cairo_fill(cr);

    cairo_restore(cr);
}

void render::draw_icon(int id, mu_Rect rect, mu_Color color)
{
    //    cairo_save(cr);
        cairo_set_source_rgba(cr, color.r/255.0, color.g/255.0, color.b/255.0, color.a/255.0);
        cairo_rectangle(cr, rect.x, rect.y, rect.w, rect.h);
        cairo_fill(cr);
    //    cairo_restore(cr);
}

 int render::get_text_width(const char *text, int len)
 {
     PangoLayout * pango_layout = pango_layout_new(pango_context);
     pango_layout_set_font_description(pango_layout, pango_font);
     pango_layout_set_text(pango_layout, text, -1);
     PangoRectangle ink;
     PangoRectangle log;
     pango_layout_get_pixel_extents(pango_layout, &ink, &log);
     g_object_unref(pango_layout);
     return log.width/PANGO_SCALE;
 }

int render::get_text_height(void)
{
    return 10;
}

void render::set_clip_rect(mu_Rect rect)
{
    cairo_reset_clip(cr);
    cairo_rectangle(cr, rect.x, rect.y, rect.w, rect.y);
    cairo_clip(cr);
}

void render::clear(mu_Color color)
{
    cairo_reset_clip(cr);
    cairo_set_source_rgba(cr, color.r/255.0, color.g/255.0, color.b/255.0, 1.0-color.a/255.0);
    cairo_paint(cr);
}

void render::present(void)
{
    cairo_surface_flush(surf);
}

void render::dorender(mu_Context * ctx)
{
    /* render */
    clear(mu_color(0, 0, 0, 0));
    mu_Command *cmd = NULL;
    while (mu_next_command(ctx, &cmd)) {
      switch (cmd->type) {
        case MU_COMMAND_TEXT: draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
        case MU_COMMAND_RECT: draw_rect(cmd->rect.rect, cmd->rect.color); break;
        case MU_COMMAND_ICON: draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
        case MU_COMMAND_CLIP: set_clip_rect(cmd->clip.rect); break;
      }
    }
    present();
}

}
