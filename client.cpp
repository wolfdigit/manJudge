#include <cstdio>
#include <windows.h>

using namespace std;

int main()
{
    char pipeName[64];
    sprintf(pipeName, "\\\\.\\pipe\\myPipe%02d", 1);

    while (1) {
        char buff[256];
        fgets(buff, sizeof(buff), stdin);

        HANDLE hPipe;
        do {
            hPipe = CreateFile(pipeName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        } while (hPipe==INVALID_HANDLE_VALUE);

        unsigned long nWrite;
        WriteFile(hPipe, buff, strlen(buff)+1, &nWrite, NULL);

        CloseHandle(hPipe);
    }

    return 0;
}
