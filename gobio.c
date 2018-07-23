#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <mswsock.h>  
    #include <windows.h>
#endif

// Need to link with Ws2_32.lib
// #pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")
// gcc <source>.c -o <executable> -lwsock32 -lws2_32

#define BUILD_VERSION "1.0.0-beta"
#define MAX_BUFF_SIZE 384

int g_ThreadCount;  

HANDLE g_hIOCP = INVALID_HANDLE_VALUE;  
SOCKET g_ServerSocket = INVALID_SOCKET;  

typedef enum {
    IO_READ,
    IO_WRITE,
    IO_CLOSE
} IO_OPERATION;  

typedef struct IO_DATA {  
    WSAOVERLAPPED   Overlapped;
    char            Buffer[MAX_BUFF_SIZE];
    WSABUF          WSABuf;  
    int             nTotalBytes;  
    int             nSentBytes;  
    IO_OPERATION    opCode;  
    SOCKET          activeSocket;  
} IO_DATA;  

DWORD WINAPI WorkerThread (LPVOID WorkThreadContext) {
    LPWSAOVERLAPPED lpOverlapped = NULL;  
    IO_DATA *lpIOContext = NULL;   

    DWORD dwRecvNumBytes = 0;  
    DWORD dwSendNumBytes = 0;  
    DWORD dwFlags = 0;  
    DWORD dwIoSize = 0;  
    BOOL bSuccess = FALSE;  

    int nRet = 0;  

        while (1)
        {  
            void *lpCompletionKey = NULL;  
            bSuccess = GetQueuedCompletionStatus(g_hIOCP,
                                                &dwIoSize,  
                                                (PULONG_PTR)&lpCompletionKey,  
                                                (LPOVERLAPPED *)&lpOverlapped,   
                                                INFINITE);  
            if (!bSuccess) {
                printf("GetQueuedCompletionStatus() error diagnosis %s\n", GetLastError());  
                break;  
            }  

            lpIOContext = (IO_DATA *)lpOverlapped;  

            if (lpIOContext->opCode == IO_READ) // A read operation already complete, write next
            {  
                lpIOContext->nTotalBytes  = lpIOContext->WSABuf.len;  
                lpIOContext->nSentBytes   = 0;
                lpIOContext->opCode = IO_CLOSE;  
                dwFlags = 0;

                // ZeroMemory(lpIOContext->WSABuf.buf, sizeof(lpIOContext->WSABuf.buf));

                strcpy(lpIOContext->WSABuf.buf ,"\
HTTP/1.1 200 OK\r\n\
Content-type: text/html\r\n\
Connection: keep-alive\r\n\
Content-Length: 25\r\n\
\r\n\
<h1>Welcome to Gobio</h1>");

                // printf("%s\n%i\n\n", lpIOContext->WSABuf.buf, lpIOContext->WSABuf.len);

                nRet = WSASend(  
                            lpIOContext->activeSocket,  
                            &lpIOContext->WSABuf,
                            1,
                            &dwSendNumBytes,  
                            dwFlags,  
                            &(lpIOContext->Overlapped),
                            NULL);

                if (nRet == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
                        printf("WASSend() error diagnosis %s\n", WSAGetLastError());  
                        closesocket(lpIOContext->activeSocket);  
                        free(lpIOContext);  
                        continue;  
                }  
        } else if (lpIOContext->opCode == IO_CLOSE)  {
            // printf("Connection ended\n");
			// Clean up all the WSASend/WSARecv communication and makes the engine ready for a new one
			if (shutdown(lpIOContext->activeSocket, SD_BOTH) != 0)
			    printf("\nshutdown() error diagnosis %s\n", WSAGetLastError());

            closesocket(lpIOContext->activeSocket);  
            free(lpIOContext);  
            continue;  

        } 
    }  
    return 0;  
}

