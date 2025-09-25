#ifndef WINSOCK_POSIX_COMPAT_H
#define WINSOCK_POSIX_COMPAT_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")

    typedef SOCKET sock_t;
    #define closesocket(s) closesocket(s)
    #define THREAD_RET_TYPE DWORD WINAPI
    #define THREAD_ARG_TYPE LPVOID
    #define THREAD_FUNC_CAST(f) f
    #define CREATE_RECEIVE_THREAD(s) CreateThread(NULL, 0, recvThread, (LPVOID)s, 0, NULL)

#else
    #include <unistd.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <pthread.h>

    typedef int sock_t;
    #define closesocket(s) close(s)
    #define THREAD_RET_TYPE void*
    #define THREAD_ARG_TYPE void*
    #define THREAD_FUNC_CAST(f) (void* (*)(void*))f
    #define CREATE_RECEIVE_THREAD(s) pthread_t recv_thread; pthread_create(&recv_thread, NULL, THREAD_FUNC_CAST(recvThread), (void*)s)

#endif

#endif