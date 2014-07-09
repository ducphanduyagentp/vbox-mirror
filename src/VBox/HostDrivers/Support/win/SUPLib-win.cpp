/* $Id$ */
/** @file
 * VirtualBox Support Library - Windows NT specific parts.
 */

/*
 * Copyright (C) 2006-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_SUP
#ifdef IN_SUP_HARDENED_R3
# undef DEBUG /* Warning: disables RT_STRICT */
# undef LOG_DISABLED
# define LOG_DISABLED
  /** @todo RTLOGREL_DISABLED */
# include <iprt/log.h>
# undef LogRelIt
# define LogRelIt(pvInst, fFlags, iGroup, fmtargs) do { } while (0)
#endif

#define USE_NT_DEVICE_IO_CONTROL_FILE
#include <iprt/nt/nt-and-windows.h>

#include <VBox/sup.h>
#include <VBox/types.h>
#include <VBox/err.h>
#include <VBox/param.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include "../SUPLibInternal.h"
#include "../SUPDrvIOC.h"
#ifdef VBOX_WITH_HARDENING
# include "win/SUPHardenedVerify-win.h"
#endif


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** The support service name. */
#define SERVICE_NAME    "VBoxDrv"


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#ifndef IN_SUP_HARDENED_R3
static int suplibOsCreateService(void);
//unused: static int suplibOsUpdateService(void);
static int suplibOsDeleteService(void);
static int suplibOsStartService(void);
static int suplibOsStopService(void);
#endif
#ifdef USE_NT_DEVICE_IO_CONTROL_FILE
static int suplibConvertNtStatus(NTSTATUS rcNt);
#else
static int suplibConvertWin32Err(int);
#endif


int suplibOsInit(PSUPLIBDATA pThis, bool fPreInited, bool fUnrestricted)
{
    /*
     * Almost nothing to do if pre-inited.
     */
    if (fPreInited)
    {
#if defined(VBOX_WITH_HARDENING) && !defined(IN_SUP_HARDENED_R3)
# ifdef IN_SUP_R3_STATIC
        return VERR_NOT_SUPPORTED;
# else
        supR3HardenedWinInitVersion();
        return supHardenedWinInitImageVerifier(NULL);
# endif
#else
        return VINF_SUCCESS;
#endif
    }

    /*
     * Try open the device.
     */
#ifndef IN_SUP_HARDENED_R3
    uint32_t cTry = 0;
#endif
    HANDLE hDevice;
    for (;;)
    {
        IO_STATUS_BLOCK     Ios   = RTNT_IO_STATUS_BLOCK_INITIALIZER;

        static const WCHAR  s_wszName[] = L"\\Device\\VBoxDrvU";
        UNICODE_STRING      NtName;
        NtName.Buffer        = (PWSTR)s_wszName;
        NtName.Length        = sizeof(s_wszName) - sizeof(WCHAR) * (fUnrestricted ? 2 : 1);
        NtName.MaximumLength = NtName.Length;

        OBJECT_ATTRIBUTES   ObjAttr;
        InitializeObjectAttributes(&ObjAttr, &NtName, OBJ_CASE_INSENSITIVE, NULL /*hRootDir*/, NULL /*pSecDesc*/);

        hDevice = RTNT_INVALID_HANDLE_VALUE;

        NTSTATUS rcNt = NtCreateFile(&hDevice,
                                     GENERIC_READ | GENERIC_WRITE,
                                     &ObjAttr,
                                     &Ios,
                                     NULL /* Allocation Size*/,
                                     FILE_ATTRIBUTE_NORMAL,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_OPEN,
                                     FILE_NON_DIRECTORY_FILE,
                                     NULL /*EaBuffer*/,
                                     0 /*EaLength*/);
        if (NT_SUCCESS(rcNt))
            rcNt = Ios.Status;
        if (!NT_SUCCESS(rcNt))
        {
#ifndef IN_SUP_HARDENED_R3
            /*
             * Failed to open, try starting the service and reopen the device
             * exactly once.
             */
            if (cTry == 0 && !NT_SUCCESS(rcNt))
            {
                cTry++;
                suplibOsStartService();
                continue;
            }
#endif
            switch (rcNt)
            {
                /** @todo someone must test what is actually returned. */
                case STATUS_DEVICE_DOES_NOT_EXIST:
                case STATUS_DEVICE_NOT_CONNECTED:
                //case ERROR_BAD_DEVICE:
                case STATUS_DEVICE_REMOVED:
                //case ERROR_DEVICE_NOT_AVAILABLE:
                    return VERR_VM_DRIVER_LOAD_ERROR;
                case STATUS_OBJECT_PATH_NOT_FOUND:
                case STATUS_NO_SUCH_DEVICE:
                case STATUS_NO_SUCH_FILE:
                case STATUS_OBJECT_NAME_NOT_FOUND:
                    return VERR_VM_DRIVER_NOT_INSTALLED;
                case STATUS_ACCESS_DENIED:
                case STATUS_SHARING_VIOLATION:
                    return VERR_VM_DRIVER_NOT_ACCESSIBLE;
                case STATUS_UNSUCCESSFUL:
                    return VERR_SUPLIB_NT_PROCESS_UNTRUSTED_0;
                case STATUS_TRUST_FAILURE:
                    return VERR_SUPLIB_NT_PROCESS_UNTRUSTED_1;
                case STATUS_TOO_LATE:
                    return VERR_SUPDRV_HARDENING_EVIL_HANDLE;
                default:
                    if (SUP_NT_STATUS_IS_VBOX(rcNt)) /* See VBoxDrvNtErr2NtStatus. */
                        return SUP_NT_STATUS_TO_VBOX(rcNt);
                    return VERR_VM_DRIVER_OPEN_ERROR;
            }
        }
        break;
    }

    /*
     * We're done.
     */
    pThis->hDevice       = hDevice;
    pThis->fUnrestricted = fUnrestricted;
    return VINF_SUCCESS;
}