int RunGobio () {
    { // Initialize winsock2 library  
        WSADATA WSAData;  
        ZeroMemory(&WSAData, sizeof(WSADATA));  
        int retVal = -1;  
        if ((retVal = WSAStartup(MAKEWORD(2, 2), &WSAData)) != 0) {  
            printf("WSAStartup() error code %s\n", retVal);
        }  
    }  
    {  // Create WSASocket instance
        g_ServerSocket = WSASocket(
                                    AF_INET,
                                    SOCK_STREAM,
                                    IPPROTO_TCP,
                                    NULL,
                                    0,
                                    WSA_FLAG_OVERLAPPED);  
        if (g_ServerSocket == INVALID_SOCKET) {  
            printf("WSASocket() error diagnosis %s\n", WSAGetLastError());
        }
    }  
    {   // Binding
        struct sockaddr_in service;  
        service.sin_family = AF_INET;  
        service.sin_addr.s_addr = htonl(INADDR_ANY);
        service.sin_port = htons(7070);
        int retVal = bind(
                        g_ServerSocket,
                        (SOCKADDR *)&service,
                        sizeof(service));
        if (retVal == SOCKET_ERROR) {  
            printf("bind() error diagnosis %s\n", WSAGetLastError());    
        }  
    }  
    {   // Listening
        int retVal = listen(g_ServerSocket, 8);  
        if (retVal == SOCKET_ERROR) {
            printf("listen() error diagnosis %s\n", WSAGetLastError());
        }  
    }  
    {   // Create IOCP  
        SYSTEM_INFO sysInfo;  
        ZeroMemory(&sysInfo, sizeof(SYSTEM_INFO));  
        GetSystemInfo(&sysInfo);  
        g_ThreadCount = sysInfo.dwNumberOfProcessors * 2;  
        g_hIOCP = CreateIoCompletionPort(
                                        INVALID_HANDLE_VALUE,
                                        NULL,
                                        0,
                                        g_ThreadCount);  
        if (g_hIOCP == NULL) {  
            printf("CreateIoCompletionPort() error diagnosis %s\n", GetLastError());    
        }  
        if (CreateIoCompletionPort((HANDLE)g_ServerSocket, g_hIOCP, 0, 0) == NULL) {  
            printf("CreateIoCompletionPort() binding error diagnosis %s\n", GetLastError()); 
        }  
    }  
    {  // Create worker threads  
        for (DWORD dwThread = 0; dwThread < g_ThreadCount; dwThread++)  
        {  
            HANDLE  hThread;  
            DWORD   dwThreadId;  
            hThread = CreateThread(
                                NULL,
                                0,
                                WorkerThread,
                                0,
                                0,
                                &dwThreadId);  
            // CloseHandle(hThread); 
        }  
    }  
    { // Accepting new connections  
        while(1)  
        {  
            SOCKET ls = accept(
                            g_ServerSocket,
                            NULL,
                            NULL);  
            if (ls == SOCKET_ERROR) break;
            // printf("Connection accepted\n");
            { // Disables buffer to improve performance  
                int nZero = 0;  
                setsockopt(
                        ls,
                        SOL_SOCKET,
                        SO_SNDBUF,
                        (char *)&nZero,
                        sizeof(nZero));
            }
            if (CreateIoCompletionPort((HANDLE)ls, g_hIOCP, 0, 0) == NULL) {
                printf("CreateIoCompletionPort() binding error diagnosis %s\n", GetLastError());  
                closesocket(ls);
            }
            else { // Issues a WSARecv request  
                IO_DATA *data = (IO_DATA *)malloc(sizeof(struct IO_DATA));  
                ZeroMemory(&data->Overlapped, sizeof(data->Overlapped));
                ZeroMemory(data->Buffer, sizeof(data->Buffer));
                data->opCode       = IO_READ;  
                data->nTotalBytes  = 0;  
                data->nSentBytes   = 0;
                data->WSABuf.buf   = data->Buffer;
                data->WSABuf.len   = sizeof(data->Buffer);  
                data->activeSocket = ls;  
                DWORD dwRecvNumBytes = 0, dwFlags = 0;  
                int nRet = WSARecv(
                                ls,
                                &data->WSABuf,
                                1,
                                &dwRecvNumBytes,  
                                &dwFlags,  
                                &data->Overlapped,
                                NULL);
                if (nRet == SOCKET_ERROR  && (ERROR_IO_PENDING != WSAGetLastError())) {  
                    printf("WASRecv() error diagnosis %s\n", WSAGetLastError());  
                    closesocket(ls);  
                    free(data);  
                }  
            }  
        }  
    }  
    closesocket(g_ServerSocket);  
    WSACleanup();  
    return 0;
}

int main (int argc, char *argv[]) {
    printf("127.0.0.1:7070\n");
    RunGobio();
}