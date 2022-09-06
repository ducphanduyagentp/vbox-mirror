/* $Id$ */
/** @file
 * VirtualBox Main - Guest session tasks.
 */

/*
 * Copyright (C) 2012-2022 Oracle and/or its affiliates.
 *
 * This file is part of VirtualBox base platform packages, as
 * available from https://www.virtualbox.org.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, in version 3 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses>.
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_MAIN_GUESTSESSION
#include "LoggingNew.h"

#include "GuestImpl.h"
#ifndef VBOX_WITH_GUEST_CONTROL
# error "VBOX_WITH_GUEST_CONTROL must defined in this file"
#endif
#include "GuestSessionImpl.h"
#include "GuestSessionImplTasks.h"
#include "GuestCtrlImplPrivate.h"

#include "Global.h"
#include "AutoCaller.h"
#include "ConsoleImpl.h"
#include "ProgressImpl.h"

#include <memory> /* For auto_ptr. */

#include <iprt/env.h>
#include <iprt/file.h> /* For CopyTo/From. */
#include <iprt/dir.h>
#include <iprt/path.h>
#include <iprt/fsvfs.h>


/*********************************************************************************************************************************
*   Defines                                                                                                                      *
*********************************************************************************************************************************/

/**
 * (Guest Additions) ISO file flags.
 * Needed for handling Guest Additions updates.
 */
#define ISOFILE_FLAG_NONE                0
/** Copy over the file from host to the
 *  guest. */
#define ISOFILE_FLAG_COPY_FROM_ISO       RT_BIT(0)
/** Execute file on the guest after it has
 *  been successfully transfered. */
#define ISOFILE_FLAG_EXECUTE             RT_BIT(7)
/** File is optional, does not have to be
 *  existent on the .ISO. */
#define ISOFILE_FLAG_OPTIONAL            RT_BIT(8)


// session task classes
/////////////////////////////////////////////////////////////////////////////

GuestSessionTask::GuestSessionTask(GuestSession *pSession)
    : ThreadTask("GenericGuestSessionTask")
{
    mSession = pSession;

    switch (mSession->i_getPathStyle())
    {
        case PathStyle_DOS:
            mfPathStyle = RTPATH_STR_F_STYLE_DOS;
            mPathStyle  = "\\";
            break;

        default:
            mfPathStyle = RTPATH_STR_F_STYLE_UNIX;
            mPathStyle  = "/";
            break;
    }
}

GuestSessionTask::~GuestSessionTask(void)
{
}

/**
 * Creates (and initializes / sets) the progress objects of a guest session task.
 *
 * @returns VBox status code.
 * @param   cOperations         Number of operation the task wants to perform.
 */
int GuestSessionTask::createAndSetProgressObject(ULONG cOperations /* = 1 */)
{
    LogFlowThisFunc(("cOperations=%ld\n", cOperations));

    /* Create the progress object. */
    ComObjPtr<Progress> pProgress;
    HRESULT hr = pProgress.createObject();
    if (FAILED(hr))
        return VERR_COM_UNEXPECTED;

    hr = pProgress->init(static_cast<IGuestSession*>(mSession),
                         Bstr(mDesc).raw(),
                         TRUE /* aCancelable */, cOperations, Bstr(mDesc).raw());
    if (FAILED(hr))
        return VERR_COM_UNEXPECTED;

    mProgress = pProgress;

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

#if 0 /* unsed */
/** @note The task object is owned by the thread after this returns, regardless of the result.  */
int GuestSessionTask::RunAsync(const Utf8Str &strDesc, ComObjPtr<Progress> &pProgress)
{
    LogFlowThisFunc(("strDesc=%s\n", strDesc.c_str()));

    mDesc = strDesc;
    mProgress = pProgress;
    HRESULT hrc = createThreadWithType(RTTHREADTYPE_MAIN_HEAVY_WORKER);

    LogFlowThisFunc(("Returning hrc=%Rhrc\n", hrc));
    return Global::vboxStatusCodeToCOM(hrc);
}
#endif

/**
 * Gets a guest property from the VM.
 *
 * @returns VBox status code.
 * @param   pGuest              Guest object of VM to get guest property from.
 * @param   strPath             Guest property to path to get.
 * @param   strValue            Where to store the guest property value on success.
 */
int GuestSessionTask::getGuestProperty(const ComObjPtr<Guest> &pGuest,
                                       const Utf8Str &strPath, Utf8Str &strValue)
{
    ComObjPtr<Console> pConsole = pGuest->i_getConsole();
    const ComPtr<IMachine> pMachine = pConsole->i_machine();

    Assert(!pMachine.isNull());
    Bstr strTemp, strFlags;
    LONG64 i64Timestamp;
    HRESULT hr = pMachine->GetGuestProperty(Bstr(strPath).raw(),
                                            strTemp.asOutParam(),
                                            &i64Timestamp, strFlags.asOutParam());
    if (SUCCEEDED(hr))
    {
        strValue = strTemp;
        return VINF_SUCCESS;
    }
    return VERR_NOT_FOUND;
}

/**
 * Sets the percentage of a guest session task progress.
 *
 * @returns VBox status code.
 * @param   uPercent            Percentage (0-100) to set.
 */
int GuestSessionTask::setProgress(ULONG uPercent)
{
    if (mProgress.isNull()) /* Progress is optional. */
        return VINF_SUCCESS;

    BOOL fCanceled;
    if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
        && fCanceled)
        return VERR_CANCELLED;
    BOOL fCompleted;
    if (   SUCCEEDED(mProgress->COMGETTER(Completed(&fCompleted)))
        && fCompleted)
    {
        AssertMsgFailed(("Setting value of an already completed progress\n"));
        return VINF_SUCCESS;
    }
    HRESULT hr = mProgress->SetCurrentOperationProgress(uPercent);
    if (FAILED(hr))
        return VERR_COM_UNEXPECTED;

    return VINF_SUCCESS;
}

/**
 * Sets the task's progress object to succeeded.
 *
 * @returns VBox status code.
 */
int GuestSessionTask::setProgressSuccess(void)
{
    if (mProgress.isNull()) /* Progress is optional. */
        return VINF_SUCCESS;

    BOOL fCompleted;
    if (   SUCCEEDED(mProgress->COMGETTER(Completed(&fCompleted)))
        && !fCompleted)
    {
#ifdef VBOX_STRICT
        ULONG uCurOp; mProgress->COMGETTER(Operation(&uCurOp));
        ULONG cOps;   mProgress->COMGETTER(OperationCount(&cOps));
        AssertMsg(uCurOp + 1 /* Zero-based */ == cOps, ("Not all operations done yet (%u/%u)\n", uCurOp + 1, cOps));
#endif
        HRESULT hr = mProgress->i_notifyComplete(S_OK);
        if (FAILED(hr))
            return VERR_COM_UNEXPECTED; /** @todo Find a better rc. */
    }

    return VINF_SUCCESS;
}

/**
 * Sets the task's progress object to an error using a string message.
 *
 * @returns Returns \a hr for covenience.
 * @param   hr                  Progress operation result to set.
 * @param   strMsg              Message to set.
 */
HRESULT GuestSessionTask::setProgressErrorMsg(HRESULT hr, const Utf8Str &strMsg)
{
    LogFlowFunc(("hr=%Rhrc, strMsg=%s\n", hr, strMsg.c_str()));

    if (mProgress.isNull()) /* Progress is optional. */
        return hr; /* Return original rc. */

    BOOL fCanceled;
    BOOL fCompleted;
    if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
        && !fCanceled
        && SUCCEEDED(mProgress->COMGETTER(Completed(&fCompleted)))
        && !fCompleted)
    {
        HRESULT hr2 = mProgress->i_notifyComplete(hr,
                                                  COM_IIDOF(IGuestSession),
                                                  GuestSession::getStaticComponentName(),
                                                  /* Make sure to hand-in the message via format string to avoid problems
                                                   * with (file) paths which e.g. contain "%s" and friends. Can happen with
                                                   * randomly generated Validation Kit stuff. */
                                                  "%s", strMsg.c_str());
        if (FAILED(hr2))
            return hr2;
    }
    return hr; /* Return original rc. */
}

/**
 * Sets the task's progress object to an error using a string message and a guest error info object.
 *
 * @returns Returns \a hr for covenience.
 * @param   hr                  Progress operation result to set.
 * @param   strMsg              Message to set.
 * @param   guestErrorInfo      Guest error info to use.
 */
HRESULT GuestSessionTask::setProgressErrorMsg(HRESULT hr, const Utf8Str &strMsg, const GuestErrorInfo &guestErrorInfo)
{
    return setProgressErrorMsg(hr, strMsg + Utf8Str(": ") + GuestBase::getErrorAsString(guestErrorInfo));
}

/**
 * Creates a directory on the guest.
 *
 * @return VBox status code.
 *         VINF_ALREADY_EXISTS if directory on the guest already exists (\a fCanExist is \c true).
 *         VWRN_ALREADY_EXISTS if directory on the guest already exists but must not exist (\a fCanExist is \c false).
 * @param  strPath                  Absolute path to directory on the guest (guest style path) to create.
 * @param  enmDirectoryCreateFlags  Directory creation flags.
 * @param  fMode                    Directory mode to use for creation.
 * @param  fFollowSymlinks          Whether to follow symlinks on the guest or not.
 * @param  fCanExist                Whether the directory to create is allowed to exist already.
 */
int GuestSessionTask::directoryCreateOnGuest(const com::Utf8Str &strPath,
                                             DirectoryCreateFlag_T enmDirectoryCreateFlags, uint32_t fMode,
                                             bool fFollowSymlinks, bool fCanExist)
{
    LogFlowFunc(("strPath=%s, enmDirectoryCreateFlags=0x%x, fMode=%RU32, fFollowSymlinks=%RTbool, fCanExist=%RTbool\n",
                 strPath.c_str(), enmDirectoryCreateFlags, fMode, fFollowSymlinks, fCanExist));

    GuestFsObjData objData;
    int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    int vrc = mSession->i_directoryQueryInfo(strPath, fFollowSymlinks, objData, &vrcGuest);
    if (RT_SUCCESS(vrc))
    {
        if (!fCanExist)
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Guest directory \"%s\" already exists"), strPath.c_str()));
            vrc = VERR_ALREADY_EXISTS;
        }
        else
            vrc = VWRN_ALREADY_EXISTS;
    }
    else
    {
        switch (vrc)
        {
            case VERR_GSTCTL_GUEST_ERROR:
            {
                switch (vrcGuest)
                {
                    case VERR_FILE_NOT_FOUND:
                        RT_FALL_THROUGH();
                    case VERR_PATH_NOT_FOUND:
                        vrc = mSession->i_directoryCreate(strPath.c_str(), fMode, enmDirectoryCreateFlags, &vrcGuest);
                        break;
                    default:
                        break;
                }

                if (RT_FAILURE(vrc))
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(tr("Guest error creating directory \"%s\" on the guest: %Rrc"),
                                                   strPath.c_str(), vrcGuest));
                break;
            }

            default:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host error creating directory \"%s\" on the guest: %Rrc"),
                                               strPath.c_str(), vrc));
                break;
        }
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Creates a directory on the host.
 *
 * @return VBox status code. VERR_ALREADY_EXISTS if directory on the guest already exists.
 * @param  strPath                  Absolute path to directory on the host (host style path) to create.
 * @param  fCreate                  Directory creation flags.
 * @param  fMode                    Directory mode to use for creation.
 * @param  fCanExist                Whether the directory to create is allowed to exist already.
 */
int GuestSessionTask::directoryCreateOnHost(const com::Utf8Str &strPath, uint32_t fCreate, uint32_t fMode, bool fCanExist)
{
    LogFlowFunc(("strPath=%s, fCreate=0x%x, fMode=%RU32, fCanExist=%RTbool\n", strPath.c_str(), fCreate, fMode, fCanExist));

    int vrc = RTDirCreate(strPath.c_str(), fMode, fCreate);
    if (RT_FAILURE(vrc))
    {
        if (vrc == VERR_ALREADY_EXISTS)
        {
            if (!fCanExist)
            {
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host directory \"%s\" already exists"), strPath.c_str()));
            }
            else
                vrc = VINF_SUCCESS;
        }
        else
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Could not create host directory \"%s\": %Rrc"),
                                           strPath.c_str(), vrc));
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Main function for copying a file from guest to the host.
 *
 * @return VBox status code.
 * @param  strSrcFile         Full path of source file on the host to copy.
 * @param  srcFile            Guest file (source) to copy to the host. Must be in opened and ready state already.
 * @param  strDstFile         Full destination path and file name (guest style) to copy file to.
 * @param  phDstFile          Pointer to host file handle (destination) to copy to. Must be in opened and ready state already.
 * @param  fFileCopyFlags     File copy flags.
 * @param  offCopy            Offset (in bytes) where to start copying the source file.
 * @param  cbSize             Size (in bytes) to copy from the source file.
 */