#ifndef IN_SUP_HARDENED_R3

int suplibOsInstall(void)
{
    return suplibOsCreateService();
}


int suplibOsUninstall(void)
{
    int rc = suplibOsStopService();
    if (!rc)
        rc = suplibOsDeleteService();
    return rc;
}


/**
 * Creates the service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
static int suplibOsCreateService(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    SC_HANDLE   hSMgrCreate = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgrCreate, ("OpenSCManager(,,create) failed rc=%d\n", LastError));
    if (hSMgrCreate)
    {
        char szDriver[RTPATH_MAX];
        int rc = RTPathExecDir(szDriver, sizeof(szDriver) - sizeof("\\VBoxDrv.sys"));
        if (RT_SUCCESS(rc))
        {
            strcat(szDriver, "\\VBoxDrv.sys");
            SC_HANDLE hService = CreateService(hSMgrCreate,
                                               SERVICE_NAME,
                                               "VBox Support Driver",
                                               SERVICE_QUERY_STATUS,
                                               SERVICE_KERNEL_DRIVER,
                                               SERVICE_DEMAND_START,
                                               SERVICE_ERROR_NORMAL,
                                               szDriver,
                                               NULL, NULL, NULL, NULL, NULL);
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsg(hService, ("CreateService failed! LastError=%Rwa szDriver=%s\n", LastError, szDriver));
            CloseServiceHandle(hService);
            CloseServiceHandle(hSMgrCreate);
            return hService ? 0 : -1;
        }
        CloseServiceHandle(hSMgrCreate);
        return rc;
    }
    return -1;
}


/**
 * Stops a possibly running service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
static int suplibOsStopService(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    int rc = -1;
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_STOP | SERVICE_QUERY_STATUS);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgr, ("OpenSCManager(,,delete) failed rc=%d\n", LastError));
    if (hSMgr)
    {
        SC_HANDLE hService = OpenService(hSMgr, SERVICE_NAME, SERVICE_STOP | SERVICE_QUERY_STATUS);
        if (hService)
        {
            /*
             * Stop the service.
             */
            SERVICE_STATUS  Status;
            QueryServiceStatus(hService, &Status);
            if (Status.dwCurrentState == SERVICE_STOPPED)
                rc = 0;
            else if (ControlService(hService, SERVICE_CONTROL_STOP, &Status))
            {
                int iWait = 100;
                while (Status.dwCurrentState == SERVICE_STOP_PENDING && iWait-- > 0)
                {
                    Sleep(100);
                    QueryServiceStatus(hService, &Status);
                }
                if (Status.dwCurrentState == SERVICE_STOPPED)
                    rc = 0;
                else
                   AssertMsgFailed(("Failed to stop service. status=%d\n", Status.dwCurrentState));
            }
            else
            {
                DWORD LastError = GetLastError(); NOREF(LastError);
                AssertMsgFailed(("ControlService failed with LastError=%Rwa. status=%d\n", LastError, Status.dwCurrentState));
            }
            CloseServiceHandle(hService);
        }
        else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            rc = 0;
        else
        {
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsgFailed(("OpenService failed LastError=%Rwa\n", LastError));
        }
        CloseServiceHandle(hSMgr);
    }
    return rc;
}


