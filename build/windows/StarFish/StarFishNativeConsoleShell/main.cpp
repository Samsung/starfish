#include <stdio.h>
#include <Windows.h>
#include <inttypes.h>
#define STARFISH_EXPORT

extern "C" size_t STARFISH_EXPORT __stdcall createWebViewInstance(
    uint32_t initialWidth, uint32_t initialHeight, void* initialBuffer, uint32_t initialBufferStride);
extern "C" void STARFISH_EXPORT __stdcall loadURL(size_t webViewInstance,
                                                  size_t utf8URL,
                                                  uint32_t urlLength);
extern "C" void STARFISH_EXPORT __stdcall startMessageLoop(
    size_t webViewInstance);
extern "C" void STARFISH_EXPORT __stdcall giveMessage(size_t webViewInstance,
                                                      MSG msg);

extern "C" size_t STARFISH_EXPORT __stdcall internalDrawingBufferAddress(
    size_t webViewInstance);

extern "C" uint32_t STARFISH_EXPORT __stdcall internalDrawingBufferWidth(
    size_t webViewInstance);
extern "C" uint32_t STARFISH_EXPORT __stdcall internalDrawingBufferHeight(
    size_t webViewInstance);
extern "C" uint32_t STARFISH_EXPORT __stdcall internalDrawingBufferStride(
    size_t webViewInstance);
extern "C" void STARFISH_EXPORT __stdcall resizeWindow(size_t webViewInstance,
                                                       uint32_t w, uint32_t h);
extern "C" void STARFISH_EXPORT __stdcall dispatchMouseDownEvent(
    size_t webViewInstance, float x, float y);
extern "C" void STARFISH_EXPORT __stdcall dispatchMouseUpEvent(
    size_t webViewInstance, float x, float y);
extern "C" void STARFISH_EXPORT __stdcall dispatchMouseMoveEvent(
    size_t webViewInstance, float x, float y, bool isLButtonPressed,
    bool isRButtonPressed);

CRITICAL_SECTION cs;
char urlbuf[256];

int main()
{
    InitializeCriticalSection(&cs);
    CreateThread(NULL, 1024 * 1024 * 4,
                 [](void*) -> DWORD {
                     size_t starfishInstance = createWebViewInstance(1024, 768, malloc(1024 * 768 * 4), 1024 * 4);

                     loadURL(starfishInstance, (size_t)("http://naver.com"),
                             sizeof("http://naver.com") - 1);

                     MSG msg;
                     while (true)
                     {
                         bool hasMessage =
                             (0 != PeekMessage(&msg, NULL, 0, 0,
                                               PM_NOREMOVE));
                         if (hasMessage) {
                             GetMessage(&msg, NULL, 0, 0);
                             giveMessage(starfishInstance, msg);
                             // printf("msg... %d\n", (int)msg.message);
                         } else {
                             EnterCriticalSection(&cs);
                             if (strlen(urlbuf) != 0) {
                                 loadURL(starfishInstance, (size_t)urlbuf,
                                         strlen(urlbuf));
                             }
                             urlbuf[0] = 0;
                             LeaveCriticalSection(&cs);
                         }
                     }
                     return 0;
                 },
                 NULL, 0, NULL);

    while (true) {
        char buf[256];
        scanf("%s", buf);
        EnterCriticalSection(&cs);
        memcpy(urlbuf, buf, 256);
        LeaveCriticalSection(&cs);
    }

    return 0;
}