int GuestSessionTask::fileCopyFromGuestInner(const Utf8Str &strSrcFile, ComObjPtr<GuestFile> &srcFile,
                                             const Utf8Str &strDstFile, PRTFILE phDstFile,
                                             FileCopyFlag_T fFileCopyFlags, uint64_t offCopy, uint64_t cbSize)
{
    RT_NOREF(fFileCopyFlags);

    BOOL fCanceled = FALSE;
    uint64_t cbWrittenTotal = 0;
    uint64_t cbToRead       = cbSize;

    uint32_t uTimeoutMs = 30 * 1000; /* 30s timeout. */

    int vrc = VINF_SUCCESS;

    if (offCopy)
    {
        uint64_t offActual;
        vrc = srcFile->i_seekAt(offCopy, GUEST_FILE_SEEKTYPE_BEGIN, uTimeoutMs, &offActual);
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Seeking to offset %RU64 of guest file \"%s\" failed: %Rrc"),
                                           offCopy, strSrcFile.c_str(), vrc));
            return vrc;
        }
    }

    BYTE byBuf[_64K]; /** @todo Can we do better here? */
    while (cbToRead)
    {
        uint32_t cbRead;
        const uint32_t cbChunk = RT_MIN(cbToRead, sizeof(byBuf));
        vrc = srcFile->i_readData(cbChunk, uTimeoutMs, byBuf, sizeof(byBuf), &cbRead);
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Reading %RU32 bytes @ %RU64 from guest \"%s\" failed: %Rrc", "", cbChunk),
                                           cbChunk, cbWrittenTotal, strSrcFile.c_str(), vrc));
            break;
        }

        vrc = RTFileWrite(*phDstFile, byBuf, cbRead, NULL /* No partial writes */);
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Writing %RU32 bytes to host file \"%s\" failed: %Rrc", "", cbRead),
                                           cbRead, strDstFile.c_str(), vrc));
            break;
        }

        AssertBreak(cbToRead >= cbRead);
        cbToRead -= cbRead;

        /* Update total bytes written to the guest. */
        cbWrittenTotal += cbRead;
        AssertBreak(cbWrittenTotal <= cbSize);

        /* Did the user cancel the operation above? */
        if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
            && fCanceled)
            break;

        vrc = setProgress((ULONG)((double)cbWrittenTotal / (double)cbSize / 100.0));
        if (RT_FAILURE(vrc))
            break;
    }

    if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
        && fCanceled)
        return VINF_SUCCESS;

    if (RT_FAILURE(vrc))
        return vrc;

    /*
     * Even if we succeeded until here make sure to check whether we really transfered
     * everything.
     */
    if (   cbSize > 0
        && cbWrittenTotal == 0)
    {
        /* If nothing was transfered but the file size was > 0 then "vbox_cat" wasn't able to write
         * to the destination -> access denied. */
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(tr("Writing guest file \"%s\" to host file \"%s\" failed: Access denied"),
                                       strSrcFile.c_str(), strDstFile.c_str()));
        vrc = VERR_ACCESS_DENIED;
    }
    else if (cbWrittenTotal < cbSize)
    {
        /* If we did not copy all let the user know. */
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(tr("Copying guest file \"%s\" to host file \"%s\" failed (%RU64/%RU64 bytes transfered)"),
                                       strSrcFile.c_str(), strDstFile.c_str(), cbWrittenTotal, cbSize));
        vrc = VERR_INTERRUPTED;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Copies a file from the guest to the host.
 *
 * @return VBox status code. VINF_NO_CHANGE if file was skipped.
 * @param  strSrc               Full path of source file on the guest to copy.
 * @param  strDst               Full destination path and file name (host style) to copy file to.
 * @param  fFileCopyFlags       File copy flags.
 */
int GuestSessionTask::fileCopyFromGuest(const Utf8Str &strSrc, const Utf8Str &strDst, FileCopyFlag_T fFileCopyFlags)
{
    LogFlowThisFunc(("strSource=%s, strDest=%s, enmFileCopyFlags=%#x\n", strSrc.c_str(), strDst.c_str(), fFileCopyFlags));

    GuestFileOpenInfo srcOpenInfo;
    srcOpenInfo.mFilename     = strSrc;
    srcOpenInfo.mOpenAction   = FileOpenAction_OpenExisting;
    srcOpenInfo.mAccessMode   = FileAccessMode_ReadOnly;
    srcOpenInfo.mSharingMode  = FileSharingMode_All; /** @todo Use _Read when implemented. */

    ComObjPtr<GuestFile> srcFile;

    GuestFsObjData srcObjData;
    int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    int vrc = mSession->i_fsQueryInfo(strSrc, TRUE /* fFollowSymlinks */, srcObjData, &vrcGuest);
    if (RT_FAILURE(vrc))
    {
        if (vrc == VERR_GSTCTL_GUEST_ERROR)
            setProgressErrorMsg(VBOX_E_IPRT_ERROR, tr("Guest file lookup failed"),
                                GuestErrorInfo(GuestErrorInfo::Type_ToolStat, vrcGuest, strSrc.c_str()));
        else
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Guest file lookup for \"%s\" failed: %Rrc"), strSrc.c_str(), vrc));
    }
    else
    {
        switch (srcObjData.mType)
        {
            case FsObjType_File:
                break;

            case FsObjType_Symlink:
                if (!(fFileCopyFlags & FileCopyFlag_FollowLinks))
                {
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(tr("Guest file \"%s\" is a symbolic link"),
                                                   strSrc.c_str()));
                    vrc = VERR_IS_A_SYMLINK;
                }
                break;

            default:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Guest object \"%s\" is not a file (is type %#x)"),
                                               strSrc.c_str(), srcObjData.mType));
                vrc = VERR_NOT_A_FILE;
                break;
        }
    }

    if (RT_FAILURE(vrc))
        return vrc;

    vrc = mSession->i_fileOpen(srcOpenInfo, srcFile, &vrcGuest);
    if (RT_FAILURE(vrc))
    {
        if (vrc == VERR_GSTCTL_GUEST_ERROR)
            setProgressErrorMsg(VBOX_E_IPRT_ERROR, tr("Guest file could not be opened"),
                                GuestErrorInfo(GuestErrorInfo::Type_File, vrcGuest, strSrc.c_str()));
        else
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Guest file \"%s\" could not be opened: %Rrc"), strSrc.c_str(), vrc));
    }

    if (RT_FAILURE(vrc))
        return vrc;

    RTFSOBJINFO dstObjInfo;
    RT_ZERO(dstObjInfo);

    bool fSkip = false; /* Whether to skip handling the file. */

    if (RT_SUCCESS(vrc))
    {
        vrc = RTPathQueryInfo(strDst.c_str(), &dstObjInfo, RTFSOBJATTRADD_NOTHING);
        if (RT_SUCCESS(vrc))
        {
            if (fFileCopyFlags & FileCopyFlag_NoReplace)
            {
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host file \"%s\" already exists"), strDst.c_str()));
                vrc = VERR_ALREADY_EXISTS;
            }

            if (fFileCopyFlags & FileCopyFlag_Update)
            {
                RTTIMESPEC srcModificationTimeTS;
                RTTimeSpecSetSeconds(&srcModificationTimeTS, srcObjData.mModificationTime);
                if (RTTimeSpecCompare(&srcModificationTimeTS, &dstObjInfo.ModificationTime) <= 0)
                {
                    LogRel2(("Guest Control: Host file \"%s\" has same or newer modification date, skipping", strDst.c_str()));
                    fSkip = true;
                }
            }
        }
        else
        {
            if (vrc != VERR_FILE_NOT_FOUND) /* Destination file does not exist (yet)? */
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host file lookup for \"%s\" failed: %Rrc"),
                                               strDst.c_str(), vrc));
        }
    }

    if (fSkip)
    {
        int vrc2 = srcFile->i_closeFile(&vrcGuest);
        AssertRC(vrc2);
        return VINF_SUCCESS;
    }

    char *pszDstFile = NULL;

    if (RT_SUCCESS(vrc))
    {
        if (RTFS_IS_FILE(dstObjInfo.Attr.fMode))
        {
            if (fFileCopyFlags & FileCopyFlag_NoReplace)
            {
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host file \"%s\" already exists"), strDst.c_str()));
                vrc = VERR_ALREADY_EXISTS;
            }
            else
                pszDstFile = RTStrDup(strDst.c_str());
        }
        else if (RTFS_IS_DIRECTORY(dstObjInfo.Attr.fMode))
        {
            /* Build the final file name with destination path (on the host). */
            char szDstPath[RTPATH_MAX];
            vrc = RTStrCopy(szDstPath, sizeof(szDstPath), strDst.c_str());
            if (RT_SUCCESS(vrc))
            {
                vrc = RTPathAppend(szDstPath, sizeof(szDstPath), RTPathFilenameEx(strSrc.c_str(), mfPathStyle));
                if (RT_SUCCESS(vrc))
                    pszDstFile = RTStrDup(szDstPath);
            }
        }
        else if (RTFS_IS_SYMLINK(dstObjInfo.Attr.fMode))
        {
            if (!(fFileCopyFlags & FileCopyFlag_FollowLinks))
            {
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host file \"%s\" is a symbolic link"),
                                               strDst.c_str()));
                vrc = VERR_IS_A_SYMLINK;
            }
            else
                pszDstFile = RTStrDup(strDst.c_str());
        }
        else
        {
            LogFlowThisFunc(("Object type %RU32 not implemented yet\n", dstObjInfo.Attr.fMode));
            vrc = VERR_NOT_IMPLEMENTED;
        }
    }
    else if (vrc == VERR_FILE_NOT_FOUND)
        pszDstFile = RTStrDup(strDst.c_str());

    if (   RT_SUCCESS(vrc)
        || vrc == VERR_FILE_NOT_FOUND)
    {
        if (!pszDstFile)
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR, Utf8StrFmt(tr("No memory to allocate host file path")));
            vrc = VERR_NO_MEMORY;
        }
        else
        {
            RTFILE hDstFile;
            vrc = RTFileOpen(&hDstFile, pszDstFile,
                             RTFILE_O_WRITE | RTFILE_O_OPEN_CREATE | RTFILE_O_DENY_WRITE); /** @todo Use the correct open modes! */
            if (RT_SUCCESS(vrc))
            {
                LogFlowThisFunc(("Copying '%s' to '%s' (%RI64 bytes) ...\n",
                                 strSrc.c_str(), pszDstFile, srcObjData.mObjectSize));

                vrc = fileCopyFromGuestInner(strSrc, srcFile, pszDstFile, &hDstFile, fFileCopyFlags,
                                             0 /* Offset, unused */, (uint64_t)srcObjData.mObjectSize);

                int vrc2 = RTFileClose(hDstFile);
                AssertRC(vrc2);
            }
            else
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Opening/creating host file \"%s\" failed: %Rrc"),
                                               pszDstFile, vrc));
        }
    }

    RTStrFree(pszDstFile);

    int vrc2 = srcFile->i_closeFile(&vrcGuest);
    AssertRC(vrc2);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Main function for copying a file from host to the guest.
 *
 * @return VBox status code.
 * @param  strSrcFile         Full path of source file on the host to copy.
 * @param  hVfsFile           The VFS file handle to read from.
 * @param  strDstFile         Full destination path and file name (guest style) to copy file to.
 * @param  fileDst            Guest file (destination) to copy to the guest. Must be in opened and ready state already.
 * @param  fFileCopyFlags     File copy flags.
 * @param  offCopy            Offset (in bytes) where to start copying the source file.
 * @param  cbSize             Size (in bytes) to copy from the source file.
 */