/**
 * Deletes the service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
int suplibOsDeleteService(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    int rc = -1;
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgr, ("OpenSCManager(,,delete) failed rc=%d\n", LastError));
    if (hSMgr)
    {
        SC_HANDLE hService = OpenService(hSMgr, SERVICE_NAME, DELETE);
        if (hService)
        {
            /*
             * Delete the service.
             */
            if (DeleteService(hService))
                rc = 0;
            else
            {
                DWORD LastError = GetLastError(); NOREF(LastError);
                AssertMsgFailed(("DeleteService failed LastError=%Rwa\n", LastError));
            }
            CloseServiceHandle(hService);
        }
        else if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
            rc = 0;
        else
        {
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsgFailed(("OpenService failed LastError=%Rwa\n", LastError));
        }
        CloseServiceHandle(hSMgr);
    }
    return rc;
}

#if 0
/**
 * Creates the service.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 */
static int suplibOsUpdateService(void)
{
    /*
     * Assume it didn't exist, so we'll create the service.
     */
    SC_HANDLE   hSMgr = OpenSCManager(NULL, NULL, SERVICE_CHANGE_CONFIG);
    DWORD LastError = GetLastError(); NOREF(LastError);
    AssertMsg(hSMgr, ("OpenSCManager(,,delete) failed LastError=%Rwa\n", LastError));
    if (hSMgr)
    {
        SC_HANDLE hService = OpenService(hSMgr, SERVICE_NAME, SERVICE_CHANGE_CONFIG);
        if (hService)
        {
            char szDriver[RTPATH_MAX];
            int rc = RTPathExecDir(szDriver, sizeof(szDriver) - sizeof("\\VBoxDrv.sys"));
            if (RT_SUCCESS(rc))
            {
                strcat(szDriver, "\\VBoxDrv.sys");

                SC_LOCK hLock = LockServiceDatabase(hSMgr);
                if (ChangeServiceConfig(hService,
                                        SERVICE_KERNEL_DRIVER,
                                        SERVICE_DEMAND_START,
                                        SERVICE_ERROR_NORMAL,
                                        szDriver,
                                        NULL, NULL, NULL, NULL, NULL, NULL))
                {

                    UnlockServiceDatabase(hLock);
                    CloseServiceHandle(hService);
                    CloseServiceHandle(hSMgr);
                    return 0;
                }
                else
                {
                    DWORD LastError = GetLastError(); NOREF(LastError);
                    AssertMsgFailed(("ChangeServiceConfig failed LastError=%Rwa\n", LastError));
                }
            }
            UnlockServiceDatabase(hLock);
            CloseServiceHandle(hService);
        }
        else
        {
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsgFailed(("OpenService failed LastError=%Rwa\n", LastError));
        }
        CloseServiceHandle(hSMgr);
    }
    return -1;
}
#endif


/**
 * Attempts to start the service, creating it if necessary.
 *
 * @returns 0 on success.
 * @returns -1 on failure.
 * @param   fRetry  Indicates retry call.
 */
static int suplibOsStartService(void)
{
    /*
     * Check if the driver service is there.
     */
    SC_HANDLE hSMgr = OpenSCManager(NULL, NULL, SERVICE_QUERY_STATUS | SERVICE_START);
    if (hSMgr == NULL)
    {
        AssertMsgFailed(("couldn't open service manager in SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS mode!\n"));
        return -1;
    }

    /*
     * Try open our service to check it's status.
     */
    SC_HANDLE hService = OpenService(hSMgr, SERVICE_NAME, SERVICE_QUERY_STATUS | SERVICE_START);
    if (!hService)
    {
        /*
         * Create the service.
         */
        int rc = suplibOsCreateService();
        if (rc)
            return rc;

        /*
         * Try open the service.
         */
        hService = OpenService(hSMgr, SERVICE_NAME, SERVICE_QUERY_STATUS | SERVICE_START);
    }

    /*
     * Check if open and on demand create succeeded.
     */
    int rc = -1;
    if (hService)
    {

        /*
         * Query service status to see if we need to start it or not.
         */
        SERVICE_STATUS  Status;
        BOOL fRc = QueryServiceStatus(hService, &Status);
        Assert(fRc);
        if (    Status.dwCurrentState != SERVICE_RUNNING
            &&  Status.dwCurrentState != SERVICE_START_PENDING)
        {
            /*
             * Start it.
             */
            fRc = StartService(hService, 0, NULL);
            DWORD LastError = GetLastError(); NOREF(LastError);
#ifndef DEBUG_bird
            AssertMsg(fRc, ("StartService failed with LastError=%Rwa\n", LastError));
#endif
        }

        /*
         * Wait for the service to finish starting.
         * We'll wait for 10 seconds then we'll give up.
         */
        QueryServiceStatus(hService, &Status);
        if (Status.dwCurrentState == SERVICE_START_PENDING)
        {
            int iWait;
            for (iWait = 100; iWait > 0 && Status.dwCurrentState == SERVICE_START_PENDING; iWait--)
            {
                Sleep(100);
                QueryServiceStatus(hService, &Status);
            }
            DWORD LastError = GetLastError(); NOREF(LastError);
            AssertMsg(Status.dwCurrentState != SERVICE_RUNNING,
                      ("Failed to start. LastError=%Rwa iWait=%d status=%d\n",
                       LastError, iWait, Status.dwCurrentState));
        }

        if (Status.dwCurrentState == SERVICE_RUNNING)
            rc = 0;

        /*
         * Close open handles.
         */
        CloseServiceHandle(hService);
    }
    else
    {
        DWORD LastError = GetLastError(); NOREF(LastError);
        AssertMsgFailed(("OpenService failed! LastError=%Rwa\n", LastError));
    }
    if (!CloseServiceHandle(hSMgr))
        AssertFailed();

    return rc;
}


