
#include <rfb/rfb.h>

#include <algorithm>
#include <string>
#include <list>
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>

#include <xkbcommon/xkbcommon.h>

#include <cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#include "microui-render.hxx"

std::list<rfbScreenInfoPtr> screens;
int listen_socket = -1;

std::map<int, int> button_map = {
        {0, MU_MOUSE_LEFT},
        {1, MU_MOUSE_RIGHT},
        {2, MU_MOUSE_MIDDLE},
};

std::map<int, int> key_map = {
        {XKB_KEY_Shift_L,   MU_KEY_SHIFT},
        {XKB_KEY_Shift_R,   MU_KEY_SHIFT},
        {XKB_KEY_Control_L, MU_KEY_CTRL},
        {XKB_KEY_Control_R, MU_KEY_CTRL},
        {XKB_KEY_Alt_L,     MU_KEY_ALT},
        {XKB_KEY_Alt_R,     MU_KEY_ALT},
        {XKB_KEY_Return,    MU_KEY_RETURN},
        {XKB_KEY_BackSpace, MU_KEY_BACKSPACE},
};

static void write_log(const char *text) {
    printf("%s\n", text);
}

mu_Real bg[3] = {128};

static void test_window(mu_Context *ctx) {
  static mu_Container window;

  /* init window manually so we can set its position and size */
  if (!window.inited) {
    mu_init_window(ctx, &window, 0);
    window.rect = mu_rect(40, 40, 800, 450);
  }

  /* limit window to minimum size */
//  window.rect.w = mu_max(window.rect.w, 240);
//  window.rect.h = mu_max(window.rect.h, 300);


  /* do window */
  if (mu_begin_popup(ctx, &window)) {

    /* window info */
    static int show_info = 0;
    if (mu_header(ctx, &show_info, "Window Info")) {
      char buf[64];
      int tmp0[] = { 54, -1 };
      mu_layout_row(ctx, 2, tmp0, 0);
      static char bufx[100] = {0};
      mu_textbox(ctx,bufx, 100);
      sprintf(buf, "%d, %d", window.rect.x, window.rect.y); mu_label(ctx, buf);
      mu_label(ctx, "Size:");
      sprintf(buf, "%d, %d", window.rect.w, window.rect.h); mu_label(ctx, buf);
    }

    /* labels + buttons */
    static int show_buttons = 1;
    if (mu_header(ctx, &show_buttons, "Test Buttons")) {
      int tmp0[] = { 86, -110, -1 };
      mu_layout_row(ctx, 3, tmp0, 0);
      mu_label(ctx, "Test buttons 1:");
      if (mu_button(ctx, "Button 1")) { write_log("Pressed button 1"); }
      if (mu_button(ctx, "Button 2")) { write_log("Pressed button 2"); }
      mu_label(ctx, "Test buttons 2:");
      if (mu_button(ctx, "Button 3")) { write_log("Pressed button 3"); }
      if (mu_button(ctx, "Button 4")) { write_log("Pressed button 4"); }
    }

    /* tree */
    static int show_tree = 1;
    if (mu_header(ctx, &show_tree, "Tree and Text")) {
      int tmp0[] = { 140, -1 };
      mu_layout_row(ctx, 2, tmp0, 0);
      mu_layout_begin_column(ctx);
      static int states[8];
      if (mu_begin_treenode(ctx, &states[0], "Test 1")) {
        if (mu_begin_treenode(ctx, &states[1], "Test 1a")) {
          mu_label(ctx, "Hello");
          mu_label(ctx, "world");
          mu_end_treenode(ctx);
        }
        if (mu_begin_treenode(ctx, &states[2], "Test 1b")) {
          if (mu_button(ctx, "Button 1")) { write_log("Pressed button 1"); }
          if (mu_button(ctx, "Button 2")) { write_log("Pressed button 2"); }
          mu_end_treenode(ctx);
        }
        mu_end_treenode(ctx);
      }
      if (mu_begin_treenode(ctx, &states[3], "Test 2")) {
        int tmp0[] = { 54, 54 };
        mu_layout_row(ctx, 2, tmp0, 0);
        if (mu_button(ctx, "Button 3")) { write_log("Pressed button 3"); }
        if (mu_button(ctx, "Button 4")) { write_log("Pressed button 4"); }
        if (mu_button(ctx, "Button 5")) { write_log("Pressed button 5"); }
        if (mu_button(ctx, "Button 6")) { write_log("Pressed button 6"); }
        mu_end_treenode(ctx);
      }
      if (mu_begin_treenode(ctx, &states[4], "Test 3")) {
        static int checks[3] = { 1, 0, 1 };
        mu_checkbox(ctx, &checks[0], "Checkbox 1");
        mu_checkbox(ctx, &checks[1], "Checkbox 2");
        mu_checkbox(ctx, &checks[2], "Checkbox 3");
        mu_end_treenode(ctx);
      }
      mu_layout_end_column(ctx);

      mu_layout_begin_column(ctx);
      int tmp1[]  = { -1 };
      mu_layout_row(ctx, 1, tmp1, 0);
      mu_text(ctx, "Lorem ipsum dolor sit amet, consectetur adipiscing "
        "elit. Maecenas lacinia, sem eu lacinia molestie, mi risus faucibus "
        "ipsum, eu varius magna felis a nulla.");
      mu_layout_end_column(ctx);
    }

    /* background color sliders */
    static int show_sliders = 1;
    if (mu_header(ctx, &show_sliders, "Background Color")) {
      int tmp0[] = { -78, -1 };
      mu_layout_row(ctx, 2, tmp0, 74);
      /* sliders */
      mu_layout_begin_column(ctx);
      int tmp1[] = { 46, -1 };
      mu_layout_row(ctx, 2, tmp1, 0);
      mu_label(ctx, "Red:");   mu_slider(ctx, &bg[0], 0, 255);
      mu_label(ctx, "Green:"); mu_slider(ctx, &bg[1], 0, 255);
      mu_label(ctx, "Blue:");  mu_slider(ctx, &bg[2], 0, 255);
      mu_layout_end_column(ctx);
      /* color preview */
      mu_Rect r = mu_layout_next(ctx);
      mu_draw_rect(ctx, r, mu_color(bg[0], bg[1], bg[2], 255));
      char buf[32];
      sprintf(buf, "#%02X%02X%02X", (int) bg[0], (int) bg[1], (int) bg[2]);
      mu_draw_control_text(ctx, buf, r, MU_COLOR_TEXT, MU_OPT_ALIGNCENTER);
    }

    mu_end_popup(ctx);
  }
}