int GuestSessionTask::fileCopyToGuestInner(const Utf8Str &strSrcFile, RTVFSFILE hVfsFile,
                                           const Utf8Str &strDstFile, ComObjPtr<GuestFile> &fileDst,
                                           FileCopyFlag_T fFileCopyFlags, uint64_t offCopy, uint64_t cbSize)
{
    RT_NOREF(fFileCopyFlags);

    BOOL fCanceled = FALSE;
    uint64_t cbWrittenTotal = 0;
    uint64_t cbToRead       = cbSize;

    uint32_t uTimeoutMs = 30 * 1000; /* 30s timeout. */

    int vrc = VINF_SUCCESS;

    if (offCopy)
    {
        uint64_t offActual;
        vrc = RTVfsFileSeek(hVfsFile, offCopy, RTFILE_SEEK_END, &offActual);
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Seeking to offset %RU64 of host file \"%s\" failed: %Rrc"),
                                           offCopy, strSrcFile.c_str(), vrc));
            return vrc;
        }
    }

    BYTE byBuf[_64K];
    while (cbToRead)
    {
        size_t cbRead;
        const uint32_t cbChunk = RT_MIN(cbToRead, sizeof(byBuf));
        vrc = RTVfsFileRead(hVfsFile, byBuf, cbChunk, &cbRead);
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Reading %RU32 bytes @ %RU64 from host file \"%s\" failed: %Rrc", "", cbChunk),
                                           cbChunk, cbWrittenTotal, strSrcFile.c_str(), vrc));
            break;
        }

        vrc = fileDst->i_writeData(uTimeoutMs, byBuf, (uint32_t)cbRead, NULL /* No partial writes */);
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Writing %zu bytes to guest file \"%s\" failed: %Rrc", "", cbRead),
                                           cbRead, strDstFile.c_str(), vrc));
            break;
        }

        Assert(cbToRead >= cbRead);
        cbToRead -= cbRead;

        /* Update total bytes written to the guest. */
        cbWrittenTotal += cbRead;
        Assert(cbWrittenTotal <= cbSize);

        /* Did the user cancel the operation above? */
        if (   SUCCEEDED(mProgress->COMGETTER(Canceled(&fCanceled)))
            && fCanceled)
            break;

        vrc = setProgress((ULONG)((double)cbWrittenTotal / (double)cbSize / 100.0));
        if (RT_FAILURE(vrc))
            break;
    }

    if (RT_FAILURE(vrc))
        return vrc;

    /*
     * Even if we succeeded until here make sure to check whether we really transfered
     * everything.
     */
    if (   cbSize > 0
        && cbWrittenTotal == 0)
    {
        /* If nothing was transfered but the file size was > 0 then "vbox_cat" wasn't able to write
         * to the destination -> access denied. */
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(tr("Writing to guest file \"%s\" failed: Access denied"),
                                       strDstFile.c_str()));
        vrc = VERR_ACCESS_DENIED;
    }
    else if (cbWrittenTotal < cbSize)
    {
        /* If we did not copy all let the user know. */
        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                            Utf8StrFmt(tr("Copying to guest file \"%s\" failed (%RU64/%RU64 bytes transfered)"),
                                       strDstFile.c_str(), cbWrittenTotal, cbSize));
        vrc = VERR_INTERRUPTED;
    }

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Copies a file from the guest to the host.
 *
 * @return VBox status code. VINF_NO_CHANGE if file was skipped.
 * @param  strSrc               Full path of source file on the host to copy.
 * @param  strDst               Full destination path and file name (guest style) to copy file to.
 * @param  fFileCopyFlags       File copy flags.
 */
int GuestSessionTask::fileCopyToGuest(const Utf8Str &strSrc, const Utf8Str &strDst, FileCopyFlag_T fFileCopyFlags)
{
    LogFlowThisFunc(("strSource=%s, strDst=%s, fFileCopyFlags=0x%x\n", strSrc.c_str(), strDst.c_str(), fFileCopyFlags));

    Utf8Str strDstFinal = strDst;

    GuestFileOpenInfo dstOpenInfo;
    dstOpenInfo.mFilename        = strDstFinal;
    if (fFileCopyFlags & FileCopyFlag_NoReplace)
        dstOpenInfo.mOpenAction  = FileOpenAction_CreateNew;
    else
        dstOpenInfo.mOpenAction  = FileOpenAction_CreateOrReplace;
    dstOpenInfo.mAccessMode      = FileAccessMode_WriteOnly;
    dstOpenInfo.mSharingMode     = FileSharingMode_All; /** @todo Use _Read when implemented. */

    ComObjPtr<GuestFile> dstFile;
    int vrcGuest;
    int vrc = mSession->i_fileOpen(dstOpenInfo, dstFile, &vrcGuest);
    if (RT_FAILURE(vrc))
    {
        if (vrc == VERR_GSTCTL_GUEST_ERROR)
            setProgressErrorMsg(VBOX_E_IPRT_ERROR, tr("Guest file could not be opened"),
                                GuestErrorInfo(GuestErrorInfo::Type_File, vrcGuest, strSrc.c_str()));
        else
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Guest file \"%s\" could not be opened: %Rrc"), strSrc.c_str(), vrc));
        return vrc;
    }

    char szSrcReal[RTPATH_MAX];

    RTFSOBJINFO srcObjInfo;
    RT_ZERO(srcObjInfo);

    bool fSkip = false; /* Whether to skip handling the file. */

    if (RT_SUCCESS(vrc))
    {
        vrc = RTPathReal(strSrc.c_str(), szSrcReal, sizeof(szSrcReal));
        if (RT_FAILURE(vrc))
        {
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Host path lookup for file \"%s\" failed: %Rrc"),
                                           strSrc.c_str(), vrc));
        }
        else
        {
            vrc = RTPathQueryInfo(szSrcReal, &srcObjInfo, RTFSOBJATTRADD_NOTHING);
            if (RT_SUCCESS(vrc))
            {
                if (fFileCopyFlags & FileCopyFlag_Update)
                {
                    GuestFsObjData dstObjData;
                    vrc = mSession->i_fileQueryInfo(strDstFinal, RT_BOOL(fFileCopyFlags & FileCopyFlag_FollowLinks), dstObjData,
                                                    &vrcGuest);
                    if (RT_SUCCESS(vrc))
                    {
                        RTTIMESPEC dstModificationTimeTS;
                        RTTimeSpecSetSeconds(&dstModificationTimeTS, dstObjData.mModificationTime);
                        if (RTTimeSpecCompare(&dstModificationTimeTS, &srcObjInfo.ModificationTime) <= 0)
                        {
                            LogRel2(("Guest Control: Guest file \"%s\" has same or newer modification date, skipping",
                                     strDstFinal.c_str()));
                            fSkip = true;
                        }
                    }
                    else
                    {
                        if (vrc == VERR_GSTCTL_GUEST_ERROR)
                        {
                            switch (vrcGuest)
                            {
                                case VERR_FILE_NOT_FOUND:
                                    vrc = VINF_SUCCESS;
                                    break;

                                default:
                                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                        Utf8StrFmt(tr("Guest error while determining object data for guest file \"%s\": %Rrc"),
                                                           strDstFinal.c_str(), vrcGuest));
                                    break;
                            }
                        }
                        else
                            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                Utf8StrFmt(tr("Host error while determining object data for guest file \"%s\": %Rrc"),
                                                           strDstFinal.c_str(), vrc));
                    }
                }
            }
            else
            {
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Host file lookup for \"%s\" failed: %Rrc"),
                                               szSrcReal, vrc));
            }
        }
    }

    if (fSkip)
    {
        int vrc2 = dstFile->i_closeFile(&vrcGuest);
        AssertRC(vrc2);
        return VINF_SUCCESS;
    }

    if (RT_SUCCESS(vrc))
    {
        RTVFSFILE hSrcFile;
        vrc = RTVfsFileOpenNormal(szSrcReal, RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE, &hSrcFile);
        if (RT_SUCCESS(vrc))
        {
            LogFlowThisFunc(("Copying '%s' to '%s' (%RI64 bytes) ...\n",
                             szSrcReal, strDstFinal.c_str(), srcObjInfo.cbObject));

            vrc = fileCopyToGuestInner(szSrcReal, hSrcFile, strDstFinal, dstFile,
                                       fFileCopyFlags, 0 /* Offset, unused */, srcObjInfo.cbObject);

            int vrc2 = RTVfsFileRelease(hSrcFile);
            AssertRC(vrc2);
        }
        else
            setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                Utf8StrFmt(tr("Opening host file \"%s\" failed: %Rrc"),
                                           szSrcReal, vrc));
    }

    int vrc2 = dstFile->i_closeFile(&vrcGuest);
    AssertRC(vrc2);

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

/**
 * Adds a guest file system entry to a given list.
 *
 * @return VBox status code.
 * @param  strFile              Path to file system entry to add.
 * @param  fsObjData            Guest file system information of entry to add.
 */
int FsList::AddEntryFromGuest(const Utf8Str &strFile, const GuestFsObjData &fsObjData)
{
    LogFlowFunc(("Adding '%s'\n", strFile.c_str()));

    FsEntry *pEntry = NULL;
    try
    {
        pEntry = new FsEntry();
        pEntry->fMode = fsObjData.GetFileMode();
        pEntry->strPath = strFile;

        mVecEntries.push_back(pEntry);
    }
    catch (std::bad_alloc &)
    {
        if (pEntry)
            delete pEntry;
        return VERR_NO_MEMORY;
    }

    return VINF_SUCCESS;
}

/**
 * Adds a host file system entry to a given list.
 *
 * @return VBox status code.
 * @param  strFile              Path to file system entry to add.
 * @param  pcObjInfo            File system information of entry to add.
 */
int FsList::AddEntryFromHost(const Utf8Str &strFile, PCRTFSOBJINFO pcObjInfo)
{
    LogFlowFunc(("Adding '%s'\n", strFile.c_str()));

    FsEntry *pEntry = NULL;
    try
    {
        pEntry = new FsEntry();
        pEntry->fMode = pcObjInfo->Attr.fMode & RTFS_TYPE_MASK;
        pEntry->strPath = strFile;

        mVecEntries.push_back(pEntry);
    }
    catch (std::bad_alloc &)
    {
        if (pEntry)
            delete pEntry;
        return VERR_NO_MEMORY;
    }

    return VINF_SUCCESS;
}

FsList::FsList(const GuestSessionTask &Task)
    : mTask(Task)
{
}

FsList::~FsList()
{
    Destroy();
}

/**
 * Initializes a file list.
 *
 * @return VBox status code.
 * @param  strSrcRootAbs        Source root path (absolute) for this file list.
 * @param  strDstRootAbs        Destination root path (absolute) for this file list.
 * @param  SourceSpec           Source specification to use.
 */
int FsList::Init(const Utf8Str &strSrcRootAbs, const Utf8Str &strDstRootAbs,
                 const GuestSessionFsSourceSpec &SourceSpec)
{
    mSrcRootAbs = strSrcRootAbs;
    mDstRootAbs = strDstRootAbs;
    mSourceSpec = SourceSpec;

    /* Note: Leave the source and dest roots unmodified -- how paths will be treated
     *       will be done directly when working on those. See @bugref{10139}. */

    LogFlowFunc(("mSrcRootAbs=%s, mDstRootAbs=%s, fCopyFlags=%#x\n",
                 mSrcRootAbs.c_str(), mDstRootAbs.c_str(), mSourceSpec.Type.Dir.fCopyFlags));

    return VINF_SUCCESS;
}

/**
 * Destroys a file list.
 */
void FsList::Destroy(void)
{
    LogFlowFuncEnter();

    FsEntries::iterator itEntry = mVecEntries.begin();
    while (itEntry != mVecEntries.end())
    {
        FsEntry *pEntry = *itEntry;
        delete pEntry;
        mVecEntries.erase(itEntry);
        itEntry = mVecEntries.begin();
    }

    Assert(mVecEntries.empty());

    LogFlowFuncLeave();
}

/**
 * Builds a guest file list from a given path (and optional filter).
 *
 * @return VBox status code.
 * @param  strPath              Directory on the guest to build list from.
 * @param  strSubDir            Current sub directory path; needed for recursion.
 *                              Set to an empty path.
 */