int suplibOsTerm(PSUPLIBDATA pThis)
{
    /*
     * Check if we're inited at all.
     */
    if (pThis->hDevice != NULL)
    {
        if (!CloseHandle((HANDLE)pThis->hDevice))
            AssertFailed();
        pThis->hDevice = NIL_RTFILE; /* yes, that's right */
    }

    return VINF_SUCCESS;
}


int suplibOsIOCtl(PSUPLIBDATA pThis, uintptr_t uFunction, void *pvReq, size_t cbReq)
{
    /*
     * Issue the device I/O control.
     */
    PSUPREQHDR pHdr = (PSUPREQHDR)pvReq;
    Assert(cbReq == RT_MAX(pHdr->cbIn, pHdr->cbOut));
# ifdef USE_NT_DEVICE_IO_CONTROL_FILE
    IO_STATUS_BLOCK Ios;
    Ios.Status = -1;
    Ios.Information = 0;
    NTSTATUS rcNt = NtDeviceIoControlFile((HANDLE)pThis->hDevice, NULL /*hEvent*/, NULL /*pfnApc*/, NULL /*pvApcCtx*/, &Ios,
                                          (ULONG)uFunction,
                                          pvReq /*pvInput */, pHdr->cbIn /* cbInput */,
                                          pvReq /*pvOutput*/, pHdr->cbOut /* cbOutput */);
    if (NT_SUCCESS(rcNt))
    {
        if (NT_SUCCESS(Ios.Status))
            return VINF_SUCCESS;
        rcNt = Ios.Status;
    }
    return suplibConvertNtStatus(rcNt);

# else
    DWORD cbReturned = (ULONG)pHdr->cbOut;
    if (DeviceIoControl((HANDLE)pThis->hDevice, uFunction, pvReq, pHdr->cbIn, pvReq, cbReturned, &cbReturned, NULL))
        return 0;
    return suplibConvertWin32Err(GetLastError());
# endif
}


int suplibOsIOCtlFast(PSUPLIBDATA pThis, uintptr_t uFunction, uintptr_t idCpu)
{
    /*
     * Issue device I/O control.
     */
# ifdef USE_NT_DEVICE_IO_CONTROL_FILE
    IO_STATUS_BLOCK Ios;
    Ios.Status = -1;
    Ios.Information = 0;
    NTSTATUS rcNt = NtDeviceIoControlFile((HANDLE)pThis->hDevice, NULL /*hEvent*/, NULL /*pfnApc*/, NULL /*pvApcCtx*/, &Ios,
                                          (ULONG)uFunction,
                                          NULL /*pvInput */, 0 /* cbInput */,
                                          (PVOID)idCpu /*pvOutput*/, 0 /* cbOutput */);
    if (NT_SUCCESS(rcNt))
    {
        if (NT_SUCCESS(Ios.Status))
            return VINF_SUCCESS;
        rcNt = Ios.Status;
    }
    return suplibConvertNtStatus(rcNt);
# else
    DWORD cbReturned = 0;
    if (DeviceIoControl((HANDLE)pThis->hDevice, uFunction, NULL, 0, (LPVOID)idCpu, 0, &cbReturned, NULL))
        return VINF_SUCCESS;
    return suplibConvertWin32Err(GetLastError());
# endif
}


