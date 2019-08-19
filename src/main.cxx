
#include <rfb/rfb.h>

#include <algorithm>
#include <string>
#include <list>
#include <cstdio>
#include <cassert>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/wait.h>

#include <cairo.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

std::list<rfbScreenInfoPtr> screens;
int listen_socket = -1;

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


static void ptrAddEvent(int buttonMask,int x,int y, rfbClientRec * client)
{
    if (buttonMask != 0) {
        printf("pointer event: buttonMask = 0x%x, x = %d, y = %d, client = %p\n", buttonMask, x, y, client);
        x = std::min(800, std::max(0, x));
        y = std::min(600, std::max(0, y));

        client->screen->frameBuffer[(x+y*800)*4+0] = 0xff;
        client->screen->frameBuffer[(x+y*800)*4+1] = 0xff;
        client->screen->frameBuffer[(x+y*800)*4+2] = 0xff;

        rfbMarkRectAsModified(client->screen, x, y, x+1, y+1);

    }
}

void kbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec * client)
{
    printf("keyboard event: down = %d, keySym = 0x%x, client = %p\n", down, keySym, client);
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

        auto surf = cairo_image_surface_create_for_data((unsigned char*)screen->frameBuffer, CAIRO_FORMAT_ARGB32, 800, 600, 800*4);
        auto cr = cairo_create(surf);

        char buf[100];
        snprintf(buf, 100, "socket number %d", sock);
        paint_text(cr, 10, 10, buf);

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