int FsList::AddDirFromGuest(const Utf8Str &strPath, const Utf8Str &strSubDir /* = "" */)
{
    Utf8Str strPathAbs = strPath;
    if (   !strPathAbs.endsWith("/")
        && !strPathAbs.endsWith("\\"))
        strPathAbs += "/";

    Utf8Str strPathSub = strSubDir;
    if (   strPathSub.isNotEmpty()
        && !strPathSub.endsWith("/")
        && !strPathSub.endsWith("\\"))
        strPathSub += "/";

    strPathAbs += strPathSub;

    LogFlowFunc(("Entering '%s' (sub '%s')\n", strPathAbs.c_str(), strPathSub.c_str()));

    LogRel2(("Guest Control: Handling directory '%s' on guest ...\n", strPathAbs.c_str()));

    GuestDirectoryOpenInfo dirOpenInfo;
    dirOpenInfo.mFilter = "";
    dirOpenInfo.mPath   = strPathAbs;
    dirOpenInfo.mFlags  = 0; /** @todo Handle flags? */

    const ComObjPtr<GuestSession> &pSession = mTask.GetSession();

    ComObjPtr <GuestDirectory> pDir;
    int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    int vrc = pSession->i_directoryOpen(dirOpenInfo, pDir, &vrcGuest);
    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
            case VERR_INVALID_PARAMETER:
               break;

            case VERR_GSTCTL_GUEST_ERROR:
                break;

            default:
                break;
        }

        return vrc;
    }

    if (strPathSub.isNotEmpty())
    {
        GuestFsObjData fsObjData;
        fsObjData.mType = FsObjType_Directory;

        vrc = AddEntryFromGuest(strPathSub, fsObjData);
    }

    if (RT_SUCCESS(vrc))
    {
        ComObjPtr<GuestFsObjInfo> fsObjInfo;
        while (RT_SUCCESS(vrc = pDir->i_read(fsObjInfo, &vrcGuest)))
        {
            FsObjType_T enmObjType = FsObjType_Unknown; /* Shut up MSC. */
            HRESULT hrc2 = fsObjInfo->COMGETTER(Type)(&enmObjType);
            AssertComRC(hrc2);

            com::Bstr bstrName;
            hrc2 = fsObjInfo->COMGETTER(Name)(bstrName.asOutParam());
            AssertComRC(hrc2);

            Utf8Str strEntry = strPathSub + Utf8Str(bstrName);

            LogFlowFunc(("Entry '%s'\n", strEntry.c_str()));

            switch (enmObjType)
            {
                case FsObjType_Directory:
                {
                    if (   bstrName.equals(".")
                        || bstrName.equals(".."))
                    {
                        break;
                    }

                    LogRel2(("Guest Control: Directory '%s'\n", strEntry.c_str()));

                    if (!(mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_Recursive))
                        break;

                    vrc = AddDirFromGuest(strPath, strEntry);
                    break;
                }

                case FsObjType_Symlink:
                {
                    if (mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_FollowLinks)
                    {
                        /** @todo Symlink handling from guest is not implemented yet.
                         *        See IGuestSession::symlinkRead(). */
                        LogRel2(("Guest Control: Warning: Symlink support on guest side not available, skipping '%s'",
                                 strEntry.c_str()));
                    }
                    break;
                }

                case FsObjType_File:
                {
                    LogRel2(("Guest Control: File '%s'\n", strEntry.c_str()));

                    vrc = AddEntryFromGuest(strEntry, fsObjInfo->i_getData());
                    break;
                }

                default:
                    break;
            }
        }

        if (vrc == VERR_NO_MORE_FILES) /* End of listing reached? */
            vrc = VINF_SUCCESS;
    }

    int vrc2 = pDir->i_closeInternal(&vrcGuest);
    if (RT_SUCCESS(vrc))
        vrc = vrc2;

    return vrc;
}

/**
 * Builds a host file list from a given path (and optional filter).
 *
 * @return VBox status code.
 * @param  strPath              Directory on the host to build list from.
 * @param  strSubDir            Current sub directory path; needed for recursion.
 *                              Set to an empty path.
 */