int suplibOsPageAlloc(PSUPLIBDATA pThis, size_t cPages, void **ppvPages)
{
    NOREF(pThis);
    *ppvPages = VirtualAlloc(NULL, (size_t)cPages << PAGE_SHIFT, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (*ppvPages)
        return VINF_SUCCESS;
    return RTErrConvertFromWin32(GetLastError());
}


int suplibOsPageFree(PSUPLIBDATA pThis, void *pvPages, size_t /* cPages */)
{
    NOREF(pThis);
    if (VirtualFree(pvPages, 0, MEM_RELEASE))
        return VINF_SUCCESS;
    return RTErrConvertFromWin32(GetLastError());
}


# ifndef USE_NT_DEVICE_IO_CONTROL_FILE
/**
 * Converts a supdrv win32 error code to an IPRT status code.
 *
 * @returns corresponding IPRT error code.
 * @param   rc  Win32 error code.
 */
static int suplibConvertWin32Err(int rc)
{
    /* Conversion program (link with ntdll.lib from ddk):
        #define _WIN32_WINNT 0x0501
        #include <windows.h>
        #include <ntstatus.h>
        #include <winternl.h>
        #include <stdio.h>

        int main()
        {
            #define CONVERT(a)  printf(#a " %#x -> %d\n", a, RtlNtStatusToDosError((a)))
            CONVERT(STATUS_SUCCESS);
            CONVERT(STATUS_NOT_SUPPORTED);
            CONVERT(STATUS_INVALID_PARAMETER);
            CONVERT(STATUS_UNKNOWN_REVISION);
            CONVERT(STATUS_INVALID_HANDLE);
            CONVERT(STATUS_INVALID_ADDRESS);
            CONVERT(STATUS_NOT_LOCKED);
            CONVERT(STATUS_IMAGE_ALREADY_LOADED);
            CONVERT(STATUS_ACCESS_DENIED);
            CONVERT(STATUS_REVISION_MISMATCH);

            return 0;
        }
     */

    switch (rc)
    {
        //case 0:                             return STATUS_SUCCESS;
        case 0:                             return VINF_SUCCESS;
        case ERROR_NOT_SUPPORTED:           return VERR_GENERAL_FAILURE;
        case ERROR_INVALID_PARAMETER:       return VERR_INVALID_PARAMETER;
        case ERROR_UNKNOWN_REVISION:        return VERR_INVALID_MAGIC;
        case ERROR_INVALID_HANDLE:          return VERR_INVALID_HANDLE;
        case ERROR_UNEXP_NET_ERR:           return VERR_INVALID_POINTER;
        case ERROR_NOT_LOCKED:              return VERR_LOCK_FAILED;
        case ERROR_SERVICE_ALREADY_RUNNING: return VERR_ALREADY_LOADED;
        case ERROR_ACCESS_DENIED:           return VERR_PERMISSION_DENIED;
        case ERROR_REVISION_MISMATCH:       return VERR_VERSION_MISMATCH;
    }

    /* fall back on the default conversion. */
    return RTErrConvertFromWin32(rc);
}
# else
/**
 * Reverse of VBoxDrvNtErr2NtStatus
 * returns VBox status code.
 * @param   rcNt    NT status code.
 */
static int suplibConvertNtStatus(NTSTATUS rcNt)
{
    switch (rcNt)
    {
        case STATUS_SUCCESS:                    return VINF_SUCCESS;
        case STATUS_NOT_SUPPORTED:              return VERR_GENERAL_FAILURE;
        case STATUS_INVALID_PARAMETER:          return VERR_INVALID_PARAMETER;
        case STATUS_UNKNOWN_REVISION:           return VERR_INVALID_MAGIC;
        case STATUS_INVALID_HANDLE:             return VERR_INVALID_HANDLE;
        case STATUS_INVALID_ADDRESS:            return VERR_INVALID_POINTER;
        case STATUS_NOT_LOCKED:                 return VERR_LOCK_FAILED;
        case STATUS_IMAGE_ALREADY_LOADED:       return VERR_ALREADY_LOADED;
        case STATUS_ACCESS_DENIED:              return VERR_PERMISSION_DENIED;
        case STATUS_REVISION_MISMATCH:          return VERR_VERSION_MISMATCH;
    }

    /* See VBoxDrvNtErr2NtStatus. */
    if (SUP_NT_STATUS_IS_VBOX(rcNt))
        return SUP_NT_STATUS_TO_VBOX(rcNt);

    /* Fall back on IPRT for the rest. */
    return RTErrConvertFromNtStatus(rcNt);
}
# endif

#endif /* !IN_SUP_HARDENED_R3 */

