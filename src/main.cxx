
#include <rfb/rfb.h>

#include <algorithm>
#include <string>
#include <list>
#include <cstdio>
#include <cassert>
#include <vector>
#include <map>
#include <memory>

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


static mu_Style style = {
  NULL,       /* font */
  { 68, 10 }, /* size */
  6, 4, 24,   /* padding, spacing, indent */
  26,         /* title_height */
  12, 8,      /* scrollbar_size, thumb_size */
  {
    { 230, 230, 230, 255 }, /* MU_COLOR_TEXT */
    { 25,  25,  25,  255 }, /* MU_COLOR_BORDER */
    { 50,  50,  50,  255 }, /* MU_COLOR_WINDOWBG */
    { 20,  20,  20,  255 }, /* MU_COLOR_TITLEBG */
    { 240, 240, 240, 255 }, /* MU_COLOR_TITLETEXT */
    { 0,   0,   0,   0   }, /* MU_COLOR_PANELBG */
    { 75,  75,  75,  255 }, /* MU_COLOR_BUTTON */
    { 95,  95,  95,  255 }, /* MU_COLOR_BUTTONHOVER */
    { 115, 115, 115, 255 }, /* MU_COLOR_BUTTONFOCUS */
    { 30,  30,  30,  255 }, /* MU_COLOR_BASE */
    { 35,  35,  35,  255 }, /* MU_COLOR_BASEHOVER */
    { 40,  40,  40,  255 }, /* MU_COLOR_BASEFOCUS */
    { 43,  43,  43,  255 }, /* MU_COLOR_SCROLLBASE */
    { 30,  30,  30,  255 }  /* MU_COLOR_SCROLLTHUMB */
  }
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
        window.rect = mu_rect((800-400)/2, (600-400)/2, 400, 400);
  }

  /* do window */
  if (mu_begin_popup(ctx, &window)) {

    /* window info */
//    int tmp0[] = { 54, 100 };
    mu_layout_rowx<2>(ctx, {54, 100}, 0);
    static char bufx[100] = {0};
    static char pasx[100] = {0};
    mu_label(ctx, "Login:");

    mu_textbox(ctx,bufx, 100);

    mu_label(ctx, "Password:");

    if(mu_textbox_ex(ctx, pasx, 100, MU_OPT_PASSWD) & MU_RES_SUBMIT) {
        // TODO login;
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

    std::shared_ptr<svs::render> render;

    void update_frame()
    {

        printf("update frame\n");

        mu_begin(ctx);
        test_window(ctx);
        mu_end(ctx);

        /* render */
        render->dorender(ctx);
        rfbMarkRectAsModified(screen, 0, 0, 800, 600);

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
    printf("keyboard event: down = %d, keySym = 0x%x, client = %p\n", down,
            keySym, client);

    auto c = reinterpret_cast<struct client*>(client->screen->screenData);

    auto x = key_map.find(keySym);
    if (x != key_map.end()) {
        if (down) {
            mu_input_keydown(c->ctx, x->second);
        } else {
            mu_input_keyup(c->ctx, x->second);
        }

        c->update_frame();

    } else if (down) {
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
    auto r = reinterpret_cast<svs::render*>(font);
      if (len == -1) { len = strlen(text); }
      return r->get_text_width(text, len);
}

static int text_height(mu_Font font) {
  auto r = reinterpret_cast<svs::render*>(font);
  return r->get_text_height();
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

        data->screen = screen;
        data->render = std::make_shared<svs::render>(screen->frameBuffer, CAIRO_FORMAT_ARGB32, 800, 600, 800*4);

        /* init microui */
        data->ctx = (mu_Context*)malloc(sizeof(mu_Context));
        mu_init(data->ctx);
        data->ctx->text_width = text_width;
        data->ctx->text_height = text_height;
        data->ctx->style = &style;
        style.font = data->render.get();

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