int FsList::AddDirFromHost(const Utf8Str &strPath, const Utf8Str &strSubDir)
{
    Utf8Str strPathAbs = strPath;
    if (   !strPathAbs.endsWith("/")
        && !strPathAbs.endsWith("\\"))
        strPathAbs += "/";

    Utf8Str strPathSub = strSubDir;
    if (   strPathSub.isNotEmpty()
        && !strPathSub.endsWith("/")
        && !strPathSub.endsWith("\\"))
        strPathSub += "/";

    strPathAbs += strPathSub;

    LogFlowFunc(("Entering '%s' (sub '%s')\n", strPathAbs.c_str(), strPathSub.c_str()));

    LogRel2(("Guest Control: Handling directory '%s' on host ...\n", strPathAbs.c_str()));

    RTFSOBJINFO objInfo;
    int vrc = RTPathQueryInfo(strPathAbs.c_str(), &objInfo, RTFSOBJATTRADD_NOTHING);
    if (RT_SUCCESS(vrc))
    {
        if (RTFS_IS_DIRECTORY(objInfo.Attr.fMode))
        {
            if (strPathSub.isNotEmpty())
                vrc = AddEntryFromHost(strPathSub, &objInfo);

            if (RT_SUCCESS(vrc))
            {
                RTDIR hDir;
                vrc = RTDirOpen(&hDir, strPathAbs.c_str());
                if (RT_SUCCESS(vrc))
                {
                    do
                    {
                        /* Retrieve the next directory entry. */
                        RTDIRENTRYEX Entry;
                        vrc = RTDirReadEx(hDir, &Entry, NULL, RTFSOBJATTRADD_NOTHING, RTPATH_F_ON_LINK);
                        if (RT_FAILURE(vrc))
                        {
                            if (vrc == VERR_NO_MORE_FILES)
                                vrc = VINF_SUCCESS;
                            break;
                        }

                        Utf8Str strEntry = strPathSub + Utf8Str(Entry.szName);

                        LogFlowFunc(("Entry '%s'\n", strEntry.c_str()));

                        switch (Entry.Info.Attr.fMode & RTFS_TYPE_MASK)
                        {
                            case RTFS_TYPE_DIRECTORY:
                            {
                                /* Skip "." and ".." entries. */
                                if (RTDirEntryExIsStdDotLink(&Entry))
                                    break;

                                LogRel2(("Guest Control: Directory '%s'\n", strEntry.c_str()));

                                if (!(mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_Recursive))
                                    break;

                                vrc = AddDirFromHost(strPath, strEntry);
                                break;
                            }

                            case RTFS_TYPE_FILE:
                            {
                                LogRel2(("Guest Control: File '%s'\n", strEntry.c_str()));

                                vrc = AddEntryFromHost(strEntry, &Entry.Info);
                                break;
                            }

                            case RTFS_TYPE_SYMLINK:
                            {
                                if (mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_FollowLinks)
                                {
                                    Utf8Str strEntryAbs = strPathAbs + Utf8Str(Entry.szName);

                                    char szPathReal[RTPATH_MAX];
                                    vrc = RTPathReal(strEntryAbs.c_str(), szPathReal, sizeof(szPathReal));
                                    if (RT_SUCCESS(vrc))
                                    {
                                        vrc = RTPathQueryInfo(szPathReal, &objInfo, RTFSOBJATTRADD_NOTHING);
                                        if (RT_SUCCESS(vrc))
                                        {
                                            if (RTFS_IS_DIRECTORY(objInfo.Attr.fMode))
                                            {
                                                LogRel2(("Guest Control: Symbolic link '%s' -> '%s' (directory)\n",
                                                         strEntryAbs.c_str(), szPathReal));
                                                vrc = AddDirFromHost(strPath, strEntry);
                                            }
                                            else if (RTFS_IS_FILE(objInfo.Attr.fMode))
                                            {
                                                LogRel2(("Guest Control: Symbolic link '%s' -> '%s' (file)\n",
                                                         strEntryAbs.c_str(), szPathReal));
                                                vrc = AddEntryFromHost(strEntry, &objInfo);
                                            }
                                            else
                                                vrc = VERR_NOT_SUPPORTED;
                                        }

                                        if (RT_FAILURE(vrc))
                                            LogRel2(("Guest Control: Unable to query symbolic link info for '%s', rc=%Rrc\n",
                                                     szPathReal, vrc));
                                    }
                                    else
                                    {
                                        LogRel2(("Guest Control: Unable to resolve symlink for '%s', rc=%Rrc\n", strPathAbs.c_str(), vrc));
                                        if (vrc == VERR_FILE_NOT_FOUND) /* Broken symlink, skip. */
                                            vrc = VINF_SUCCESS;
                                    }
                                }
                                else
                                    LogRel2(("Guest Control: Symbolic link '%s' (skipped)\n", strEntry.c_str()));
                                break;
                            }

                            default:
                                break;
                        }

                    } while (RT_SUCCESS(vrc));

                    RTDirClose(hDir);
                }
            }
        }
        else if (RTFS_IS_FILE(objInfo.Attr.fMode))
        {
            vrc = VERR_IS_A_FILE;
        }
        else if (RTFS_IS_SYMLINK(objInfo.Attr.fMode))
        {
            vrc = VERR_IS_A_SYMLINK;
        }
        else
            vrc = VERR_NOT_SUPPORTED;
    }
    else
        LogFlowFunc(("Unable to query '%s', rc=%Rrc\n", strPathAbs.c_str(), vrc));

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

GuestSessionTaskOpen::GuestSessionTaskOpen(GuestSession *pSession, uint32_t uFlags, uint32_t uTimeoutMS)
                                           : GuestSessionTask(pSession)
                                           , mFlags(uFlags)
                                           , mTimeoutMS(uTimeoutMS)
{
    m_strTaskName = "gctlSesOpen";
}

GuestSessionTaskOpen::~GuestSessionTaskOpen(void)
{

}

/** @copydoc GuestSessionTask::Run */
int GuestSessionTaskOpen::Run(void)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(mSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int vrc = mSession->i_startSession(NULL /*pvrcGuest*/);
    /* Nothing to do here anymore. */

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

GuestSessionCopyTask::GuestSessionCopyTask(GuestSession *pSession)
                                           : GuestSessionTask(pSession)
{
}

GuestSessionCopyTask::~GuestSessionCopyTask()
{
    FsLists::iterator itList = mVecLists.begin();
    while (itList != mVecLists.end())
    {
        FsList *pFsList = (*itList);
        pFsList->Destroy();
        delete pFsList;
        mVecLists.erase(itList);
        itList = mVecLists.begin();
    }

    Assert(mVecLists.empty());
}

GuestSessionTaskCopyFrom::GuestSessionTaskCopyFrom(GuestSession *pSession, GuestSessionFsSourceSet const &vecSrc,
                                                   const Utf8Str &strDest)
    : GuestSessionCopyTask(pSession)
{
    m_strTaskName = "gctlCpyFrm";

    mSources = vecSrc;
    mDest    = strDest;
}

GuestSessionTaskCopyFrom::~GuestSessionTaskCopyFrom(void)
{
}

/**
 * Initializes a copy-from-guest task.
 *
 * @returns HRESULT
 * @param   strTaskDesc         Friendly task description.
 */
HRESULT GuestSessionTaskCopyFrom::Init(const Utf8Str &strTaskDesc)
{
    setTaskDesc(strTaskDesc);

    /* Create the progress object. */
    ComObjPtr<Progress> pProgress;
    HRESULT hrc = pProgress.createObject();
    if (FAILED(hrc))
        return hrc;

    mProgress = pProgress;

    int vrc = VINF_SUCCESS;

    ULONG cOperations = 0;
    Utf8Str strErrorInfo;

    /**
     * Note: We need to build up the file/directory here instead of GuestSessionTaskCopyFrom::Run
     *       because the caller expects a ready-for-operation progress object on return.
     *       The progress object will have a variable operation count, based on the elements to
     *       be processed.
     */

    if (mDest.isEmpty())
    {
        strErrorInfo = Utf8StrFmt(tr("Host destination must not be empty"));
        vrc = VERR_INVALID_PARAMETER;
    }
    else
    {
        GuestSessionFsSourceSet::iterator itSrc = mSources.begin();
        while (itSrc != mSources.end())
        {
            Utf8Str strSrc = itSrc->strSource;
            Utf8Str strDst = mDest;

            bool    fFollowSymlinks;

            if (strSrc.isEmpty())
            {
                strErrorInfo = Utf8StrFmt(tr("Guest source entry must not be empty"));
                vrc = VERR_INVALID_PARAMETER;
                break;
            }

            if (itSrc->enmType == FsObjType_Directory)
            {
                /* If the source does not end with a slash, copy over the entire directory
                 * (and not just its contents). */
                /** @todo r=bird: Try get the path style stuff right and stop assuming all guest are windows guests.  */
                if (   !strSrc.endsWith("/")
                    && !strSrc.endsWith("\\"))
                {
                    if (!RTPATH_IS_SLASH(strDst[strDst.length() - 1]))
                        strDst += "/";

                    strDst += Utf8Str(RTPathFilenameEx(strSrc.c_str(), mfPathStyle));
                }

                fFollowSymlinks = itSrc->Type.Dir.fCopyFlags & DirectoryCopyFlag_FollowLinks;
            }
            else
            {
                fFollowSymlinks = RT_BOOL(itSrc->Type.File.fCopyFlags & FileCopyFlag_FollowLinks);
            }

            LogFlowFunc(("strSrc=%s, strDst=%s, fFollowSymlinks=%RTbool\n", strSrc.c_str(), strDst.c_str(), fFollowSymlinks));

            GuestFsObjData srcObjData;
            int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
            vrc = mSession->i_fsQueryInfo(strSrc, fFollowSymlinks, srcObjData, &vrcGuest);
            if (RT_FAILURE(vrc))
            {
                if (vrc == VERR_GSTCTL_GUEST_ERROR)
                    strErrorInfo = GuestBase::getErrorAsString(tr("Guest file lookup failed"),
                                                               GuestErrorInfo(GuestErrorInfo::Type_ToolStat, vrcGuest, strSrc.c_str()));
                else
                    strErrorInfo = Utf8StrFmt(tr("Guest file lookup for \"%s\" failed: %Rrc"),
                                              strSrc.c_str(), vrc);
                break;
            }

            if (srcObjData.mType == FsObjType_Directory)
            {
                if (itSrc->enmType != FsObjType_Directory)
                {
                    strErrorInfo = Utf8StrFmt(tr("Guest source is not a file: %s"), strSrc.c_str());
                    vrc = VERR_NOT_A_FILE;
                    break;
                }
            }
            else
            {
                if (itSrc->enmType != FsObjType_File)
                {
                    strErrorInfo = Utf8StrFmt(tr("Guest source is not a directory: %s"), strSrc.c_str());
                    vrc = VERR_NOT_A_DIRECTORY;
                    break;
                }
            }

            FsList *pFsList = NULL;
            try
            {
                pFsList = new FsList(*this);
                vrc = pFsList->Init(strSrc, strDst, *itSrc);
                if (RT_SUCCESS(vrc))
                {
                    if (itSrc->enmType == FsObjType_Directory)
                        vrc = pFsList->AddDirFromGuest(strSrc);
                    else
                        vrc = pFsList->AddEntryFromGuest(RTPathFilename(strSrc.c_str()), srcObjData);
                }

                if (RT_FAILURE(vrc))
                {
                    delete pFsList;
                    strErrorInfo = Utf8StrFmt(tr("Error adding guest source '%s' to list: %Rrc"),
                                              strSrc.c_str(), vrc);
                    break;
                }

                mVecLists.push_back(pFsList);
            }
            catch (std::bad_alloc &)
            {
                vrc = VERR_NO_MEMORY;
                break;
            }

            AssertPtr(pFsList);
            cOperations += (ULONG)pFsList->mVecEntries.size();

            itSrc++;
        }
    }

    if (cOperations) /* Use the first element as description (if available). */
    {
        Assert(mVecLists.size());
        Assert(mVecLists[0]->mVecEntries.size());

        Utf8Str strFirstOp = mDest + mVecLists[0]->mVecEntries[0]->strPath;
        hrc = pProgress->init(static_cast<IGuestSession*>(mSession), Bstr(mDesc).raw(),
                              TRUE /* aCancelable */, cOperations + 1 /* Number of operations */, Bstr(strFirstOp).raw());
    }
    else /* If no operations have been defined, go with an "empty" progress object when will be used for error handling. */
        hrc = pProgress->init(static_cast<IGuestSession*>(mSession), Bstr(mDesc).raw(),
                              TRUE /* aCancelable */, 1 /* cOperations */, Bstr(mDesc).raw());

    if (RT_FAILURE(vrc))
    {
        if (strErrorInfo.isEmpty())
            strErrorInfo = Utf8StrFmt(tr("Failed with %Rrc"), vrc);
        setProgressErrorMsg(VBOX_E_IPRT_ERROR, strErrorInfo);
    }

    LogFlowFunc(("Returning %Rhrc (%Rrc)\n", hrc, vrc));
    return hrc;
}

/** @copydoc GuestSessionTask::Run */
int GuestSessionTaskCopyFrom::Run(void)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(mSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int vrc = VINF_SUCCESS;

    FsLists::const_iterator itList = mVecLists.begin();
    while (itList != mVecLists.end())
    {
        FsList *pList = *itList;
        AssertPtr(pList);

        const bool     fCopyIntoExisting = pList->mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_CopyIntoExisting;
        const bool     fFollowSymlinks   = true; /** @todo */
        const uint32_t fDirMode          = 0700; /** @todo Play safe by default; implement ACLs. */
              uint32_t fDirCreate        = 0;

        if (!fFollowSymlinks)
            fDirCreate |= RTDIRCREATE_FLAGS_NO_SYMLINKS;

        LogFlowFunc(("List: srcRootAbs=%s, dstRootAbs=%s\n", pList->mSrcRootAbs.c_str(), pList->mDstRootAbs.c_str()));

        /* Create the root directory. */
        if (   pList->mSourceSpec.enmType == FsObjType_Directory
            && pList->mSourceSpec.fDryRun == false)
        {
            vrc = directoryCreateOnHost(pList->mDstRootAbs, fDirCreate, fDirMode, fCopyIntoExisting);
            if (RT_FAILURE(vrc))
                break;
        }

        char szPath[RTPATH_MAX];

        FsEntries::const_iterator itEntry = pList->mVecEntries.begin();
        while (itEntry != pList->mVecEntries.end())
        {
            FsEntry *pEntry = *itEntry;
            AssertPtr(pEntry);

            Utf8Str strSrcAbs = pList->mSrcRootAbs;
            Utf8Str strDstAbs = pList->mDstRootAbs;

            LogFlowFunc(("Entry: srcRootAbs=%s, dstRootAbs=%s\n", pList->mSrcRootAbs.c_str(), pList->mDstRootAbs.c_str()));

            if (pList->mSourceSpec.enmType == FsObjType_Directory)
            {
                /* Build the source path on the guest. */
                vrc = RTStrCopy(szPath, sizeof(szPath), pList->mSrcRootAbs.c_str());
                if (RT_SUCCESS(vrc))
                {
                    vrc = RTPathAppend(szPath, sizeof(szPath), pEntry->strPath.c_str());
                    if (RT_SUCCESS(vrc))
                        strSrcAbs = szPath;
                }

                /* Build the destination path on the host. */
                vrc = RTStrCopy(szPath, sizeof(szPath), pList->mDstRootAbs.c_str());
                if (RT_SUCCESS(vrc))
                {
                    vrc = RTPathAppend(szPath, sizeof(szPath), pEntry->strPath.c_str());
                    if (RT_SUCCESS(vrc))
                        strDstAbs = szPath;
                }
            }

            if (pList->mSourceSpec.enmPathStyle == PathStyle_DOS)
                strDstAbs.findReplace('\\', '/');

            mProgress->SetNextOperation(Bstr(strSrcAbs).raw(), 1);

            LogRel2(("Guest Control: Copying '%s' from guest to '%s' on host ...\n", strSrcAbs.c_str(), strDstAbs.c_str()));

            switch (pEntry->fMode & RTFS_TYPE_MASK)
            {
                case RTFS_TYPE_DIRECTORY:
                    LogFlowFunc(("Directory '%s': %s -> %s\n", pEntry->strPath.c_str(), strSrcAbs.c_str(), strDstAbs.c_str()));
                    if (!pList->mSourceSpec.fDryRun)
                        vrc = directoryCreateOnHost(strDstAbs, fDirCreate, fDirMode, fCopyIntoExisting);
                    break;

                case RTFS_TYPE_FILE:
                    RT_FALL_THROUGH();
                case RTFS_TYPE_SYMLINK:
                    LogFlowFunc(("%s '%s': %s -> %s\n", pEntry->strPath.c_str(),
                                 (pEntry->fMode & RTFS_TYPE_MASK) == RTFS_TYPE_SYMLINK ? "Symlink" : "File",
                                  strSrcAbs.c_str(), strDstAbs.c_str()));
                    if (!pList->mSourceSpec.fDryRun)
                        vrc = fileCopyFromGuest(strSrcAbs, strDstAbs, FileCopyFlag_None);
                    break;

                default:
                    LogFlowFunc(("Warning: Type %d for '%s' is not supported\n",
                                 pEntry->fMode & RTFS_TYPE_MASK, strSrcAbs.c_str()));
                    break;
            }

            if (RT_FAILURE(vrc))
                break;

            ++itEntry;
        }

        if (RT_FAILURE(vrc))
            break;

        ++itList;
    }

    if (RT_SUCCESS(vrc))
        vrc = setProgressSuccess();

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

GuestSessionTaskCopyTo::GuestSessionTaskCopyTo(GuestSession *pSession, GuestSessionFsSourceSet const &vecSrc,
                                               const Utf8Str &strDest)
    : GuestSessionCopyTask(pSession)
{
    m_strTaskName = "gctlCpyTo";

    mSources = vecSrc;
    mDest    = strDest;
}

GuestSessionTaskCopyTo::~GuestSessionTaskCopyTo(void)
{
}

/**
 * Initializes a copy-to-guest task.
 *
 * @returns HRESULT
 * @param   strTaskDesc         Friendly task description.
 */
HRESULT GuestSessionTaskCopyTo::Init(const Utf8Str &strTaskDesc)
{
    LogFlowFuncEnter();

    setTaskDesc(strTaskDesc);

    /* Create the progress object. */
    ComObjPtr<Progress> pProgress;
    HRESULT hrc = pProgress.createObject();
    if (FAILED(hrc))
        return hrc;

    mProgress = pProgress;

    int vrc = VINF_SUCCESS;

    ULONG cOperations = 0;
    Utf8Str strErrorInfo;

    /**
     * Note: We need to build up the file/directory here instead of GuestSessionTaskCopyTo::Run
     *       because the caller expects a ready-for-operation progress object on return.
     *       The progress object will have a variable operation count, based on the elements to
     *       be processed.
     */

    if (mDest.isEmpty())
    {
        strErrorInfo = Utf8StrFmt(tr("Guest destination must not be empty"));
        vrc = VERR_INVALID_PARAMETER;
    }
    else
    {
        GuestSessionFsSourceSet::iterator itSrc = mSources.begin();
        while (itSrc != mSources.end())
        {
            Utf8Str strSrc = itSrc->strSource;
            Utf8Str strDst = mDest;

            LogFlowFunc(("strSrc=%s, strDst=%s\n", strSrc.c_str(), strDst.c_str()));

            if (strSrc.isEmpty())
            {
                strErrorInfo = Utf8StrFmt(tr("Host source entry must not be empty"));
                vrc = VERR_INVALID_PARAMETER;
                break;
            }

            RTFSOBJINFO srcFsObjInfo;
            vrc = RTPathQueryInfo(strSrc.c_str(), &srcFsObjInfo, RTFSOBJATTRADD_NOTHING);
            if (RT_FAILURE(vrc))
            {
                strErrorInfo = Utf8StrFmt(tr("No such host file/directory: %s"), strSrc.c_str());
                break;
            }

            if (RTFS_IS_DIRECTORY(srcFsObjInfo.Attr.fMode))
            {
                if (itSrc->enmType != FsObjType_Directory)
                {
                    strErrorInfo = Utf8StrFmt(tr("Host source is not a file: %s"), strSrc.c_str());
                    vrc = VERR_NOT_A_FILE;
                    break;
                }
            }
            else
            {
                if (itSrc->enmType == FsObjType_Directory)
                {
                    strErrorInfo = Utf8StrFmt(tr("Host source is not a directory: %s"), strSrc.c_str());
                    vrc = VERR_NOT_A_DIRECTORY;
                    break;
                }
            }

            FsList *pFsList = NULL;
            try
            {
                pFsList = new FsList(*this);
                vrc = pFsList->Init(strSrc, strDst, *itSrc);
                if (RT_SUCCESS(vrc))
                {
                    if (itSrc->enmType == FsObjType_Directory)
                    {
                        vrc = pFsList->AddDirFromHost(strSrc);
                    }
                    else
                        vrc = pFsList->AddEntryFromHost(RTPathFilename(strSrc.c_str()), &srcFsObjInfo);
                }

                if (RT_FAILURE(vrc))
                {
                    delete pFsList;
                    strErrorInfo = Utf8StrFmt(tr("Error adding host source '%s' to list: %Rrc"),
                                              strSrc.c_str(), vrc);
                    break;
                }

                mVecLists.push_back(pFsList);
            }
            catch (std::bad_alloc &)
            {
                vrc = VERR_NO_MEMORY;
                break;
            }

            AssertPtr(pFsList);
            cOperations += (ULONG)pFsList->mVecEntries.size();

            itSrc++;
        }
    }

    if (cOperations) /* Use the first element as description (if available). */
    {
        Assert(mVecLists.size());
        Assert(mVecLists[0]->mVecEntries.size());

        hrc = pProgress->init(static_cast<IGuestSession*>(mSession), Bstr(mDesc).raw(),
                              TRUE /* aCancelable */, cOperations + 1 /* Number of operations */,
                              Bstr(mDesc).raw());
    }
    else /* If no operations have been defined, go with an "empty" progress object when will be used for error handling. */
        hrc = pProgress->init(static_cast<IGuestSession*>(mSession), Bstr(mDesc).raw(),
                              TRUE /* aCancelable */, 1 /* cOperations */, Bstr(mDesc).raw());

    if (RT_FAILURE(vrc))
    {
        if (strErrorInfo.isEmpty())
            strErrorInfo = Utf8StrFmt(tr("Failed with %Rrc"), vrc);
        setProgressErrorMsg(VBOX_E_IPRT_ERROR, strErrorInfo);
    }

    LogFlowFunc(("Returning %Rhrc (%Rrc)\n", hrc, vrc));
    return hrc;
}

/** @copydoc GuestSessionTask::Run */
int GuestSessionTaskCopyTo::Run(void)
{
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(mSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int vrc = VINF_SUCCESS;

    FsLists::const_iterator itList = mVecLists.begin();
    while (itList != mVecLists.end())
    {
        FsList *pList = *itList;
        AssertPtr(pList);

        Utf8Str strSrcRootAbs = pList->mSrcRootAbs;
        Utf8Str strDstRootAbs = pList->mDstRootAbs;

        bool     fCopyIntoExisting = false;
        bool     fFollowSymlinks   = false;
        uint32_t fDirMode          = 0700; /** @todo Play safe by default; implement ACLs. */

        GuestFsObjData dstObjData;
        int vrcGuest;
        vrc = mSession->i_fsQueryInfo(strDstRootAbs, pList->mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_FollowLinks,
                                      dstObjData, &vrcGuest);
        if (RT_FAILURE(vrc))
        {
            if (vrc == VERR_GSTCTL_GUEST_ERROR)
            {
                switch (vrcGuest)
                {
                    case VERR_PATH_NOT_FOUND:
                        RT_FALL_THROUGH();
                    case VERR_FILE_NOT_FOUND:
                        /* We will deal with this down below. */
                        vrc = VINF_SUCCESS;
                        break;
                    default:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(tr("Querying information on guest for '%s' failed: %Rrc"),
                                            strDstRootAbs.c_str(), vrcGuest));
                        break;
                }
            }
            else
            {
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Querying information on guest for '%s' failed: %Rrc"),
                                               strDstRootAbs.c_str(), vrc));
                break;
            }
        }

        char szPath[RTPATH_MAX];

        LogFlowFunc(("List inital: rc=%Rrc, srcRootAbs=%s, dstRootAbs=%s\n",
                     vrc, strSrcRootAbs.c_str(), strDstRootAbs.c_str()));

        /* Calculated file copy flags for the current source spec. */
        FileCopyFlag_T fFileCopyFlags = FileCopyFlag_None;

        /* Create the root directory. */
        if (pList->mSourceSpec.enmType == FsObjType_Directory)
        {
            fCopyIntoExisting = RT_BOOL(pList->mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_CopyIntoExisting);
            fFollowSymlinks   = RT_BOOL(pList->mSourceSpec.Type.Dir.fCopyFlags & DirectoryCopyFlag_FollowLinks);

            LogFlowFunc(("Directory: fDirCopyFlags=%#x, fCopyIntoExisting=%RTbool, fFollowSymlinks=%RTbool\n",
                         pList->mSourceSpec.Type.Dir.fCopyFlags, fCopyIntoExisting, fFollowSymlinks));

            /* If the directory on the guest already exists, append the name of the root source directory to it. */
            switch (dstObjData.mType)
            {
                case FsObjType_Directory:
                {
                    if (fCopyIntoExisting)
                    {
                        /* Build the destination path on the guest. */
                        vrc = RTStrCopy(szPath, sizeof(szPath), strDstRootAbs.c_str());
                        if (RT_SUCCESS(vrc))
                        {
                            vrc = RTPathAppend(szPath, sizeof(szPath), RTPathFilenameEx(strSrcRootAbs.c_str(), mfPathStyle));
                            if (RT_SUCCESS(vrc))
                                strDstRootAbs = szPath;
                        }
                    }
                    else
                    {
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(tr("Guest directory \"%s\" already exists"),
                                                       strDstRootAbs.c_str()));
                        vrc = VERR_ALREADY_EXISTS;
                    }
                    break;
                }

                case FsObjType_File:
                    RT_FALL_THROUGH();
                case FsObjType_Symlink:
                    /* Nothing to do. */
                    break;

                default:
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(tr("Unknown object type (%#x) on guest for \"%s\""),
                                                   dstObjData.mType, strDstRootAbs.c_str()));
                    vrc = VERR_NOT_SUPPORTED;
                    break;
            }

            /* Make sure the destination root directory exists. */
            if (   RT_SUCCESS(vrc)
                && pList->mSourceSpec.fDryRun == false)
            {
                vrc = directoryCreateOnGuest(strDstRootAbs, DirectoryCreateFlag_None, fDirMode,
                                             fFollowSymlinks, true /* fCanExist */);
            }

            /* No tweaking of fFileCopyFlags needed here. */
        }
        else if (pList->mSourceSpec.enmType == FsObjType_File)
        {
            fCopyIntoExisting = !(pList->mSourceSpec.Type.File.fCopyFlags & FileCopyFlag_NoReplace);
            fFollowSymlinks   = RT_BOOL(pList->mSourceSpec.Type.File.fCopyFlags & FileCopyFlag_FollowLinks);

            LogFlowFunc(("File: fFileCopyFlags=%#x, fCopyIntoExisting=%RTbool, fFollowSymlinks=%RTbool\n",
                         pList->mSourceSpec.Type.File.fCopyFlags, fCopyIntoExisting, fFollowSymlinks));

            fFileCopyFlags = pList->mSourceSpec.Type.File.fCopyFlags; /* Just use the flags directly from the spec. */
        }
        else
            AssertFailedStmt(vrc = VERR_NOT_SUPPORTED);

        LogFlowFunc(("List final: rc=%Rrc, srcRootAbs=%s, dstRootAbs=%s, fFileCopyFlags=%#x\n",
                     vrc, strSrcRootAbs.c_str(), strDstRootAbs.c_str(), fFileCopyFlags));

        LogRel2(("Guest Control: Copying '%s' from host to '%s' on guest ...\n", strSrcRootAbs.c_str(), strDstRootAbs.c_str()));

        if (RT_FAILURE(vrc))
            break;

        FsEntries::const_iterator itEntry = pList->mVecEntries.begin();
        while (   RT_SUCCESS(vrc)
               && itEntry != pList->mVecEntries.end())
        {
            FsEntry *pEntry = *itEntry;
            AssertPtr(pEntry);

            Utf8Str strSrcAbs = strSrcRootAbs;
            Utf8Str strDstAbs = strDstRootAbs;

            if (pList->mSourceSpec.enmType == FsObjType_Directory)
            {
                /* Build the final (absolute) source path (on the host). */
                vrc = RTStrCopy(szPath, sizeof(szPath), strSrcAbs.c_str());
                if (RT_SUCCESS(vrc))
                {
                    vrc = RTPathAppend(szPath, sizeof(szPath), pEntry->strPath.c_str());
                    if (RT_SUCCESS(vrc))
                        strSrcAbs = szPath;
                }

                if (RT_FAILURE(vrc))
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(tr("Building source host path for entry \"%s\" failed (%Rrc)"),
                                                   pEntry->strPath.c_str(), vrc));
            }

            /** @todo Handle stuff like "C:" for destination, where the destination will be the CWD for drive C. */
            if (dstObjData.mType == FsObjType_Directory)
            {
                /* Build the final (absolute) destination path (on the guest). */
                vrc = RTStrCopy(szPath, sizeof(szPath), strDstAbs.c_str());
                if (RT_SUCCESS(vrc))
                {
                    vrc = RTPathAppend(szPath, sizeof(szPath), pEntry->strPath.c_str());
                    if (RT_SUCCESS(vrc))
                        strDstAbs = szPath;
                }

                if (RT_FAILURE(vrc))
                    setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                        Utf8StrFmt(tr("Building destination guest path for entry \"%s\" failed (%Rrc)"),
                                                   pEntry->strPath.c_str(), vrc));
            }

            mProgress->SetNextOperation(Bstr(strSrcAbs).raw(), 1);

            LogRel2(("Guest Control: Copying '%s' from host to '%s' on guest ...\n", strSrcAbs.c_str(), strDstAbs.c_str()));

            switch (pEntry->fMode & RTFS_TYPE_MASK)
            {
                case RTFS_TYPE_DIRECTORY:
                {
                    if (!pList->mSourceSpec.fDryRun)
                        vrc = directoryCreateOnGuest(strDstAbs, DirectoryCreateFlag_None, fDirMode,
                                                     fFollowSymlinks, fCopyIntoExisting);
                    break;
                }

                case RTFS_TYPE_FILE:
                {
                    if (!pList->mSourceSpec.fDryRun)
                        vrc = fileCopyToGuest(strSrcAbs, strDstAbs, fFileCopyFlags);
                    break;
                }

                default:
                    LogRel2(("Guest Control: Warning: Type 0x%x for '%s' is not supported, skipping\n",
                             pEntry->fMode & RTFS_TYPE_MASK, strSrcAbs.c_str()));
                    break;
            }

            if (RT_FAILURE(vrc))
                break;

            ++itEntry;
        }

        if (RT_FAILURE(vrc))
            break;

        ++itList;
    }

    if (RT_SUCCESS(vrc))
        vrc = setProgressSuccess();

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}