void paint_text(cairo_t * cr, int x, int y, std::string text)
{
    auto pango_font = pango_font_description_from_string("Arial");
//    auto pango_font_map = pango_cairo_font_map_new();

    auto pango_font_map = pango_cairo_font_map_get_default();
    auto pango_context = pango_font_map_create_context(pango_font_map);

    cairo_save(cr);

    cairo_translate(cr, x, y);

    cairo_set_line_width(cr, 3.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

    {
        PangoLayout * pango_layout = pango_layout_new(pango_context);
        pango_layout_set_font_description(pango_layout, pango_font);
        pango_cairo_update_layout(cr, pango_layout);
        pango_layout_set_text(pango_layout, text.c_str(), -1);
        pango_layout_set_width(pango_layout, 1000 * PANGO_SCALE);
        pango_layout_set_wrap(pango_layout, PANGO_WRAP_CHAR);
        pango_layout_set_ellipsize(pango_layout, PANGO_ELLIPSIZE_END);
        pango_cairo_layout_path(cr, pango_layout);
        g_object_unref(pango_layout);
    }

    cairo_stroke_preserve(cr);

    cairo_set_line_width(cr, 1.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_fill(cr);

    cairo_restore(cr);
}


struct client {

    mu_Context * ctx;

    cairo_surface_t * surf;
    rfbScreenInfoPtr screen;


    void paint_dialog(int x, int y)
    {
////        auto surf = cairo_image_surface_create_for_data((unsigned char*)screen->frameBuffer, CAIRO_FORMAT_ARGB32, 800, 600, 800*4);
//        auto cr = cairo_create(surf);
//
//        cairo_translate(cr, x, y);
//
//        cairo_rectangle(cr, -100, 0, 200, 100);
//        cairo_set_source_rgb(cr, 0.7, 0.7, 0.0);
//        cairo_fill(cr);
//
//        paint_text(cr, -100, 10, &msg[0]);
//        paint_text(cr, -100, 50, &rsp[0]);
//
//        cairo_destroy(cr);
//        cairo_surface_flush(surf);
//
//        rfbMarkRectAsModified(screen, x-100, y, x+201, y+100);

    }

    void update_frame() {

        mu_begin(ctx);
        test_window(ctx);
        mu_end(ctx);

        /* render */
        r_clear(mu_color(bg[0], bg[1], bg[2], 0));
        mu_Command *cmd = NULL;
        while (mu_next_command(ctx, &cmd)) {
          switch (cmd->type) {
            case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
            case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
            case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
            case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
          }
        }
        r_present();

        rfbMarkRectAsModified(screen, 0, 0, 800, 600);

//        printf("update frame\n");

    }


};

static void ptrAddEvent(int buttonMask,int x,int y, rfbClientRec * client)
{
    auto c = reinterpret_cast<struct client*>(client->screen->screenData);
    static int old_buttonMask = 0;
    static int ox = 0;
    static int oy = 0;

    if (old_buttonMask != buttonMask) {

        for (int i = 0; i < 3; ++i) {
            if (((1<<i)&buttonMask) ^ ((1<<i)&old_buttonMask)) {
                if ((1<<i)&buttonMask) {
                    mu_input_mousedown(c->ctx, x, y, button_map[i]);
                } else {
                    mu_input_mouseup(c->ctx, x, y, button_map[i]);
                }
            }
        }

        old_buttonMask = buttonMask;

    } else {
        mu_input_mousemove(c->ctx, x, y);

    }

    c->update_frame();

}

void kbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec * client)
{
    printf("keyboard event: down = %d, keySym = 0x%x, client = %p\n", down, keySym, client);

        auto c = reinterpret_cast<struct client*>(client->screen->screenData);

        auto x = key_map.find(keySym);
        if (x != key_map.end()) {
            if (down)
                mu_input_keydown(c->ctx, x->second);
            else
                mu_input_keyup(c->ctx, x->second);

            c->update_frame();

        } else {
            char buf[9];
            int s = xkb_keysym_to_utf8(keySym, buf, 8);
            if (s != 0 and s < 8) {
                buf[s] = 0;
                mu_input_text(c->ctx, buf);
                c->update_frame();
            }
        }

}


void clientGoneHook(rfbClientRec* cl)
{
    rfbShutdownServer(cl->screen, FALSE);
}

rfbNewClientAction newClientHook(rfbClientRec* cl)
{
    cl->clientGoneHook = &clientGoneHook;
    return RFB_CLIENT_ACCEPT;
}

static int text_width(mu_Font font, const char *text, int len) {
  if (len == -1) { len = strlen(text); }
  return r_get_text_width(text, len);
}

static int text_height(mu_Font font) {
  return r_get_text_height();
}


void new_client(int sock)
{

    int pid = fork();

    if (pid < 0) {
        printf("failed to fork");
        exit(1);

    } else if (pid == 0) { // child

        /** create a new screen for the next connection **/
        auto screen = rfbGetScreen(nullptr, nullptr, 800, 600, 8, 3, 4);
        screen->ptrAddEvent = &ptrAddEvent;
        screen->kbdAddEvent = &kbdAddEvent;
        screen->frameBuffer = (char*)malloc(800*600*4);
        screen->inetdSock = sock;
        screen->newClientHook = &newClientHook;
        printf("new client %d\n", screen->inetdSock);

        auto data = new client;
        screen->screenData = data;

        ::surf = cairo_image_surface_create_for_data((unsigned char*)screen->frameBuffer, CAIRO_FORMAT_ARGB32, 800, 600, 800*4);
        ::cr = cairo_create(::surf);

        data->screen = screen;

        r_init();

        /* init microui */
        data->ctx = (mu_Context*)malloc(sizeof(mu_Context));
        mu_init(data->ctx);
        data->ctx->text_width = text_width;
        data->ctx->text_height = text_height;

        data->paint_dialog(400,300);

        rfbInitServer(screen);

        close(listen_socket);

        rfbRunEventLoop(screen, 1000, FALSE);

        free(screen->frameBuffer);
        free(screen);

        exit(0);


    }

    // parent

    // close socket
    close(sock);


}

// avoid zombies
void handle_sigchld(int n) {
    int saved_errno = errno;
    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
    errno = saved_errno;
}

int main(int argc, char ** argv)
{

    {
        struct sigaction sa;
        sa.sa_handler = &handle_sigchld;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
        if (sigaction(SIGCHLD, &sa, 0) == -1) {
          perror(0);
          exit(1);
        }
    }

    in_addr_t addr = htonl(INADDR_ANY);

    listen_socket = rfbListenOnTCPPort(5900, addr);

    if (listen_socket < 0) {
        printf("failed to open the listen socket");
        return 1;
    }


    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(listen_socket, &fds);
        int maxfd = listen_socket;

        int nfds = select(maxfd + 1, &fds, NULL, NULL, NULL);

        if (FD_ISSET(listen_socket, &fds)) {
            int sock = accept(listen_socket, nullptr, nullptr);
            if (sock >= 0) {
                new_client(sock);
            }
        }

    }

	return 0;
}
