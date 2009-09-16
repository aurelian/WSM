// A sample Service for testing WSM php extension.
// Based on http://www.codersource.net/win32_nt_service.html
//
// More on writing a Win32NT service can be found here:
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dllproc/base/using_services.asp
//
// To compile this file using win32 gcc compiler use this command:
//      g++ -o wsm.exe service.cpp
// You can get gcc compiler for windows from here: http://www.mingw.org/
// For instructions with Visual Studio C Compiler use the instructions provided on the 
// codesource.net web page.
// 
// wsm.exe will create a file with the name wsm.txt on c:\
// The text "wsm Service Created this file\n" will be added 
// on this file when the service will start
// 
// $Id: service.cpp,v 1.1 2005/08/25 08:51:01 aurelian Exp $
// 

#include <stdio.h>
#include <windows.h>
#include <tchar.h>

TCHAR *gszServiceName = TEXT("wsm");
SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle = 0;
HANDLE ServiceControlEvent = 0;
FILE *g_Handle;
char gszFilePath[] = "C:\\wsm.txt";


void WINAPI ServiceControlHandler( DWORD controlCode )
{
    switch ( controlCode ) {
        case SERVICE_CONTROL_INTERROGATE:
            break;
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            SetServiceStatus( serviceStatusHandle, &serviceStatus );
            SetEvent( ServiceControlEvent );
            return;
        case SERVICE_CONTROL_PAUSE:
            break;
        case SERVICE_CONTROL_CONTINUE:
            break;
        default:
            if ( controlCode >= 128 && controlCode <= 255 ) {
                // user defined control code
                break;
            } else {
                // unrecognised control code
                break;
            }
        }
    SetServiceStatus( serviceStatusHandle, &serviceStatus );
}

void WINAPI ServiceMain( DWORD /*argc*/, TCHAR* /*argv*/[] ) {
    // initialise service status
    serviceStatus.dwServiceType = SERVICE_WIN32;
    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    serviceStatus.dwControlsAccepted = 0;
    serviceStatus.dwWin32ExitCode = NO_ERROR;
    serviceStatus.dwServiceSpecificExitCode = NO_ERROR;
    serviceStatus.dwCheckPoint = 0;
    serviceStatus.dwWaitHint = 0;

    serviceStatusHandle = RegisterServiceCtrlHandler(gszServiceName, ServiceControlHandler);

    if (serviceStatusHandle) {
        // service is starting
        serviceStatus.dwCurrentState = SERVICE_START_PENDING;
        SetServiceStatus( serviceStatusHandle, &serviceStatus );

        // Create the Controlling Event here
        ServiceControlEvent = CreateEvent( 0, FALSE, FALSE, 0 );

        // Service running
        serviceStatus.dwControlsAccepted |= (SERVICE_ACCEPT_STOP |
        SERVICE_ACCEPT_SHUTDOWN);
        serviceStatus.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( serviceStatusHandle, &serviceStatus );
        fprintf(g_Handle,"wsm Service Created this file\n");
        fflush(g_Handle);

        WaitForSingleObject( ServiceControlEvent, INFINITE );

        // service was stopped
        serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus( serviceStatusHandle, &serviceStatus );

        // do cleanup here
        fclose(g_Handle);
        CloseHandle( ServiceControlEvent );
        ServiceControlEvent = 0;

        // service is now stopped
        serviceStatus.dwControlsAccepted &= ~(SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN);
        serviceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus( serviceStatusHandle, &serviceStatus );
    }
}

void RunService() {
    SERVICE_TABLE_ENTRY serviceTable[] =
    {
        { gszServiceName, ServiceMain },
        { 0, 0 }
    };
    StartServiceCtrlDispatcher( serviceTable );
}

int _tmain( int argc, TCHAR* argv[] ) {
    if((g_Handle = fopen(gszFilePath, "w")) != NULL) {
        RunService();
    } else {
        exit(1);
    }
    return 0;
}