GuestSessionTaskUpdateAdditions::GuestSessionTaskUpdateAdditions(GuestSession *pSession,
                                                                 const Utf8Str &strSource,
                                                                 const ProcessArguments &aArguments,
                                                                 uint32_t fFlags)
                                                                 : GuestSessionTask(pSession)
{
    m_strTaskName = "gctlUpGA";

    mSource    = strSource;
    mArguments = aArguments;
    mFlags     = fFlags;
}

GuestSessionTaskUpdateAdditions::~GuestSessionTaskUpdateAdditions(void)
{

}

/**
 * Adds arguments to existing process arguments.
 * Identical / already existing arguments will be filtered out.
 *
 * @returns VBox status code.
 * @param   aArgumentsDest      Destination to add arguments to.
 * @param   aArgumentsSource    Arguments to add.
 */
int GuestSessionTaskUpdateAdditions::addProcessArguments(ProcessArguments &aArgumentsDest, const ProcessArguments &aArgumentsSource)
{
    try
    {
        /* Filter out arguments which already are in the destination to
         * not end up having them specified twice. Not the fastest method on the
         * planet but does the job. */
        ProcessArguments::const_iterator itSource = aArgumentsSource.begin();
        while (itSource != aArgumentsSource.end())
        {
            bool fFound = false;
            ProcessArguments::iterator itDest = aArgumentsDest.begin();
            while (itDest != aArgumentsDest.end())
            {
                if ((*itDest).equalsIgnoreCase((*itSource)))
                {
                    fFound = true;
                    break;
                }
                ++itDest;
            }

            if (!fFound)
                aArgumentsDest.push_back((*itSource));

            ++itSource;
        }
    }
    catch(std::bad_alloc &)
    {
        return VERR_NO_MEMORY;
    }

    return VINF_SUCCESS;
}

/**
 * Helper function to copy a file from a VISO to the guest.
 *
 * @returns VBox status code.
 * @param   pSession            Guest session to use.
 * @param   hVfsIso             VISO handle to use.
 * @param   strFileSrc          Source file path on VISO to copy.
 * @param   strFileDst          Destination file path on guest.
 * @param   fOptional           When set to \c true, the file is optional, i.e. can be skipped
 *                              when not found, \c false if not.
 */
int GuestSessionTaskUpdateAdditions::copyFileToGuest(GuestSession *pSession, RTVFS hVfsIso,
                                                     Utf8Str const &strFileSrc, const Utf8Str &strFileDst, bool fOptional)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertReturn(hVfsIso != NIL_RTVFS, VERR_INVALID_POINTER);

    RTVFSFILE hVfsFile = NIL_RTVFSFILE;
    int vrc = RTVfsFileOpen(hVfsIso, strFileSrc.c_str(), RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE, &hVfsFile);
    if (RT_SUCCESS(vrc))
    {
        uint64_t cbSrcSize = 0;
        vrc = RTVfsFileQuerySize(hVfsFile, &cbSrcSize);
        if (RT_SUCCESS(vrc))
        {
            LogRel(("Copying Guest Additions installer file \"%s\" to \"%s\" on guest ...\n",
                    strFileSrc.c_str(), strFileDst.c_str()));

            GuestFileOpenInfo dstOpenInfo;
            dstOpenInfo.mFilename    = strFileDst;
            dstOpenInfo.mOpenAction  = FileOpenAction_CreateOrReplace;
            dstOpenInfo.mAccessMode  = FileAccessMode_WriteOnly;
            dstOpenInfo.mSharingMode = FileSharingMode_All; /** @todo Use _Read when implemented. */

            ComObjPtr<GuestFile> dstFile;
            int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
            vrc = mSession->i_fileOpen(dstOpenInfo, dstFile, &vrcGuest);
            if (RT_FAILURE(vrc))
            {
                switch (vrc)
                {
                    case VERR_GSTCTL_GUEST_ERROR:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR, GuestFile::i_guestErrorToString(vrcGuest, strFileDst.c_str()));
                        break;

                    default:
                        setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                            Utf8StrFmt(tr("Guest file \"%s\" could not be opened: %Rrc"),
                                                       strFileDst.c_str(), vrc));
                        break;
                }
            }
            else
            {
                vrc = fileCopyToGuestInner(strFileSrc, hVfsFile, strFileDst, dstFile, FileCopyFlag_None, 0 /*offCopy*/, cbSrcSize);

                int vrc2 = dstFile->i_closeFile(&vrcGuest);
                AssertRC(vrc2);
            }
        }

        RTVfsFileRelease(hVfsFile);
    }
    else if (fOptional)
        vrc = VINF_SUCCESS;

    return vrc;
}

