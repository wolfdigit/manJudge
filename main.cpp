#include <cstdio>
#include <windows.h>
#include <process.h>

//using namespace std;
int myPipeId = -1;
FILE *pPipe=NULL;

void ErrorExit(PTSTR);

HANDLE hChildIn, hChildOut;

void pipeInThread(void *argp) {
    HANDLE hPipe;
    for (int p=0; p<10; p++) {
        char pipeName[64];
        sprintf(pipeName, "\\\\.\\pipe\\myPipe%02d", p);

        hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE|PIPE_WAIT, /*PIPE_UNLIMITED_INSTANCES*/1, 256, 256, 0, 0);

        if (hPipe!=INVALID_HANDLE_VALUE) {
            myPipeId = p;
            break;
        }
    }


    int cnt=0;
    while (hPipe != INVALID_HANDLE_VALUE) {
        ++cnt;
        //printf("opening pipe..., cnt=%d\n", cnt);
        if (ConnectNamedPipe(hPipe, NULL)!=false) {
            //printf("pipe waiting, cnt=%d\n", cnt);
            char buff[256];
            unsigned long nRead, nWrite;
            while (ReadFile(hPipe, buff, sizeof(buff)-1, &nRead, NULL)!=false) {
                buff[nRead] = '\0';
                //printf("got {%s} !!!\n", buff);
                printf("%s", buff);
                WriteFile(hChildIn, buff, nRead, &nWrite, NULL);
            }
        }
        //printf("closing pipe, cnt=%d\n", cnt);
        DisconnectNamedPipe(hPipe);
    }

    exit(0);
}

void createChildProcess(char exename[], HANDLE *inPipe, HANDLE *outPipe) {
   HANDLE g_hChildStd_OUT_Wr = NULL;
   HANDLE g_hChildStd_IN_Rd = NULL;

   SECURITY_ATTRIBUTES saAttr;

   //printf("\n->Start of parent execution.\n");

// Set the bInheritHandle flag so pipe handles are inherited.

   saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
   saAttr.bInheritHandle = TRUE;
   saAttr.lpSecurityDescriptor = NULL;

// Create a pipe for the child process's STDOUT.

   if ( ! CreatePipe(outPipe, &g_hChildStd_OUT_Wr, &saAttr, 0) )
      ErrorExit(TEXT("StdoutRd CreatePipe"));

// Ensure the read handle to the pipe for STDOUT is not inherited.

   if ( ! SetHandleInformation(*outPipe, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdout SetHandleInformation"));

// Create a pipe for the child process's STDIN.

   if (! CreatePipe(&g_hChildStd_IN_Rd, inPipe, &saAttr, 0))
      ErrorExit(TEXT("Stdin CreatePipe"));

// Ensure the write handle to the pipe for STDIN is not inherited.

   if ( ! SetHandleInformation(*inPipe, HANDLE_FLAG_INHERIT, 0) )
      ErrorExit(TEXT("Stdin SetHandleInformation"));

// Create the child process.

   //CreateChildProcess();

   TCHAR *szCmdline=TEXT(exename);
   PROCESS_INFORMATION piProcInfo;
   STARTUPINFO siStartInfo;
   BOOL bSuccess = FALSE;

// Set up members of the PROCESS_INFORMATION structure.

   ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

// Set up members of the STARTUPINFO structure.
// This structure specifies the STDIN and STDOUT handles for redirection.

   ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
   siStartInfo.cb = sizeof(STARTUPINFO);
   siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
   siStartInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
   siStartInfo.hStdInput = g_hChildStd_IN_Rd;
   siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

// Create the child process.

   bSuccess = CreateProcess(NULL,
      szCmdline,     // command line
      NULL,          // process security attributes
      NULL,          // primary thread security attributes
      TRUE,          // handles are inherited
      0,             // creation flags
      NULL,          // use parent's environment
      NULL,          // use parent's current directory
      &siStartInfo,  // STARTUPINFO pointer
      &piProcInfo);  // receives PROCESS_INFORMATION

   // If an error occurs, exit the application.
   if ( ! bSuccess )
      ErrorExit(TEXT("CreateProcess"));
   else
   {
      // Close handles to the child process and its primary thread.
      // Some applications might keep these handles to monitor the status
      // of the child process, for example.

      CloseHandle(piProcInfo.hProcess);
      CloseHandle(piProcInfo.hThread);
   }

}

int main(int argc, char *argv[])
{
    if (argc<2) exit(-1);
    createChildProcess(argv[1], &hChildIn, &hChildOut);
    _beginthread(pipeInThread, 0, NULL);

    // https://msdn.microsoft.com/en-us/library/ms682499(VS.85).aspx

    while (1) {
        char buff[256];
        //fgets(buff, sizeof(buff), stdin);
        unsigned long nRead;
        unsigned long nWrite;

        ReadFile(GetStdHandle(STD_INPUT_HANDLE), buff, sizeof(buff), &nRead, NULL);

        for (int p=0; p<10; p++) {
            if (p==myPipeId) {
                WriteFile(hChildIn, buff, nRead, &nWrite, NULL);
            }
            else {
                char pipeName[64];
                sprintf(pipeName, "\\\\.\\pipe\\myPipe%02d", p);

                HANDLE hPipeC;
                hPipeC = CreateFile(pipeName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (hPipeC==INVALID_HANDLE_VALUE) continue;

                WriteFile(hPipeC, buff, nRead, &nWrite, NULL);

                CloseHandle(hPipeC);
            }
        }
    }

    return 0;
}

void ErrorExit(PTSTR lpszFunction)

// Format a readable error message, display a message box,
// and exit from the application.
{
    /*
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    */

    printf(lpszFunction);
    ExitProcess(1);
}
