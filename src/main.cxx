
#include <rfb/rfb.h>

#include <algorithm>
#include <cstdio>

static void ptrAddEvent(int buttonMask,int x,int y, rfbClientRec * client)
{
    if (buttonMask != 0) {
        printf("pointer event: buttonMask = 0x%x, x = %d, y = %d, client = %p\n", buttonMask, x, y, client);
        x = std::min(800, std::max(0, x));
        y = std::min(600, std::max(0, y));

        client->screen->frameBuffer[(x+y*800)*3+0] = 0xff;
        client->screen->frameBuffer[(x+y*800)*3+1] = 0xff;
        client->screen->frameBuffer[(x+y*800)*3+2] = 0xff;

        rfbMarkRectAsModified(client->screen, x, y, x+1, y+1);

    }
}

void kbdAddEvent(rfbBool down, rfbKeySym keySym, rfbClientRec * client)
{
    printf("keyboard event: down = %d, keySym = 0x%x, client = %p\n", down, keySym, client);
}

int main(int argc, char ** argv)
{

    auto screen = rfbGetScreen(&argc, argv, 800, 600, 8, 3, 3);

    screen->ptrAddEvent = &ptrAddEvent;
    screen->kbdAddEvent = &kbdAddEvent;
    screen->frameBuffer = (char*)malloc(800*600*3);

    rfbInitServer(screen);
    rfbRunEventLoop(screen, 1000, FALSE);
    free(screen->frameBuffer);
    rfbScreenCleanup(screen);

	return 0;
}