/**
 * Helper function to run (start) a file on the guest.
 *
 * @returns VBox status code.
 * @param   pSession            Guest session to use.
 * @param   procInfo            Guest process startup info to use.
 */
int GuestSessionTaskUpdateAdditions::runFileOnGuest(GuestSession *pSession, GuestProcessStartupInfo &procInfo)
{
    AssertPtrReturn(pSession, VERR_INVALID_POINTER);

    LogRel(("Running %s ...\n", procInfo.mName.c_str()));

    GuestProcessTool procTool;
    int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
    int vrc = procTool.init(pSession, procInfo, false /* Async */, &vrcGuest);
    if (RT_SUCCESS(vrc))
    {
        if (RT_SUCCESS(vrcGuest))
            vrc = procTool.wait(GUESTPROCESSTOOL_WAIT_FLAG_NONE, &vrcGuest);
        if (RT_SUCCESS(vrc))
            vrc = procTool.getTerminationStatus();
    }

    if (RT_FAILURE(vrc))
    {
        switch (vrc)
        {
            case VERR_GSTCTL_PROCESS_EXIT_CODE:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Running update file \"%s\" on guest failed: %Rrc"),
                                               procInfo.mExecutable.c_str(), procTool.getRc()));
                break;

            case VERR_GSTCTL_GUEST_ERROR:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR, tr("Running update file on guest failed"),
                                    GuestErrorInfo(GuestErrorInfo::Type_Process, vrcGuest, procInfo.mExecutable.c_str()));
                break;

            case VERR_INVALID_STATE: /** @todo Special guest control rc needed! */
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Update file \"%s\" reported invalid running state"),
                                               procInfo.mExecutable.c_str()));
                break;

            default:
                setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                    Utf8StrFmt(tr("Error while running update file \"%s\" on guest: %Rrc"),
                                               procInfo.mExecutable.c_str(), vrc));
                break;
        }
    }

    return vrc;
}

/** @copydoc GuestSessionTask::Run */
int GuestSessionTaskUpdateAdditions::Run(void)
{
    LogFlowThisFuncEnter();

    ComObjPtr<GuestSession> pSession = mSession;
    Assert(!pSession.isNull());

    AutoCaller autoCaller(pSession);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int vrc = setProgress(10);
    if (RT_FAILURE(vrc))
        return vrc;

    HRESULT hrc = S_OK;

    LogRel(("Automatic update of Guest Additions started, using \"%s\"\n", mSource.c_str()));

    ComObjPtr<Guest> pGuest(mSession->i_getParent());
#if 0
    /*
     * Wait for the guest being ready within 30 seconds.
     */
    AdditionsRunLevelType_T addsRunLevel;
    uint64_t tsStart = RTTimeSystemMilliTS();
    while (   SUCCEEDED(hrc = pGuest->COMGETTER(AdditionsRunLevel)(&addsRunLevel))
           && (    addsRunLevel != AdditionsRunLevelType_Userland
                && addsRunLevel != AdditionsRunLevelType_Desktop))
    {
        if ((RTTimeSystemMilliTS() - tsStart) > 30 * 1000)
        {
            vrc = VERR_TIMEOUT;
            break;
        }

        RTThreadSleep(100); /* Wait a bit. */
    }

    if (FAILED(hrc)) vrc = VERR_TIMEOUT;
    if (vrc == VERR_TIMEOUT)
        hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                  Utf8StrFmt(tr("Guest Additions were not ready within time, giving up")));
#else
    /*
     * For use with the GUI we don't want to wait, just return so that the manual .ISO mounting
     * can continue.
     */
    AdditionsRunLevelType_T addsRunLevel;
    if (   FAILED(hrc = pGuest->COMGETTER(AdditionsRunLevel)(&addsRunLevel))
        || (   addsRunLevel != AdditionsRunLevelType_Userland
            && addsRunLevel != AdditionsRunLevelType_Desktop))
    {
        if (addsRunLevel == AdditionsRunLevelType_System)
            hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                      Utf8StrFmt(tr("Guest Additions are installed but not fully loaded yet, aborting automatic update")));
        else
            hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                      Utf8StrFmt(tr("Guest Additions not installed or ready, aborting automatic update")));
        vrc = VERR_NOT_SUPPORTED;
    }
#endif

    if (RT_SUCCESS(vrc))
    {
        /*
         * Determine if we are able to update automatically. This only works
         * if there are recent Guest Additions installed already.
         */
        Utf8Str strAddsVer;
        vrc = getGuestProperty(pGuest, "/VirtualBox/GuestAdd/Version", strAddsVer);
        if (   RT_SUCCESS(vrc)
            && RTStrVersionCompare(strAddsVer.c_str(), "4.1") < 0)
        {
            hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                      Utf8StrFmt(tr("Guest has too old Guest Additions (%s) installed for automatic updating, please update manually"),
                                                 strAddsVer.c_str()));
            vrc = VERR_NOT_SUPPORTED;
        }
    }

    Utf8Str strOSVer;
    eOSType osType = eOSType_Unknown;
    if (RT_SUCCESS(vrc))
    {
        /*
         * Determine guest OS type and the required installer image.
         */
        Utf8Str strOSType;
        vrc = getGuestProperty(pGuest, "/VirtualBox/GuestInfo/OS/Product", strOSType);
        if (RT_SUCCESS(vrc))
        {
            if (   strOSType.contains("Microsoft", Utf8Str::CaseInsensitive)
                || strOSType.contains("Windows", Utf8Str::CaseInsensitive))
            {
                osType = eOSType_Windows;

                /*
                 * Determine guest OS version.
                 */
                vrc = getGuestProperty(pGuest, "/VirtualBox/GuestInfo/OS/Release", strOSVer);
                if (RT_FAILURE(vrc))
                {
                    hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                              Utf8StrFmt(tr("Unable to detected guest OS version, please update manually")));
                    vrc = VERR_NOT_SUPPORTED;
                }

                /* Because Windows 2000 + XP and is bitching with WHQL popups even if we have signed drivers we
                 * can't do automated updates here. */
                /* Windows XP 64-bit (5.2) is a Windows 2003 Server actually, so skip this here. */
                if (   RT_SUCCESS(vrc)
                    && RTStrVersionCompare(strOSVer.c_str(), "5.0") >= 0)
                {
                    if (   strOSVer.startsWith("5.0") /* Exclude the build number. */
                        || strOSVer.startsWith("5.1") /* Exclude the build number. */)
                    {
                        /* If we don't have AdditionsUpdateFlag_WaitForUpdateStartOnly set we can't continue
                         * because the Windows Guest Additions installer will fail because of WHQL popups. If the
                         * flag is set this update routine ends successfully as soon as the installer was started
                         * (and the user has to deal with it in the guest). */
                        if (!(mFlags & AdditionsUpdateFlag_WaitForUpdateStartOnly))
                        {
                            hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                                      Utf8StrFmt(tr("Windows 2000 and XP are not supported for automatic updating due to WHQL interaction, please update manually")));
                            vrc = VERR_NOT_SUPPORTED;
                        }
                    }
                }
                else
                {
                    hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                              Utf8StrFmt(tr("%s (%s) not supported for automatic updating, please update manually"),
                                                         strOSType.c_str(), strOSVer.c_str()));
                    vrc = VERR_NOT_SUPPORTED;
                }
            }
            else if (strOSType.contains("Solaris", Utf8Str::CaseInsensitive))
            {
                osType = eOSType_Solaris;
            }
            else /* Everything else hopefully means Linux :-). */
                osType = eOSType_Linux;

            if (   RT_SUCCESS(vrc)
                && (   osType != eOSType_Windows
                    && osType != eOSType_Linux))
                /** @todo Support Solaris. */
            {
                hrc = setProgressErrorMsg(VBOX_E_NOT_SUPPORTED,
                                          Utf8StrFmt(tr("Detected guest OS (%s) does not support automatic Guest Additions updating, please update manually"),
                                                     strOSType.c_str()));
                vrc = VERR_NOT_SUPPORTED;
            }
        }
    }

    if (RT_SUCCESS(vrc))
    {
        /*
         * Try to open the .ISO file to extract all needed files.
         */
        RTVFSFILE hVfsFileIso;
        vrc = RTVfsFileOpenNormal(mSource.c_str(), RTFILE_O_OPEN | RTFILE_O_READ | RTFILE_O_DENY_WRITE, &hVfsFileIso);
        if (RT_FAILURE(vrc))
        {
            hrc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                      Utf8StrFmt(tr("Unable to open Guest Additions .ISO file \"%s\": %Rrc"),
                                                 mSource.c_str(), vrc));
        }
        else
        {
            RTVFS hVfsIso;
            vrc = RTFsIso9660VolOpen(hVfsFileIso, 0 /*fFlags*/, &hVfsIso, NULL);
            if (RT_FAILURE(vrc))
            {
                hrc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                          Utf8StrFmt(tr("Unable to open file as ISO 9660 file system volume: %Rrc"), vrc));
            }
            else
            {
                Utf8Str strUpdateDir;

                vrc = setProgress(5);
                if (RT_SUCCESS(vrc))
                {
                    /* Try getting the installed Guest Additions version to know whether we
                     * can install our temporary Guest Addition data into the original installation
                     * directory.
                     *
                     * Because versions prior to 4.2 had bugs wrt spaces in paths we have to choose
                     * a different location then.
                     */
                    bool fUseInstallDir = false;

                    Utf8Str strAddsVer;
                    vrc = getGuestProperty(pGuest, "/VirtualBox/GuestAdd/Version", strAddsVer);
                    if (   RT_SUCCESS(vrc)
                        && RTStrVersionCompare(strAddsVer.c_str(), "4.2r80329") > 0)
                    {
                        fUseInstallDir = true;
                    }

                    if (fUseInstallDir)
                    {
                        vrc = getGuestProperty(pGuest, "/VirtualBox/GuestAdd/InstallDir", strUpdateDir);
                        if (RT_SUCCESS(vrc))
                        {
                            if (strUpdateDir.isNotEmpty())
                            {
                                if (osType == eOSType_Windows)
                                {
                                    strUpdateDir.findReplace('/', '\\');
                                    strUpdateDir.append("\\Update\\");
                                }
                                else
                                    strUpdateDir.append("/update/");
                            }
                            /* else Older Guest Additions might not handle this property correctly. */
                        }
                        /* Ditto. */
                    }

                    /** @todo Set fallback installation directory. Make this a *lot* smarter. Later. */
                    if (strUpdateDir.isEmpty())
                    {
                        if (osType == eOSType_Windows)
                            strUpdateDir = "C:\\Temp\\";
                        else
                            strUpdateDir = "/tmp/";
                    }
                }

                /* Create the installation directory. */
                int vrcGuest = VERR_IPE_UNINITIALIZED_STATUS;
                if (RT_SUCCESS(vrc))
                {
                    LogRel(("Guest Additions update directory is: %s\n", strUpdateDir.c_str()));

                    vrc = pSession->i_directoryCreate(strUpdateDir, 755 /* Mode */, DirectoryCreateFlag_Parents, &vrcGuest);
                    if (RT_FAILURE(vrc))
                    {
                        switch (vrc)
                        {
                            case VERR_GSTCTL_GUEST_ERROR:
                                hrc = setProgressErrorMsg(VBOX_E_IPRT_ERROR, tr("Creating installation directory on guest failed"),
                                                          GuestErrorInfo(GuestErrorInfo::Type_Directory, vrcGuest, strUpdateDir.c_str()));
                                break;

                            default:
                                hrc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                          Utf8StrFmt(tr("Creating installation directory \"%s\" on guest failed: %Rrc"),
                                                                     strUpdateDir.c_str(), vrc));
                                break;
                        }
                    }
                }

                if (RT_SUCCESS(vrc))
                    vrc = setProgress(10);

                if (RT_SUCCESS(vrc))
                {
                    /* Prepare the file(s) we want to copy over to the guest and
                     * (maybe) want to run. */
                    switch (osType)
                    {
                        case eOSType_Windows:
                        {
                            /* Do we need to install our certificates? We do this for W2K and up. */
                            bool fInstallCert = false;

                            /* Only Windows 2000 and up need certificates to be installed. */
                            if (RTStrVersionCompare(strOSVer.c_str(), "5.0") >= 0)
                            {
                                fInstallCert = true;
                                LogRel(("Certificates for auto updating WHQL drivers will be installed\n"));
                            }
                            else
                                LogRel(("Skipping installation of certificates for WHQL drivers\n"));

                            if (fInstallCert)
                            {
                                static struct { const char *pszDst, *pszIso; } const s_aCertFiles[] =
                                {
                                    { "vbox.cer",           "/CERT/VBOX.CER" },
                                    { "vbox-sha1.cer",      "/CERT/VBOX-SHA1.CER" },
                                    { "vbox-sha256.cer",    "/CERT/VBOX-SHA256.CER" },
                                    { "vbox-sha256-r3.cer", "/CERT/VBOX-SHA256-R3.CER" },
                                    { "oracle-vbox.cer",    "/CERT/ORACLE-VBOX.CER" },
                                };
                                uint32_t fCopyCertUtil = ISOFILE_FLAG_COPY_FROM_ISO;
                                for (uint32_t i = 0; i < RT_ELEMENTS(s_aCertFiles); i++)
                                {
                                    /* Skip if not present on the ISO. */
                                    RTFSOBJINFO ObjInfo;
                                    vrc = RTVfsQueryPathInfo(hVfsIso, s_aCertFiles[i].pszIso, &ObjInfo, RTFSOBJATTRADD_NOTHING,
                                                             RTPATH_F_ON_LINK);
                                    if (RT_FAILURE(vrc))
                                        continue;

                                    /* Copy the certificate certificate. */
                                    Utf8Str const strDstCert(strUpdateDir + s_aCertFiles[i].pszDst);
                                    mFiles.push_back(ISOFile(s_aCertFiles[i].pszIso,
                                                             strDstCert,
                                                             ISOFILE_FLAG_COPY_FROM_ISO | ISOFILE_FLAG_OPTIONAL));

                                    /* Out certificate installation utility. */
                                    /* First pass: Copy over the file (first time only) + execute it to remove any
                                     *             existing VBox certificates. */
                                    GuestProcessStartupInfo siCertUtilRem;
                                    siCertUtilRem.mName = "VirtualBox Certificate Utility, removing old VirtualBox certificates";
                                    /* The argv[0] should contain full path to the executable module */
                                    siCertUtilRem.mArguments.push_back(strUpdateDir + "VBoxCertUtil.exe");
                                    siCertUtilRem.mArguments.push_back(Utf8Str("remove-trusted-publisher"));
                                    siCertUtilRem.mArguments.push_back(Utf8Str("--root")); /* Add root certificate as well. */
                                    siCertUtilRem.mArguments.push_back(strDstCert);
                                    siCertUtilRem.mArguments.push_back(strDstCert);
                                    mFiles.push_back(ISOFile("CERT/VBOXCERTUTIL.EXE",
                                                             strUpdateDir + "VBoxCertUtil.exe",
                                                             fCopyCertUtil | ISOFILE_FLAG_EXECUTE | ISOFILE_FLAG_OPTIONAL,
                                                             siCertUtilRem));
                                    fCopyCertUtil = 0;
                                    /* Second pass: Only execute (but don't copy) again, this time installng the
                                     *              recent certificates just copied over. */
                                    GuestProcessStartupInfo siCertUtilAdd;
                                    siCertUtilAdd.mName = "VirtualBox Certificate Utility, installing VirtualBox certificates";
                                    /* The argv[0] should contain full path to the executable module */
                                    siCertUtilAdd.mArguments.push_back(strUpdateDir + "VBoxCertUtil.exe");
                                    siCertUtilAdd.mArguments.push_back(Utf8Str("add-trusted-publisher"));
                                    siCertUtilAdd.mArguments.push_back(Utf8Str("--root")); /* Add root certificate as well. */
                                    siCertUtilAdd.mArguments.push_back(strDstCert);
                                    siCertUtilAdd.mArguments.push_back(strDstCert);
                                    mFiles.push_back(ISOFile("CERT/VBOXCERTUTIL.EXE",
                                                             strUpdateDir + "VBoxCertUtil.exe",
                                                             ISOFILE_FLAG_EXECUTE | ISOFILE_FLAG_OPTIONAL,
                                                             siCertUtilAdd));
                                }
                            }
                            /* The installers in different flavors, as we don't know (and can't assume)
                             * the guest's bitness. */
                            mFiles.push_back(ISOFile("VBOXWINDOWSADDITIONS-X86.EXE",
                                                     strUpdateDir + "VBoxWindowsAdditions-x86.exe",
                                                     ISOFILE_FLAG_COPY_FROM_ISO));
                            mFiles.push_back(ISOFile("VBOXWINDOWSADDITIONS-AMD64.EXE",
                                                     strUpdateDir + "VBoxWindowsAdditions-amd64.exe",
                                                     ISOFILE_FLAG_COPY_FROM_ISO));
                            /* The stub loader which decides which flavor to run. */
                            GuestProcessStartupInfo siInstaller;
                            siInstaller.mName = "VirtualBox Windows Guest Additions Installer";
                            /* Set a running timeout of 5 minutes -- the Windows Guest Additions
                             * setup can take quite a while, so be on the safe side. */
                            siInstaller.mTimeoutMS = 5 * 60 * 1000;

                            /* The argv[0] should contain full path to the executable module */
                            siInstaller.mArguments.push_back(strUpdateDir + "VBoxWindowsAdditions.exe");
                            siInstaller.mArguments.push_back(Utf8Str("/S")); /* We want to install in silent mode. */
                            siInstaller.mArguments.push_back(Utf8Str("/l")); /* ... and logging enabled. */
                            /* Don't quit VBoxService during upgrade because it still is used for this
                             * piece of code we're in right now (that is, here!) ... */
                            siInstaller.mArguments.push_back(Utf8Str("/no_vboxservice_exit"));
                            /* Tell the installer to report its current installation status
                             * using a running VBoxTray instance via balloon messages in the
                             * Windows taskbar. */
                            siInstaller.mArguments.push_back(Utf8Str("/post_installstatus"));
                            /* Add optional installer command line arguments from the API to the
                             * installer's startup info. */
                            vrc = addProcessArguments(siInstaller.mArguments, mArguments);
                            AssertRC(vrc);
                            /* If the caller does not want to wait for out guest update process to end,
                             * complete the progress object now so that the caller can do other work. */
                            if (mFlags & AdditionsUpdateFlag_WaitForUpdateStartOnly)
                                siInstaller.mFlags |= ProcessCreateFlag_WaitForProcessStartOnly;
                            mFiles.push_back(ISOFile("VBOXWINDOWSADDITIONS.EXE",
                                                     strUpdateDir + "VBoxWindowsAdditions.exe",
                                                     ISOFILE_FLAG_COPY_FROM_ISO | ISOFILE_FLAG_EXECUTE, siInstaller));
                            break;
                        }
                        case eOSType_Linux:
                        {
                            /* Copy over the installer to the guest but don't execute it.
                             * Execution will be done by the shell instead. */
                            mFiles.push_back(ISOFile("VBOXLINUXADDITIONS.RUN",
                                                     strUpdateDir + "VBoxLinuxAdditions.run", ISOFILE_FLAG_COPY_FROM_ISO));

                            GuestProcessStartupInfo siInstaller;
                            siInstaller.mName = "VirtualBox Linux Guest Additions Installer";
                            /* Set a running timeout of 5 minutes -- compiling modules and stuff for the Linux Guest Additions
                             * setup can take quite a while, so be on the safe side. */
                            siInstaller.mTimeoutMS = 5 * 60 * 1000;
                            /* The argv[0] should contain full path to the shell we're using to execute the installer. */
                            siInstaller.mArguments.push_back("/bin/sh");
                            /* Now add the stuff we need in order to execute the installer.  */
                            siInstaller.mArguments.push_back(strUpdateDir + "VBoxLinuxAdditions.run");
                            /* Make sure to add "--nox11" to the makeself wrapper in order to not getting any blocking xterm
                             * window spawned when doing any unattended Linux GA installations. */
                            siInstaller.mArguments.push_back("--nox11");
                            siInstaller.mArguments.push_back("--");
                            /* Force the upgrade. Needed in order to skip the confirmation dialog about warning to upgrade. */
                            siInstaller.mArguments.push_back("--force"); /** @todo We might want a dedicated "--silent" switch here. */
                            /* If the caller does not want to wait for out guest update process to end,
                             * complete the progress object now so that the caller can do other work. */
                            if (mFlags & AdditionsUpdateFlag_WaitForUpdateStartOnly)
                                siInstaller.mFlags |= ProcessCreateFlag_WaitForProcessStartOnly;
                            mFiles.push_back(ISOFile("/bin/sh" /* Source */, "/bin/sh" /* Dest */,
                                                     ISOFILE_FLAG_EXECUTE, siInstaller));
                            break;
                        }
                        case eOSType_Solaris:
                            /** @todo Add Solaris support. */
                            break;
                        default:
                            AssertReleaseMsgFailed(("Unsupported guest type: %d\n", osType));
                            break;
                    }
                }

                if (RT_SUCCESS(vrc))
                {
                    /* We want to spend 40% total for all copying operations. So roughly
                     * calculate the specific percentage step of each copied file. */
                    uint8_t uOffset = 20; /* Start at 20%. */
                    uint8_t uStep = 40 / (uint8_t)mFiles.size(); Assert(mFiles.size() <= 10);

                    LogRel(("Copying over Guest Additions update files to the guest ...\n"));

                    std::vector<ISOFile>::const_iterator itFiles = mFiles.begin();
                    while (itFiles != mFiles.end())
                    {
                        if (itFiles->fFlags & ISOFILE_FLAG_COPY_FROM_ISO)
                        {
                            bool fOptional = false;
                            if (itFiles->fFlags & ISOFILE_FLAG_OPTIONAL)
                                fOptional = true;
                            vrc = copyFileToGuest(pSession, hVfsIso, itFiles->strSource, itFiles->strDest, fOptional);
                            if (RT_FAILURE(vrc))
                            {
                                hrc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                                          Utf8StrFmt(tr("Error while copying file \"%s\" to \"%s\" on the guest: %Rrc"),
                                                                     itFiles->strSource.c_str(), itFiles->strDest.c_str(), vrc));
                                break;
                            }
                        }

                        vrc = setProgress(uOffset);
                        if (RT_FAILURE(vrc))
                            break;
                        uOffset += uStep;

                        ++itFiles;
                    }
                }

                /* Done copying, close .ISO file. */
                RTVfsRelease(hVfsIso);

                if (RT_SUCCESS(vrc))
                {
                    /* We want to spend 35% total for all copying operations. So roughly
                     * calculate the specific percentage step of each copied file. */
                    uint8_t uOffset = 60; /* Start at 60%. */
                    uint8_t uStep = 35 / (uint8_t)mFiles.size(); Assert(mFiles.size() <= 10);

                    LogRel(("Executing Guest Additions update files ...\n"));

                    std::vector<ISOFile>::iterator itFiles = mFiles.begin();
                    while (itFiles != mFiles.end())
                    {
                        if (itFiles->fFlags & ISOFILE_FLAG_EXECUTE)
                        {
                            vrc = runFileOnGuest(pSession, itFiles->mProcInfo);
                            if (RT_FAILURE(vrc))
                                break;
                        }

                        vrc = setProgress(uOffset);
                        if (RT_FAILURE(vrc))
                            break;
                        uOffset += uStep;

                        ++itFiles;
                    }
                }

                if (RT_SUCCESS(vrc))
                {
                    LogRel(("Automatic update of Guest Additions succeeded\n"));
                    vrc = setProgressSuccess();
                }
            }

            RTVfsFileRelease(hVfsFileIso);
        }
    }

    if (RT_FAILURE(vrc))
    {
        if (vrc == VERR_CANCELLED)
        {
            LogRel(("Automatic update of Guest Additions was canceled\n"));

            hrc = setProgressErrorMsg(VBOX_E_IPRT_ERROR,
                                      Utf8StrFmt(tr("Installation was canceled")));
        }
        else
        {
            Utf8Str strError = Utf8StrFmt("No further error information available (%Rrc)", vrc);
            if (!mProgress.isNull()) /* Progress object is optional. */
            {
#ifdef VBOX_STRICT
                /* If we forgot to set the progress object accordingly, let us know. */
                LONG rcProgress;
                AssertMsg(   SUCCEEDED(mProgress->COMGETTER(ResultCode(&rcProgress)))
                          && FAILED(rcProgress), ("Task indicated an error (%Rrc), but progress did not indicate this (%Rhrc)\n",
                                                  vrc, rcProgress));
#endif
                com::ProgressErrorInfo errorInfo(mProgress);
                if (   errorInfo.isFullAvailable()
                    || errorInfo.isBasicAvailable())
                {
                    strError = errorInfo.getText();
                }
            }

            LogRel(("Automatic update of Guest Additions failed: %s (%Rhrc)\n",
                    strError.c_str(), hrc));
        }

        LogRel(("Please install Guest Additions manually\n"));
    }

    /** @todo Clean up copied / left over installation files. */

    LogFlowFuncLeaveRC(vrc);
    return vrc;
}
