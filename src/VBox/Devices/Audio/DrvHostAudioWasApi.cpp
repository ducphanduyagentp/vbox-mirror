/* $Id$ */
/** @file
 * Host audio driver - Windows Audio Session API.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DRV_HOST_AUDIO
/*#define INITGUID - defined in VBoxhostAudioDSound.cpp already */
#include <VBox/log.h>
#include <iprt/win/windows.h>
#include <Mmdeviceapi.h>
#include <iprt/win/audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <AudioSessionTypes.h>
#ifndef AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY
# define AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY UINT32_C(0x08000000)
#endif
#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
# define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM UINT32_C(0x80000000)
#endif

#include <VBox/vmm/pdmaudioinline.h>
#include <VBox/vmm/pdmaudiohostenuminline.h>

#include <iprt/rand.h>
#include <iprt/utf16.h>
#include <iprt/uuid.h>

#include <new> /* std::bad_alloc */

#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** Max GetCurrentPadding value we accept (to make sure it's safe to convert to bytes). */
#define VBOX_WASAPI_MAX_PADDING         UINT32_C(0x007fffff)

/** @name WM_DRVHOSTAUDIOWAS_XXX - Worker thread messages.
 * @{ */
/** Adds entry to the cache.
 * lParam points to a PDMAUDIOSTREAMCFG structure with the details. RTMemFree
 * when done. */
#define WM_DRVHOSTAUDIOWAS_HINT         (WM_APP + 2)
#define WM_DRVHOSTAUDIOWAS_PURGE_CACHE  (WM_APP + 1)
/** @} */


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
class DrvHostAudioWasMmNotifyClient;

/** Pointer to the cache entry for a host audio device (+dir). */
typedef struct DRVHOSTAUDIOWASCACHEDEV *PDRVHOSTAUDIOWASCACHEDEV;

/**
 * Cached pre-initialized audio client for a device.
 *
 * The activation and initialization of an IAudioClient has been observed to be
 * very very slow (> 100 ms) and not suitable to be done on an EMT.  So, we'll
 * pre-initialize the device clients at construction time and when the default
 * device changes to try avoid this problem.
 *
 * A client is returned to the cache after we're done with it, provided it still
 * works fine.
 */
typedef struct DRVHOSTAUDIOWASCACHEDEVCFG
{
    /** Entry in DRVHOSTAUDIOWASCACHEDEV::ConfigList. */
    RTLISTNODE                  ListEntry;
    /** The device.   */
    PDRVHOSTAUDIOWASCACHEDEV    pDevEntry;
    /** The cached audio client. */
    IAudioClient               *pIAudioClient;
    /** Output streams: The render client interface. */
    IAudioRenderClient         *pIAudioRenderClient;
    /** Input streams: The capture client interface. */
    IAudioCaptureClient        *pIAudioCaptureClient;
    /** The configuration. */
    PDMAUDIOPCMPROPS            Props;
    /** The buffer size in frames. */
    uint32_t                    cFramesBufferSize;
    /** The device/whatever period in frames. */
    uint32_t                    cFramesPeriod;
    /** The stringified properties. */
    char                        szProps[32];
} DRVHOSTAUDIOWASCACHEDEVCFG;
/** Pointer to a pre-initialized audio client. */
typedef DRVHOSTAUDIOWASCACHEDEVCFG *PDRVHOSTAUDIOWASCACHEDEVCFG;

/**
 * Per audio device (+ direction) cache entry.
 */
typedef struct DRVHOSTAUDIOWASCACHEDEV
{
    /** Entry in DRVHOSTAUDIOWAS::CacheHead. */
    RTLISTNODE                  ListEntry;
    /** The MM device associated with the stream. */
    IMMDevice                  *pIDevice;
    /** The direction of the device. */
    PDMAUDIODIR                 enmDir;
#if 0 /* According to https://social.msdn.microsoft.com/Forums/en-US/1d974d90-6636-4121-bba3-a8861d9ab92a,
         these were always support just missing from the SDK. */
    /** Support for AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM: -1=unknown, 0=no, 1=yes. */
    int8_t                      fSupportsAutoConvertPcm;
    /** Support for AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY: -1=unknown, 0=no, 1=yes. */
    int8_t                      fSupportsSrcDefaultQuality;
#endif
    /** List of cached configurations (DRVHOSTAUDIOWASCACHEDEVCFG). */
    RTLISTANCHOR                ConfigList;
    /** The device ID length in RTUTF16 units. */
    size_t                      cwcDevId;
    /** The device ID. */
    RTUTF16                     wszDevId[RT_FLEXIBLE_ARRAY];
} DRVHOSTAUDIOWASCACHEDEV;


/**
 * Data for a WASABI stream.
 */
typedef struct DRVHOSTAUDIOWASSTREAM
{
    /** Common part. */
    PDMAUDIOBACKENDSTREAM       Core;

    /** Entry in DRVHOSTAUDIOWAS::StreamHead. */
    RTLISTNODE                  ListEntry;
    /** The stream's acquired configuration. */
    PDMAUDIOSTREAMCFG           Cfg;
    /** Cache entry to be relased when destroying the stream. */
    PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg;

    /** Set if the stream is enabled. */
    bool                        fEnabled;
    /** Set if the stream is started (playing/capturing). */
    bool                        fStarted;
    /** Set if the stream is draining (output only). */
    bool                        fDraining;
    /** Set if we should restart the stream on resume (saved pause state). */
    bool                        fRestartOnResume;

    /** The RTTimeMilliTS() deadline for the draining of this stream (output). */
    uint64_t                    msDrainDeadline;
    /** Internal stream offset (bytes). */
    uint64_t                    offInternal;
    /** The RTTimeMilliTS() at the end of the last transfer. */
    uint64_t                    msLastTransfer;

    /** Input: Current capture buffer (advanced as we read). */
    uint8_t                    *pbCapture;
    /** Input: The number of bytes left in the current capture buffer. */
    uint32_t                    cbCapture;
    /** Input: The full size of what pbCapture is part of (for ReleaseBuffer). */
    uint32_t                    cFramesCaptureToRelease;

    /** Critical section protecting: . */
    RTCRITSECT                  CritSect;
    /** Buffer that drvHostWasStreamStatusString uses. */
    char                        szStatus[128];
} DRVHOSTAUDIOWASSTREAM;
/** Pointer to a WASABI stream. */
typedef DRVHOSTAUDIOWASSTREAM *PDRVHOSTAUDIOWASSTREAM;


/**
 * WASAPI-specific device entry.
 */
typedef struct DRVHOSTAUDIOWASDEV
{
    /** The core structure. */
    PDMAUDIOHOSTDEV         Core;
    /** The device ID (flexible length). */
    RTUTF16                 wszDevId[RT_FLEXIBLE_ARRAY];
} DRVHOSTAUDIOWASDEV;
/** Pointer to a DirectSound device entry. */
typedef DRVHOSTAUDIOWASDEV *PDRVHOSTAUDIOWASDEV;


/**
 * Data for a WASAPI host audio instance.
 */
typedef struct DRVHOSTAUDIOWAS
{
    /** The audio host audio interface we export. */
    PDMIHOSTAUDIO                   IHostAudio;
    /** Pointer to the PDM driver instance. */
    PPDMDRVINS                      pDrvIns;
    /** Audio device enumerator instance that we use for getting the default
     * devices (or specific ones if overriden by config).  Also used for
     * implementing enumeration. */
    IMMDeviceEnumerator            *pIEnumerator;
    /** Notification interface.   */
    PPDMIAUDIONOTIFYFROMHOST        pIAudioNotifyFromHost;
    /** The output device ID, NULL for default. */
    PRTUTF16                        pwszOutputDevId;
    /** The input device ID, NULL for default. */
    PRTUTF16                        pwszInputDevId;

    /** Pointer to the MM notification client instance. */
    DrvHostAudioWasMmNotifyClient  *pNotifyClient;
    /** The input device to use.  This can be NULL if there wasn't a suitable one
     * around when we last looked or if it got removed/disabled/whatever.
    * All access must be done inside the pNotifyClient critsect. */
    IMMDevice                      *pIDeviceInput;
    /** The output device to use.  This can be NULL if there wasn't a suitable one
     * around when we last looked or if it got removed/disabled/whatever.
     * All access must be done inside the pNotifyClient critsect. */
    IMMDevice                      *pIDeviceOutput;

    /** A drain stop timer that makes sure a draining stream will be properly
     * stopped (mainly for clean state and to reduce resource usage). */
    TMTIMERHANDLE                   hDrainTimer;
    /** List of streams (DRVHOSTAUDIOWASSTREAM).
     * Requires CritSect ownership.  */
    RTLISTANCHOR                    StreamHead;
    /** Serializing access to StreamHead. */
    RTCRITSECTRW                    CritSectStreamList;

    /** List of cached devices (DRVHOSTAUDIOWASCACHEDEV).
     * Protected by CritSectCache  */
    RTLISTANCHOR                    CacheHead;
    /** Serializing access to CacheHead. */
    RTCRITSECT                      CritSectCache;

    /** The worker thread. */
    RTTHREAD                        hWorkerThread;
    /** The TID of the worker thread (for posting messages to it). */
    DWORD                           idWorkerThread;
    /** The fixed wParam value for the worker thread. */
    WPARAM                          uWorkerThreadFixedParam;

} DRVHOSTAUDIOWAS;
/** Pointer to the data for a WASAPI host audio driver instance. */
typedef DRVHOSTAUDIOWAS *PDRVHOSTAUDIOWAS;




/**
 * Gets the stream status.
 *
 * @returns Pointer to stream status string.
 * @param   pStreamWas          The stream to get the status for.
 */
static const char *drvHostWasStreamStatusString(PDRVHOSTAUDIOWASSTREAM pStreamWas)
{
    static RTSTRTUPLE const s_aEnable[2] =
    {
        RT_STR_TUPLE("DISABLED"),
        RT_STR_TUPLE("ENABLED ")
    };
    PCRTSTRTUPLE pTuple = &s_aEnable[pStreamWas->fEnabled];
    memcpy(pStreamWas->szStatus, pTuple->psz, pTuple->cch);
    size_t off = pTuple->cch;

    static RTSTRTUPLE const s_aStarted[2] =
    {
        RT_STR_TUPLE(" STARTED"),
        RT_STR_TUPLE(" STOPPED")
    };
    pTuple = &s_aStarted[pStreamWas->fStarted];
    memcpy(&pStreamWas->szStatus[off], pTuple->psz, pTuple->cch);
    off += pTuple->cch;

    static RTSTRTUPLE const s_aDraining[2] =
    {
        RT_STR_TUPLE(""),
        RT_STR_TUPLE(" DRAINING")
    };
    pTuple = &s_aDraining[pStreamWas->fDraining];
    memcpy(&pStreamWas->szStatus[off], pTuple->psz, pTuple->cch);
    off += pTuple->cch;

    pStreamWas->szStatus[off] = '\0';
    return pStreamWas->szStatus;
}


/*********************************************************************************************************************************
*   IMMNotificationClient implementation
*********************************************************************************************************************************/
/**
 * Multimedia notification client.
 *
 * We want to know when the default device changes so we can switch running
 * streams to use the new one and so we can pre-activate it in preparation
 * for new streams.
 */
class DrvHostAudioWasMmNotifyClient : public IMMNotificationClient
{
private:
    /** Reference counter. */
    uint32_t volatile           m_cRefs;
    /** The WASAPI host audio driver instance data.
     * @note    This can be NULL.  Only access after entering critical section. */
    PDRVHOSTAUDIOWAS            m_pDrvWas;
    /** Critical section serializing access to m_pDrvWas.  */
    RTCRITSECT                  m_CritSect;

public:
    /**
     * @throws int on critical section init failure.
     */
    DrvHostAudioWasMmNotifyClient(PDRVHOSTAUDIOWAS a_pDrvWas)
        : m_cRefs(1)
        , m_pDrvWas(a_pDrvWas)
    {
        int rc = RTCritSectInit(&m_CritSect);
        AssertRCStmt(rc, throw(rc));
    }

    virtual ~DrvHostAudioWasMmNotifyClient() RT_NOEXCEPT
    {
        RTCritSectDelete(&m_CritSect);
    }

    /**
     * Called by drvHostAudioWasDestruct to set m_pDrvWas to NULL.
     */
    void notifyDriverDestroyed() RT_NOEXCEPT
    {
        RTCritSectEnter(&m_CritSect);
        m_pDrvWas = NULL;
        RTCritSectLeave(&m_CritSect);
    }

    /**
     * Enters the notification critsect for getting at the IMMDevice members in
     * PDMHOSTAUDIOWAS.
     */
    void lockEnter() RT_NOEXCEPT
    {
        RTCritSectEnter(&m_CritSect);
    }

    /**
     * Leaves the notification critsect.
     */
    void lockLeave() RT_NOEXCEPT
    {
        RTCritSectLeave(&m_CritSect);
    }

    /** @name IUnknown interface
     * @{ */
    IFACEMETHODIMP_(ULONG)  AddRef()
    {
        uint32_t cRefs = ASMAtomicIncU32(&m_cRefs);
        AssertMsg(cRefs < 64, ("%#x\n", cRefs));
        Log6Func(("returns %u\n", cRefs));
        return cRefs;
    }

    IFACEMETHODIMP_(ULONG)  Release()
    {
        uint32_t cRefs = ASMAtomicDecU32(&m_cRefs);
        AssertMsg(cRefs < 64, ("%#x\n", cRefs));
        if (cRefs == 0)
            delete this;
        Log6Func(("returns %u\n", cRefs));
        return cRefs;
    }

    IFACEMETHODIMP          QueryInterface(const IID &rIID, void **ppvInterface)
    {
        if (IsEqualIID(rIID, IID_IUnknown))
            *ppvInterface = static_cast<IUnknown *>(this);
        else if (IsEqualIID(rIID, __uuidof(IMMNotificationClient)))
            *ppvInterface = static_cast<IMMNotificationClient *>(this);
        else
        {
            LogFunc(("Unknown rIID={%RTuuid}\n", &rIID));
            *ppvInterface = NULL;
            return E_NOINTERFACE;
        }
        Log6Func(("returns S_OK + %p\n", *ppvInterface));
        return S_OK;
    }
    /** @} */

    /** @name IMMNotificationClient interface
     * @{ */
    IFACEMETHODIMP OnDeviceStateChanged(LPCWSTR pwszDeviceId, DWORD dwNewState)
    {
        RT_NOREF(pwszDeviceId, dwNewState);
        Log7Func(("pwszDeviceId=%ls dwNewState=%u (%#x)\n", pwszDeviceId, dwNewState, dwNewState));
        return S_OK;
    }

    IFACEMETHODIMP OnDeviceAdded(LPCWSTR pwszDeviceId)
    {
        RT_NOREF(pwszDeviceId);
        Log7Func(("pwszDeviceId=%ls\n", pwszDeviceId));

        /*
         * Is this a device we're interested in?  Grab the enumerator if it is.
         */
        bool                 fOutput      = false;
        IMMDeviceEnumerator *pIEnumerator = NULL;
        RTCritSectEnter(&m_CritSect);
        if (   m_pDrvWas != NULL
            && (   (fOutput = RTUtf16ICmp(m_pDrvWas->pwszOutputDevId, pwszDeviceId) == 0)
                || RTUtf16ICmp(m_pDrvWas->pwszInputDevId, pwszDeviceId) == 0))
        {
            pIEnumerator = m_pDrvWas->pIEnumerator;
            if (pIEnumerator /* paranoia */)
                pIEnumerator->AddRef();
        }
        RTCritSectLeave(&m_CritSect);
        if (pIEnumerator)
        {
            /*
             * Get the device and update it.
             */
            IMMDevice *pIDevice = NULL;
            HRESULT hrc = pIEnumerator->GetDevice(pwszDeviceId, &pIDevice);
            if (SUCCEEDED(hrc))
                setDevice(fOutput, pIDevice, pwszDeviceId, __PRETTY_FUNCTION__);
            else
                LogRelMax(64, ("WasAPI: Failed to get %s device '%ls' (OnDeviceAdded): %Rhrc\n",
                               fOutput ? "output" : "input", pwszDeviceId, hrc));
            pIEnumerator->Release();
        }
        return S_OK;
    }

    IFACEMETHODIMP OnDeviceRemoved(LPCWSTR pwszDeviceId)
    {
        RT_NOREF(pwszDeviceId);
        Log7Func(("pwszDeviceId=%ls\n", pwszDeviceId));

        /*
         * Is this a device we're interested in?  Then set it to NULL.
         */
        bool fOutput = false;
        RTCritSectEnter(&m_CritSect);
        if (   m_pDrvWas != NULL
            && (   (fOutput = RTUtf16ICmp(m_pDrvWas->pwszOutputDevId, pwszDeviceId) == 0)
                || RTUtf16ICmp(m_pDrvWas->pwszInputDevId, pwszDeviceId) == 0))
            setDevice(fOutput, NULL, pwszDeviceId, __PRETTY_FUNCTION__);
        RTCritSectLeave(&m_CritSect);
        return S_OK;
    }

    IFACEMETHODIMP OnDefaultDeviceChanged(EDataFlow enmFlow, ERole enmRole, LPCWSTR pwszDefaultDeviceId)
    {
        /*
         * Are we interested in this device?  If so grab the enumerator.
         */
        IMMDeviceEnumerator *pIEnumerator = NULL;
        RTCritSectEnter(&m_CritSect);
        if (    m_pDrvWas != NULL
            && (   (enmFlow == eRender  && enmRole == eMultimedia && !m_pDrvWas->pwszOutputDevId)
                || (enmFlow == eCapture && enmRole == eMultimedia && !m_pDrvWas->pwszInputDevId)))
        {
            pIEnumerator = m_pDrvWas->pIEnumerator;
            if (pIEnumerator /* paranoia */)
                pIEnumerator->AddRef();
        }
        RTCritSectLeave(&m_CritSect);
        if (pIEnumerator)
        {
            /*
             * Get the device and update it.
             */
            IMMDevice *pIDevice = NULL;
            HRESULT hrc = pIEnumerator->GetDefaultAudioEndpoint(enmFlow, enmRole, &pIDevice);
            if (SUCCEEDED(hrc))
                setDevice(enmFlow == eRender, pIDevice, pwszDefaultDeviceId, __PRETTY_FUNCTION__);
            else
                LogRelMax(64, ("WasAPI: Failed to get default %s device (OnDefaultDeviceChange): %Rhrc\n",
                               enmFlow == eRender ? "output" : "input", hrc));
            pIEnumerator->Release();
        }

        RT_NOREF(enmFlow, enmRole, pwszDefaultDeviceId);
        Log7Func(("enmFlow=%d enmRole=%d pwszDefaultDeviceId=%ls\n", enmFlow, enmRole, pwszDefaultDeviceId));
        return S_OK;
    }

    IFACEMETHODIMP OnPropertyValueChanged(LPCWSTR pwszDeviceId, const PROPERTYKEY Key)
    {
        RT_NOREF(pwszDeviceId, Key);
        Log7Func(("pwszDeviceId=%ls Key={%RTuuid, %u (%#x)}\n", pwszDeviceId, &Key.fmtid, Key.pid, Key.pid));
        return S_OK;
    }
    /** @} */

private:
    /**
     * Sets DRVHOSTAUDIOWAS::pIDeviceOutput or DRVHOSTAUDIOWAS::pIDeviceInput to @a pIDevice.
     */
    void setDevice(bool fOutput, IMMDevice *pIDevice, LPCWSTR pwszDeviceId, const char *pszCaller)
    {
        RT_NOREF(pszCaller, pwszDeviceId);

        RTCritSectEnter(&m_CritSect);
        if (m_pDrvWas)
        {
            if (fOutput)
            {
                Log7((LOG_FN_FMT ": Changing output device from %p to %p (%ls)\n",
                      pszCaller, m_pDrvWas->pIDeviceOutput, pIDevice, pwszDeviceId));
                if (m_pDrvWas->pIDeviceOutput)
                    m_pDrvWas->pIDeviceOutput->Release();
                m_pDrvWas->pIDeviceOutput = pIDevice;
            }
            else
            {
                Log7((LOG_FN_FMT ": Changing input device from %p to %p (%ls)\n",
                      pszCaller, m_pDrvWas->pIDeviceInput, pIDevice, pwszDeviceId));
                if (m_pDrvWas->pIDeviceInput)
                    m_pDrvWas->pIDeviceInput->Release();
                m_pDrvWas->pIDeviceInput = pIDevice;
            }

            /** @todo Invalid/update in-use streams. */
        }
        else if (pIDevice)
            pIDevice->Release();
        RTCritSectLeave(&m_CritSect);
    }
};


/*********************************************************************************************************************************
*   Pre-configured audio client cache.                                                                                           *
*********************************************************************************************************************************/
#define WAS_CACHE_MAX_ENTRIES_SAME_DEVICE   2

/**
 * Converts from PDM stream config to windows WAVEFORMATEX struct.
 *
 * @param   pCfg    The PDM audio stream config to convert from.
 * @param   pFmt    The windows structure to initialize.
 */
static void drvHostAudioWasWaveFmtExFromCfg(PCPDMAUDIOSTREAMCFG pCfg, PWAVEFORMATEX pFmt)
{
    RT_ZERO(*pFmt);
    pFmt->wFormatTag      = WAVE_FORMAT_PCM;
    pFmt->nChannels       = PDMAudioPropsChannels(&pCfg->Props);
    pFmt->wBitsPerSample  = PDMAudioPropsSampleBits(&pCfg->Props);
    pFmt->nSamplesPerSec  = PDMAudioPropsHz(&pCfg->Props);
    pFmt->nBlockAlign     = PDMAudioPropsFrameSize(&pCfg->Props);
    pFmt->nAvgBytesPerSec = PDMAudioPropsFramesToBytes(&pCfg->Props, PDMAudioPropsHz(&pCfg->Props));
    pFmt->cbSize          = 0; /* No extra data specified. */
}


/**
 * Converts from windows WAVEFORMATEX and stream props to PDM audio properties.
 *
 * @returns VINF_SUCCESS on success, VERR_AUDIO_STREAM_COULD_NOT_CREATE if not
 *          supported.
 * @param   pCfg        The stream configuration to update (input:
 *                      requested config; output: acquired).
 * @param   pFmt        The windows wave format structure.
 * @param   pszStream   The stream name for error logging.
 * @param   pwszDevId   The device ID for error logging.
 */
static int drvHostAudioWasCacheWaveFmtExToProps(PPDMAUDIOPCMPROPS pProps, WAVEFORMATEX const *pFmt,
                                                const char *pszStream, PCRTUTF16 pwszDevId)
{
    if (pFmt->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (   pFmt->wBitsPerSample == 8
            || pFmt->wBitsPerSample == 16
            || pFmt->wBitsPerSample == 32)
        {
            if (pFmt->nChannels > 0 && pFmt->nChannels < 16)
            {
                if (pFmt->nSamplesPerSec >= 4096 && pFmt->nSamplesPerSec <= 768000)
                {
                    PDMAudioPropsInit(pProps, pFmt->wBitsPerSample / 8, true /*fSigned*/, pFmt->nChannels, pFmt->nSamplesPerSec);
                    if (PDMAudioPropsFrameSize(pProps) == pFmt->nBlockAlign)
                        return VINF_SUCCESS;
                }
            }
        }
    }
    LogRelMax(64, ("WasAPI: Error! Unsupported stream format for '%s' suggested by '%ls':\n"
                   "WasAPI:   wFormatTag      = %RU16 (expected %d)\n"
                   "WasAPI:   nChannels       = %RU16 (expected 1..15)\n"
                   "WasAPI:   nSamplesPerSec  = %RU32 (expected 4096..768000)\n"
                   "WasAPI:   nAvgBytesPerSec = %RU32\n"
                   "WasAPI:   nBlockAlign     = %RU16\n"
                   "WasAPI:   wBitsPerSample  = %RU16 (expected 8, 16, or 32)\n"
                   "WasAPI:   cbSize          = %RU16\n",
                   pszStream, pwszDevId, pFmt->wFormatTag, WAVE_FORMAT_PCM, pFmt->nChannels, pFmt->nSamplesPerSec, pFmt->nAvgBytesPerSec,
                   pFmt->nBlockAlign, pFmt->wBitsPerSample, pFmt->cbSize));
    return VERR_AUDIO_STREAM_COULD_NOT_CREATE;
}


/**
 * Destroys a devie config cache entry.
 *
 * @param   pDevCfg     Device config entry.  Must not be in the list.
 */
static void drvHostAudioWasCacheDestroyDevConfig(PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg)
{
    uint32_t cTypeClientRefs = 0;
    if (pDevCfg->pIAudioCaptureClient)
    {
        cTypeClientRefs = pDevCfg->pIAudioCaptureClient->Release();
        pDevCfg->pIAudioCaptureClient = NULL;
    }

    if (pDevCfg->pIAudioRenderClient)
    {
        cTypeClientRefs = pDevCfg->pIAudioRenderClient->Release();
        pDevCfg->pIAudioRenderClient = NULL;
    }

    uint32_t cClientRefs = 0;
    if (pDevCfg->pIAudioClient /* paranoia */)
    {
        cClientRefs = pDevCfg->pIAudioClient->Release();
        pDevCfg->pIAudioClient = NULL;
    }

    Log8Func(("Destroying cache config entry: '%ls: %s' - cClientRefs=%u cTypeClientRefs=%u\n",
              pDevCfg->pDevEntry->wszDevId, pDevCfg->szProps, cClientRefs, cTypeClientRefs));
    RT_NOREF(cClientRefs, cTypeClientRefs);

    pDevCfg->pDevEntry = NULL;
    RTMemFree(pDevCfg);
}


/**
 * Destroys a device cache entry.
 *
 * @param   pDevEntry   The device entry. Must not be in the cache!
 */
static void drvHostAudioWasCacheDestroyDevEntry(PDRVHOSTAUDIOWASCACHEDEV pDevEntry)
{
    Log8Func(("Destroying cache entry: %p - '%ls'\n", pDevEntry, pDevEntry->wszDevId));

    PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg, pDevCfgNext;
    RTListForEachSafe(&pDevEntry->ConfigList, pDevCfg, pDevCfgNext, DRVHOSTAUDIOWASCACHEDEVCFG, ListEntry)
    {
        drvHostAudioWasCacheDestroyDevConfig(pDevCfg);
    }

    uint32_t cDevRefs = 0;
    if (pDevEntry->pIDevice /* paranoia */)
    {
        cDevRefs = pDevEntry->pIDevice->Release();
        pDevEntry->pIDevice = NULL;
    }

    pDevEntry->cwcDevId = 0;
    pDevEntry->wszDevId[0] = '\0';
    RTMemFree(pDevEntry);
    Log8Func(("Destroyed cache entry: %p cDevRefs=%u\n", pDevEntry, cDevRefs));
}


/**
 * Purges all the entries in the cache.
 */
static void drvHostAudioWasCachePurge(PDRVHOSTAUDIOWAS pThis)
{
    for (;;)
    {
        RTCritSectEnter(&pThis->CritSectCache);
        PDRVHOSTAUDIOWASCACHEDEV pDevEntry = RTListRemoveFirst(&pThis->CacheHead, DRVHOSTAUDIOWASCACHEDEV, ListEntry);
        RTCritSectLeave(&pThis->CritSectCache);
        if (!pDevEntry)
            break;
        drvHostAudioWasCacheDestroyDevEntry(pDevEntry);
    }
}


/**
 * Looks up a specific configuration.
 *
 * @returns Pointer to the device config (removed from cache) on success.  NULL
 *          if no matching config found.
 * @param   pDevEntry       Where to perform the lookup.
 * @param   pProp           The config properties to match.
 */
static PDRVHOSTAUDIOWASCACHEDEVCFG
drvHostAudioWasCacheLookupLocked(PDRVHOSTAUDIOWASCACHEDEV pDevEntry, PCPDMAUDIOPCMPROPS pProps)
{
    PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg;
    RTListForEach(&pDevEntry->ConfigList, pDevCfg, DRVHOSTAUDIOWASCACHEDEVCFG, ListEntry)
    {
        if (PDMAudioPropsAreEqual(&pDevCfg->Props, pProps))
        {
            RTListNodeRemove(&pDevCfg->ListEntry);
            return pDevCfg;
        }
    }
    return NULL;
}

/**
 * Creates a device config entry using the given parameters.
 *
 * The entry is not added to the cache but returned.
 *
 * @returns Pointer to the new device config entry. NULL on failure.
 * @param   pDevEntry       The device entry it belongs to.
 * @param   pCfgReq         The requested configuration.
 * @param   pWaveFmtEx      The actual configuration.
 * @param   pIAudioClient   The audio client, reference consumed.
 */
static PDRVHOSTAUDIOWASCACHEDEVCFG
drvHostAudioWasCacheCreateConfig(PDRVHOSTAUDIOWASCACHEDEV pDevEntry, PCPDMAUDIOSTREAMCFG pCfgReq,
                                 WAVEFORMATEX const *pWaveFmtEx, IAudioClient *pIAudioClient)
{
    PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg = (PDRVHOSTAUDIOWASCACHEDEVCFG)RTMemAllocZ(sizeof(*pDevCfg));
    if (pDevCfg)
    {
        RTListInit(&pDevCfg->ListEntry);
        pDevCfg->pDevEntry          = pDevEntry;
        pDevCfg->pIAudioClient      = pIAudioClient;
        HRESULT hrc;
        if (pCfgReq->enmDir == PDMAUDIODIR_IN)
            hrc = pIAudioClient->GetService(__uuidof(IAudioCaptureClient), (void **)&pDevCfg->pIAudioCaptureClient);
        else
            hrc = pIAudioClient->GetService(__uuidof(IAudioRenderClient), (void **)&pDevCfg->pIAudioRenderClient);
        Log8Func(("GetService -> %Rhrc + %p\n", hrc, pCfgReq->enmDir == PDMAUDIODIR_IN
                  ? (void *)pDevCfg->pIAudioCaptureClient : (void *)pDevCfg->pIAudioRenderClient));
        if (SUCCEEDED(hrc))
        {
            /*
             * Obtain the actual stream format and buffer config.
             * (A bit ugly structure here to keep it from hitting the right margin. Sorry.)
             */
            UINT32          cFramesBufferSize       = 0;
            REFERENCE_TIME  cDefaultPeriodInNtTicks = 0;
            REFERENCE_TIME  cMinimumPeriodInNtTicks = 0;
            REFERENCE_TIME  cLatencyinNtTicks       = 0;
            hrc = pIAudioClient->GetBufferSize(&cFramesBufferSize);
            if (SUCCEEDED(hrc))
                hrc = pIAudioClient->GetDevicePeriod(&cDefaultPeriodInNtTicks, &cMinimumPeriodInNtTicks);
            else
                LogRelMax(64, ("WasAPI: GetBufferSize failed: %Rhrc\n", hrc));
            if (SUCCEEDED(hrc))
                hrc = pIAudioClient->GetStreamLatency(&cLatencyinNtTicks);
            else
                LogRelMax(64, ("WasAPI: GetDevicePeriod failed: %Rhrc\n", hrc));
            if (SUCCEEDED(hrc))
            {
                LogRel2(("WasAPI: Aquired buffer parameters for %s:\n"
                         "WasAPI:   cFramesBufferSize       = %RU32\n"
                         "WasAPI:   cDefaultPeriodInNtTicks = %RI64\n"
                         "WasAPI:   cMinimumPeriodInNtTicks = %RI64\n"
                         "WasAPI:   cLatencyinNtTicks       = %RI64\n",
                         pCfgReq->szName, cFramesBufferSize, cDefaultPeriodInNtTicks, cMinimumPeriodInNtTicks, cLatencyinNtTicks));

                int rc = drvHostAudioWasCacheWaveFmtExToProps(&pDevCfg->Props, pWaveFmtEx, pCfgReq->szName, pDevEntry->wszDevId);
                if (RT_SUCCESS(rc))
                {
                    pDevCfg->cFramesBufferSize = cFramesBufferSize;
                    pDevCfg->cFramesPeriod     = PDMAudioPropsNanoToFrames(&pDevCfg->Props, cDefaultPeriodInNtTicks * 100);

                    PDMAudioPropsToString(&pDevCfg->Props, pDevCfg->szProps, sizeof(pDevCfg->szProps));
                    return pDevCfg;
                }
            }
            else
                LogRelMax(64, ("WasAPI: GetStreamLatency failed: %Rhrc\n", hrc));

            if (pDevCfg->pIAudioCaptureClient)
            {
                pDevCfg->pIAudioCaptureClient->Release();
                pDevCfg->pIAudioCaptureClient = NULL;
            }

            if (pDevCfg->pIAudioRenderClient)
            {
                pDevCfg->pIAudioRenderClient->Release();
                pDevCfg->pIAudioRenderClient = NULL;
            }
        }
        RTMemFree(pDevCfg);
    }
    pIAudioClient->Release();
    return NULL;
}


/**
 * Worker for drvHostAudioWasCacheLookupOrCreate.
 *
 * If lookup fails, a new entry will be created.
 *
 * @note    Called holding the lock, returning without holding it!
 */
static PDRVHOSTAUDIOWASCACHEDEVCFG
drvHostAudioWasCacheLookupOrCreateConfig(PDRVHOSTAUDIOWAS pThis, PDRVHOSTAUDIOWASCACHEDEV pDevEntry, PCPDMAUDIOSTREAMCFG pCfgReq)
{
    char szProps[64];
    PDMAudioPropsToString(&pCfgReq->Props, szProps, sizeof(szProps));

    /*
     * Check if we've got a matching config.
     */
    PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg = drvHostAudioWasCacheLookupLocked(pDevEntry, &pCfgReq->Props);
    if (pDevCfg)
    {
        RTCritSectLeave(&pThis->CritSectCache);
        Log8Func(("Config cache hit '%s' (for '%s') on '%ls': %p\n", pDevCfg->szProps, szProps, pDevEntry->wszDevId, pDevCfg));
        return pDevCfg;
    }

    /*
     * We now need an IAudioClient interface for calling IsFormatSupported
     * on so we can get guidance as to what to do next.
     *
     * Initially, I thought the AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM was not
     * supported all the way back to Vista and that we'd had to try different
     * things here to get the most optimal format.  However, according to
     * https://social.msdn.microsoft.com/Forums/en-US/1d974d90-6636-4121-bba3-a8861d9ab92a
     * it is supported, just maybe missing from the SDK or something...
     */
    RTCritSectLeave(&pThis->CritSectCache);

    REFERENCE_TIME const cBufferSizeInNtTicks = PDMAudioPropsFramesToNtTicks(&pCfgReq->Props, pCfgReq->Backend.cFramesBufferSize);

    Log8Func(("Activating an IAudioClient for '%ls' ...\n", pDevEntry->wszDevId));
    IAudioClient *pIAudioClient = NULL;
    HRESULT hrc = pDevEntry->pIDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL,
                                                NULL /*pActivationParams*/, (void **)&pIAudioClient);
    Log8Func(("Activate('%ls', IAudioClient) -> %Rhrc\n", pDevEntry->wszDevId, hrc));
    if (FAILED(hrc))
    {
        LogRelMax(64, ("WasAPI: Activate(%ls, IAudioClient) failed: %Rhrc\n", pDevEntry->wszDevId, hrc));
        return NULL;
    }

    WAVEFORMATEX  WaveFmtEx;
    drvHostAudioWasWaveFmtExFromCfg(pCfgReq, &WaveFmtEx);

    PWAVEFORMATEX pClosestMatch = NULL;
    hrc = pIAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &WaveFmtEx, &pClosestMatch);

    /*
     * If the format is supported, create a cache entry for it.
     */
    if (SUCCEEDED(hrc))
    {
        if (hrc == S_OK)
            Log8Func(("IsFormatSupport(,%s,) -> S_OK + %p: requested format is supported\n", szProps, pClosestMatch));
        else
            Log8Func(("IsFormatSupport(,%s,) -> %Rhrc + %p: %uch S%u %uHz\n", szProps, hrc, pClosestMatch,
                      pClosestMatch ? pClosestMatch->nChannels : 0, pClosestMatch ? pClosestMatch->wBitsPerSample : 0,
                      pClosestMatch ? pClosestMatch->nSamplesPerSec : 0));

        uint32_t fInitFlags = AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                            | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
        hrc = pIAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, fInitFlags, cBufferSizeInNtTicks,
                                        0 /*cPeriodicityInNtTicks*/, &WaveFmtEx, NULL /*pAudioSessionGuid*/);
        Log8Func(("Initialize(,%x, %RI64, %s,) -> %Rhrc\n", fInitFlags, cBufferSizeInNtTicks, szProps, hrc));
        if (SUCCEEDED(hrc))
        {
            if (pClosestMatch)
                CoTaskMemFree(pClosestMatch);
            Log8Func(("Creating new config for '%s' on '%ls': %p\n", szProps, pDevEntry->wszDevId, pDevCfg));
            return drvHostAudioWasCacheCreateConfig(pDevEntry, pCfgReq, &WaveFmtEx, pIAudioClient);
        }

        LogRelMax(64, ("WasAPI: IAudioClient::Initialize(%s: %s) failed: %Rhrc\n", pCfgReq->szName, szProps, hrc));

#if 0 /* later if needed */
        /*
         * Try lookup or instantiate the closest config.
         */
        PDMAUDIOSTREAMCFG ClosestCfg = *pCfgReq;
        int rc = drvHostAudioWasCacheWaveFmtExToProps(&ClosestCfg.Props, pClosestMatch, pDevEntry->wszDevId);
        if (RT_SUCCESS(rc))
        {
            RTCritSectEnter(&pThis->CritSectCache);
            pDevCfg = drvHostAudioWasCacheLookupLocked(pDevEntry, &pCfgReq->Props);
            if (pDevCfg)
            {
                CoTaskMemFree(pClosestMatch);
                Log8Func(("Config cache hit '%s' (for '%s') on '%ls': %p\n", pDevCfg->szProps, szProps, pDevEntry->wszDevId, pDevCfg));
                return pDevCfg;
            }
            RTCritSectLeave(&pThis->CritSectCache);
        }
#endif
    }
    else
        LogRelMax(64,("WasAPI: IAudioClient::IsFormatSupport(,%s: %s,) failed: %Rhrc\n", pCfgReq->szName, szProps, hrc));

    pIAudioClient->Release();
    if (pClosestMatch)
        CoTaskMemFree(pClosestMatch);
    Log8Func(("returns NULL\n"));
    return NULL;
}


/**
 * Looks up the given device + config combo in the cache, creating a new entry
 * if missing.
 *
 * @returns Pointer to the requested device config (or closest alternative).
 *          NULL on failure (TODO: need to return why).
 * @param   pThis       The WASAPI host audio driver instance data.
 * @param   pIDevice    The device to look up.
 * @param   pCfgReq     The configuration to look up.
 */
static PDRVHOSTAUDIOWASCACHEDEVCFG
drvHostAudioWasCacheLookupOrCreate(PDRVHOSTAUDIOWAS pThis, IMMDevice *pIDevice, PCPDMAUDIOSTREAMCFG pCfgReq)
{
    /*
     * Get the device ID so we can perform the lookup.
     */
    LPWSTR  pwszDevId = NULL;
    HRESULT hrc = pIDevice->GetId(&pwszDevId);
    if (SUCCEEDED(hrc))
    {
        size_t cwcDevId = RTUtf16Len(pwszDevId);

        /*
         * The cache has two levels, so first the device entry.
         */
        PDRVHOSTAUDIOWASCACHEDEV pDevEntry;
        RTCritSectEnter(&pThis->CritSectCache);
        RTListForEach(&pThis->CacheHead, pDevEntry, DRVHOSTAUDIOWASCACHEDEV, ListEntry)
        {
            if (   pDevEntry->cwcDevId == cwcDevId
                && pDevEntry->enmDir   == pCfgReq->enmDir
                && RTUtf16Cmp(pDevEntry->wszDevId, pwszDevId) == 0)
            {
                CoTaskMemFree(pwszDevId);
                Log8Func(("Cache hit for device '%ls': %p\n", pDevEntry->wszDevId, pDevEntry));
                return drvHostAudioWasCacheLookupOrCreateConfig(pThis, pDevEntry, pCfgReq);
            }
        }
        RTCritSectLeave(&pThis->CritSectCache);

        /*
         * Device not in the cache, add it.
         */
        pDevEntry = (PDRVHOSTAUDIOWASCACHEDEV)RTMemAllocZVar(RT_UOFFSETOF_DYN(DRVHOSTAUDIOWASCACHEDEV, wszDevId[cwcDevId + 1]));
        if (pDevEntry)
        {
            pIDevice->AddRef();
            pDevEntry->pIDevice                   = pIDevice;
            pDevEntry->enmDir                     = pCfgReq->enmDir;
            pDevEntry->cwcDevId                   = cwcDevId;
#if 0
            pDevEntry->fSupportsAutoConvertPcm    = -1;
            pDevEntry->fSupportsSrcDefaultQuality = -1;
#endif
            RTListInit(&pDevEntry->ConfigList);
            memcpy(pDevEntry->wszDevId, pwszDevId, cwcDevId * sizeof(RTUTF16));
            pDevEntry->wszDevId[cwcDevId] = '\0';

            CoTaskMemFree(pwszDevId);
            pwszDevId = NULL;

            /*
             * Before adding the device, check that someone didn't race us adding it.
             */
            RTCritSectEnter(&pThis->CritSectCache);
            PDRVHOSTAUDIOWASCACHEDEV pDevEntry2;
            RTListForEach(&pThis->CacheHead, pDevEntry2, DRVHOSTAUDIOWASCACHEDEV, ListEntry)
            {
                if (   pDevEntry2->cwcDevId == cwcDevId
                    && pDevEntry2->enmDir   == pCfgReq->enmDir
                    && RTUtf16Cmp(pDevEntry2->wszDevId, pDevEntry->wszDevId) == 0)
                {
                    pIDevice->Release();
                    RTMemFree(pDevEntry);
                    pDevEntry = NULL;

                    Log8Func(("Lost race adding device '%ls': %p\n", pDevEntry2->wszDevId, pDevEntry2));
                    return drvHostAudioWasCacheLookupOrCreateConfig(pThis, pDevEntry2, pCfgReq);
                }
            }
            RTListPrepend(&pThis->CacheHead, &pDevEntry->ListEntry);

            Log8Func(("Added device '%ls' to cache: %p\n", pDevEntry->wszDevId, pDevEntry));
            return drvHostAudioWasCacheLookupOrCreateConfig(pThis, pDevEntry, pCfgReq);
        }
        CoTaskMemFree(pwszDevId);
    }
    else
        LogRelMax(64, ("WasAPI: GetId failed (lookup): %Rhrc\n", hrc));
    return NULL;
}


/**
 * Return the given config to the cache.
 *
 * @param   pThis       The WASAPI host audio driver instance data.
 * @param   pDevCfg     The device config to put back.
 */
static void drvHostAudioWasCachePutBack(PDRVHOSTAUDIOWAS pThis, PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg)
{
    /*
     * Reset the audio client to see that it works and to make sure it's in a sensible state.
     */
    HRESULT hrc = pDevCfg->pIAudioClient->Reset();
    if (SUCCEEDED(hrc))
    {
        Log8Func(("Putting %p/'%s' back\n", pDevCfg, pDevCfg->szProps));
        RTCritSectEnter(&pThis->CritSectCache);
        RTListAppend(&pDevCfg->pDevEntry->ConfigList, &pDevCfg->ListEntry);
        RTCritSectLeave(&pThis->CritSectCache);
    }
    else
    {
        Log8Func(("IAudioClient::Reset failed (%Rhrc) on %p/'%s', destroying it.\n", hrc, pDevCfg, pDevCfg->szProps));
        drvHostAudioWasCacheDestroyDevConfig(pDevCfg);
    }
}


static void drvHostWasCacheConfigHinting(PDRVHOSTAUDIOWAS pThis, PPDMAUDIOSTREAMCFG pCfgReq)
{
    /*
     * Get the device.
     */
    pThis->pNotifyClient->lockEnter();
    IMMDevice *pIDevice = pCfgReq->enmDir == PDMAUDIODIR_IN ? pThis->pIDeviceInput : pThis->pIDeviceOutput;
    if (pIDevice)
        pIDevice->AddRef();
    pThis->pNotifyClient->lockLeave();
    if (pIDevice)
    {
        /*
         * Look up the config and put it back.
         */
        PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg = drvHostAudioWasCacheLookupOrCreate(pThis, pIDevice, pCfgReq);
        LogFlowFunc(("pDevCfg=%p\n"));
        if (pDevCfg)
            drvHostAudioWasCachePutBack(pThis, pDevCfg);
        pIDevice->Release();
    }
}


/**
 * Prefills the cache.
 *
 * @param   pThis       The WASAPI host audio driver instance data.
 */
static void drvHostAudioWasCacheFill(PDRVHOSTAUDIOWAS pThis)
{
#if 0 /* we don't have the buffer config nor do we really know which frequences to expect */
    Log8Func(("enter\n"));
    struct
    {
        PCRTUTF16       pwszDevId;
        PDMAUDIODIR     enmDir;
    } aToCache[] =
    {
        { pThis->pwszInputDevId, PDMAUDIODIR_IN },
        { pThis->pwszOutputDevId, PDMAUDIODIR_OUT }
    };
    for (unsigned i = 0; i < RT_ELEMENTS(aToCache); i++)
    {
        PCRTUTF16       pwszDevId = aToCache[i].pwszDevId;
        IMMDevice      *pIDevice  = NULL;
        HRESULT         hrc;
        if (pwszDevId)
            hrc = pThis->pIEnumerator->GetDevice(pwszDevId, &pIDevice);
        else
        {
            hrc = pThis->pIEnumerator->GetDefaultAudioEndpoint(aToCache[i].enmDir == PDMAUDIODIR_IN ? eCapture : eRender,
                                                               eMultimedia, &pIDevice);
            pwszDevId = aToCache[i].enmDir == PDMAUDIODIR_IN ? L"{Default-In}" : L"{Default-Out}";
        }
        if (SUCCEEDED(hrc))
        {
            PDMAUDIOSTREAMCFG Cfg = { aToCache[i].enmDir, { PDMAUDIOPLAYBACKDST_INVALID },
                                      PDMAUDIOPCMPROPS_INITIALIZER(2, true, 2, 44100, false) };
            Cfg.Backend.cFramesBufferSize = PDMAudioPropsMilliToFrames(&Cfg.Props, 300);
            PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg = drvHostAudioWasCacheLookupOrCreate(pThis, pIDevice, &Cfg);
            if (pDevCfg)
                drvHostAudioWasCachePutBack(pThis, pDevCfg);

            pIDevice->Release();
        }
        else
            LogRelMax(64, ("WasAPI: Failed to open audio device '%ls' (pre-caching): %Rhrc\n", pwszDevId, hrc));
    }
    Log8Func(("leave\n"));
#else
    RT_NOREF(pThis);
#endif
}


/*********************************************************************************************************************************
*   Worker thread                                                                                                                *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNRTTHREAD,
 * Asynchronous thread for setting up audio client configs.}
 */
static DECLCALLBACK(int) drvHostWasWorkerThread(RTTHREAD hThreadSelf, void *pvUser)
{
    PDRVHOSTAUDIOWAS pThis = (PDRVHOSTAUDIOWAS)pvUser;

    /*
     * We need to set the thread ID so others can post us thread messages.
     * And before we signal that we're ready, make sure we've got a message queue.
     */
    pThis->idWorkerThread = GetCurrentThreadId();
    LogFunc(("idWorkerThread=%#x (%u)\n", pThis->idWorkerThread, pThis->idWorkerThread));

    MSG Msg;
    PeekMessageW(&Msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    int rc = RTThreadUserSignal(hThreadSelf);
    AssertRC(rc);

    /*
     * Message loop.
     */
    BOOL fRet;
    while ((fRet = GetMessageW(&Msg, NULL, 0, 0)) != FALSE)
    {
        if (fRet != -1)
        {
            TranslateMessage(&Msg);
            Log9Func(("Msg: time=%u: msg=%#x l=%p w=%p for hwnd=%p\n", Msg.time, Msg.message, Msg.lParam, Msg.wParam, Msg.hwnd));
            switch (Msg.message)
            {
                case WM_DRVHOSTAUDIOWAS_HINT:
                {
                    AssertMsgBreak(Msg.wParam == pThis->uWorkerThreadFixedParam, ("%p\n", Msg.wParam));
                    AssertBreak(Msg.hwnd == NULL);
                    PPDMAUDIOSTREAMCFG pCfgReq = (PPDMAUDIOSTREAMCFG)Msg.lParam;
                    AssertPtrBreak(pCfgReq);

                    drvHostWasCacheConfigHinting(pThis, pCfgReq);
                    RTMemFree(pCfgReq);
                    break;
                }

                case WM_DRVHOSTAUDIOWAS_PURGE_CACHE:
                {
                    AssertMsgBreak(Msg.wParam == pThis->uWorkerThreadFixedParam, ("%p\n", Msg.wParam));
                    AssertBreak(Msg.hwnd == NULL);
                    AssertBreak(Msg.lParam == 0);

                    drvHostAudioWasCachePurge(pThis);
                    break;
                }

                default:
                    break;
            }
            DispatchMessageW(&Msg);
        }
        else
            AssertMsgFailed(("GetLastError()=%u\n", GetLastError()));
    }

    LogFlowFunc(("Pre-quit cache purge...\n"));
    drvHostAudioWasCachePurge(pThis);

    LogFunc(("Quits\n"));
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   PDMIHOSTAUDIO                                                                                                                *
*********************************************************************************************************************************/

/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetConfig}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_GetConfig(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDCFG pBackendCfg)
{
    RT_NOREF(pInterface);
    AssertPtrReturn(pInterface,  VERR_INVALID_POINTER);
    AssertPtrReturn(pBackendCfg, VERR_INVALID_POINTER);


    /*
     * Fill in the config structure.
     */
    RTStrCopy(pBackendCfg->szName, sizeof(pBackendCfg->szName), "WasAPI");
    pBackendCfg->cbStream       = sizeof(DRVHOSTAUDIOWASSTREAM);
    pBackendCfg->fFlags         = 0;
    pBackendCfg->cMaxStreamsIn  = UINT32_MAX;
    pBackendCfg->cMaxStreamsOut = UINT32_MAX;

    return VINF_SUCCESS;
}


/**
 * Queries information for @a pDevice and adds an entry to the enumeration.
 *
 * @returns VBox status code.
 * @param   pDevEnm     The enumeration to add the device to.
 * @param   pIDevice    The device.
 * @param   enmType     The type of device.
 * @param   fDefault    Whether it's the default device.
 */
static int drvHostWasEnumAddDev(PPDMAUDIOHOSTENUM pDevEnm, IMMDevice *pIDevice, EDataFlow enmType, bool fDefault)
{
    int rc = VINF_SUCCESS; /* ignore most errors */
    RT_NOREF(fDefault); /** @todo default device marking/skipping. */

    /*
     * Gather the necessary properties.
     */
    IPropertyStore *pProperties = NULL;
    HRESULT hrc = pIDevice->OpenPropertyStore(STGM_READ, &pProperties);
    if (SUCCEEDED(hrc))
    {
        /* Get the friendly name (string). */
        PROPVARIANT VarName;
        PropVariantInit(&VarName);
        hrc = pProperties->GetValue(PKEY_Device_FriendlyName, &VarName);
        if (SUCCEEDED(hrc))
        {
            /* Get the device ID (string). */
            LPWSTR pwszDevId = NULL;
            hrc = pIDevice->GetId(&pwszDevId);
            if (SUCCEEDED(hrc))
            {
                size_t const cwcDevId = RTUtf16Len(pwszDevId);

                /* Get the device format (blob). */
                PROPVARIANT VarFormat;
                PropVariantInit(&VarFormat);
                hrc = pProperties->GetValue(PKEY_AudioEngine_DeviceFormat, &VarFormat);
                if (SUCCEEDED(hrc))
                {
                    WAVEFORMATEX const * const pFormat = (WAVEFORMATEX const *)VarFormat.blob.pBlobData;
                    AssertPtr(pFormat);

                    /*
                     * Create a enumeration entry for it.
                     */
                    size_t const cbDev = RT_ALIGN_Z(  RT_OFFSETOF(DRVHOSTAUDIOWASDEV, wszDevId)
                                                    + (cwcDevId + 1) * sizeof(RTUTF16),
                                                    64);
                    PDRVHOSTAUDIOWASDEV pDev = (PDRVHOSTAUDIOWASDEV)PDMAudioHostDevAlloc(cbDev);
                    if (pDev)
                    {
                        pDev->Core.enmUsage = enmType == eRender ? PDMAUDIODIR_OUT : PDMAUDIODIR_IN;
                        pDev->Core.enmType  = PDMAUDIODEVICETYPE_BUILTIN;
                        if (enmType == eRender)
                            pDev->Core.cMaxOutputChannels = pFormat->nChannels;
                        else
                            pDev->Core.cMaxInputChannels  = pFormat->nChannels;

                        memcpy(pDev->wszDevId, pwszDevId, cwcDevId * sizeof(RTUTF16));
                        pDev->wszDevId[cwcDevId] = '\0';

                        char *pszName;
                        rc = RTUtf16ToUtf8(VarName.pwszVal, &pszName);
                        if (RT_SUCCESS(rc))
                        {
                            RTStrCopy(pDev->Core.szName, sizeof(pDev->Core.szName), pszName);
                            RTStrFree(pszName);

                            PDMAudioHostEnumAppend(pDevEnm, &pDev->Core);
                        }
                        else
                            PDMAudioHostDevFree(&pDev->Core);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                    PropVariantClear(&VarFormat);
                }
                else
                    LogFunc(("Failed to get PKEY_AudioEngine_DeviceFormat: %Rhrc\n", hrc));
                CoTaskMemFree(pwszDevId);
            }
            else
                LogFunc(("Failed to get the device ID: %Rhrc\n", hrc));
            PropVariantClear(&VarName);
        }
        else
            LogFunc(("Failed to get PKEY_Device_FriendlyName: %Rhrc\n", hrc));
        pProperties->Release();
    }
    else
        LogFunc(("OpenPropertyStore failed: %Rhrc\n", hrc));

    if (hrc == E_OUTOFMEMORY && RT_SUCCESS_NP(rc))
        rc = VERR_NO_MEMORY;
    return rc;
}


/**
 * Does a (Re-)enumeration of the host's playback + capturing devices.
 *
 * @return  VBox status code.
 * @param   pThis       The WASAPI host audio driver instance data.
 * @param   pDevEnm     Where to store the enumerated devices.
 */
static int drvHostWasEnumerateDevices(PDRVHOSTAUDIOWAS pThis, PPDMAUDIOHOSTENUM pDevEnm)
{
    LogRel2(("WasAPI: Enumerating devices ...\n"));

    int rc = VINF_SUCCESS;
    for (unsigned idxPass = 0; idxPass < 2 && RT_SUCCESS(rc); idxPass++)
    {
        EDataFlow const enmType = idxPass == 0 ? EDataFlow::eRender : EDataFlow::eCapture;

        /* Get the default device first. */
        IMMDevice *pIDefaultDevice = NULL;
        HRESULT hrc = pThis->pIEnumerator->GetDefaultAudioEndpoint(enmType, eMultimedia, &pIDefaultDevice);
        if (SUCCEEDED(hrc))
            rc = drvHostWasEnumAddDev(pDevEnm, pIDefaultDevice, enmType, true);
        else
            pIDefaultDevice = NULL;

        /* Enumerate the devices. */
        IMMDeviceCollection *pCollection = NULL;
        hrc = pThis->pIEnumerator->EnumAudioEndpoints(enmType, DEVICE_STATE_ACTIVE /*| DEVICE_STATE_UNPLUGGED?*/, &pCollection);
        if (SUCCEEDED(hrc) && pCollection != NULL)
        {
            UINT cDevices = 0;
            hrc = pCollection->GetCount(&cDevices);
            if (SUCCEEDED(hrc))
            {
                for (UINT idxDevice = 0; idxDevice < cDevices && RT_SUCCESS(rc); idxDevice++)
                {
                    IMMDevice *pIDevice = NULL;
                    hrc = pCollection->Item(idxDevice, &pIDevice);
                    if (SUCCEEDED(hrc) && pIDevice)
                    {
                        if (pIDevice != pIDefaultDevice)
                            rc = drvHostWasEnumAddDev(pDevEnm, pIDevice, enmType, false);
                        pIDevice->Release();
                    }
                }
            }
            pCollection->Release();
        }
        else
            LogRelMax(10, ("EnumAudioEndpoints(%s) failed: %Rhrc\n", idxPass == 0 ? "output" : "input", hrc));

        if (pIDefaultDevice)
            pIDefaultDevice->Release();
    }

    LogRel2(("WasAPI: Enumerating devices done - %u device (%Rrc)\n", pDevEnm->cDevices, rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetDevices}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_GetDevices(PPDMIHOSTAUDIO pInterface, PPDMAUDIOHOSTENUM pDeviceEnum)
{
    PDRVHOSTAUDIOWAS pThis = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    AssertPtrReturn(pDeviceEnum, VERR_INVALID_POINTER);

    PDMAudioHostEnumInit(pDeviceEnum);
    int rc = drvHostWasEnumerateDevices(pThis, pDeviceEnum);
    if (RT_FAILURE(rc))
        PDMAudioHostEnumDelete(pDeviceEnum);

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvHostAudioWasHA_GetStatus(PPDMIHOSTAUDIO pInterface, PDMAUDIODIR enmDir)
{
    RT_NOREF(pInterface, enmDir);
    return PDMAUDIOBACKENDSTS_RUNNING;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamConfigHint}
 */
static DECLCALLBACK(void) drvHostAudioWasHA_StreamConfigHint(PPDMIHOSTAUDIO pInterface, PPDMAUDIOSTREAMCFG pCfg)
{
    PDRVHOSTAUDIOWAS pThis = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    LogFlowFunc(("pCfg=%p\n", pCfg));

    if (pThis->hWorkerThread != NIL_RTTHREAD)
    {
        PPDMAUDIOSTREAMCFG pCfgCopy = PDMAudioStrmCfgDup(pCfg);
        if (pCfgCopy)
        {
            if (PostThreadMessageW(pThis->idWorkerThread, WM_DRVHOSTAUDIOWAS_HINT,
                                   pThis->uWorkerThreadFixedParam, (LPARAM)pCfgCopy))
                LogFlowFunc(("Posted %p to worker thread\n", pCfgCopy));
            else
            {
                LogRelMax(64, ("WasAPI: PostThreadMessageW failed: %u\n", GetLastError()));
                PDMAudioStrmCfgFree(pCfgCopy);
            }
        }
    }
    else
        drvHostWasCacheConfigHinting(pThis, pCfg);
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamCreate(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                        PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgAcq, VERR_INVALID_POINTER);
    AssertReturn(pCfgReq->enmDir == PDMAUDIODIR_IN || pCfgReq->enmDir == PDMAUDIODIR_OUT, VERR_INVALID_PARAMETER);
    Assert(PDMAudioStrmCfgEquals(pCfgReq, pCfgAcq));

    const char * const pszStreamType = pCfgReq->enmDir == PDMAUDIODIR_IN ? "capture" : "playback"; RT_NOREF(pszStreamType);
    LogFlowFunc(("enmSrc/Dst=%s '%s'\n",
                 pCfgReq->enmDir == PDMAUDIODIR_IN ? PDMAudioRecSrcGetName(pCfgReq->u.enmSrc)
                 : PDMAudioPlaybackDstGetName(pCfgReq->u.enmDst), pCfgReq->szName));
#if defined(RTLOG_REL_ENABLED) || defined(LOG_ENABLED)
    char szTmp[64];
#endif
    LogRel2(("WasAPI: Opening %s stream '%s' (%s)\n", pCfgReq->szName, pszStreamType,
             PDMAudioPropsToString(&pCfgReq->Props, szTmp, sizeof(szTmp))));

    RTListInit(&pStreamWas->ListEntry);

    /*
     * Do configuration conversion.
     */
    WAVEFORMATEX WaveFmtX;
    drvHostAudioWasWaveFmtExFromCfg(pCfgReq, &WaveFmtX);
    LogRel2(("WasAPI: Requested %s format for '%s':\n"
             "WasAPI:   wFormatTag      = %RU16\n"
             "WasAPI:   nChannels       = %RU16\n"
             "WasAPI:   nSamplesPerSec  = %RU32\n"
             "WasAPI:   nAvgBytesPerSec = %RU32\n"
             "WasAPI:   nBlockAlign     = %RU16\n"
             "WasAPI:   wBitsPerSample  = %RU16\n"
             "WasAPI:   cbSize          = %RU16\n"
             "WasAPI:   cBufferSizeInNtTicks = %RU64\n",
             pszStreamType, pCfgReq->szName, WaveFmtX.wFormatTag, WaveFmtX.nChannels, WaveFmtX.nSamplesPerSec,
             WaveFmtX.nAvgBytesPerSec, WaveFmtX.nBlockAlign, WaveFmtX.wBitsPerSample, WaveFmtX.cbSize,
             PDMAudioPropsFramesToNtTicks(&pCfgReq->Props, pCfgReq->Backend.cFramesBufferSize) ));

    /*
     * Get the device we're supposed to use.
     * (We cache this as it takes ~2ms to get the default device on a random W10 19042 system.)
     */
    pThis->pNotifyClient->lockEnter();
    IMMDevice *pIDevice = pCfgReq->enmDir == PDMAUDIODIR_IN ? pThis->pIDeviceInput : pThis->pIDeviceOutput;
    if (pIDevice)
        pIDevice->AddRef();
    pThis->pNotifyClient->lockLeave();

    PRTUTF16       pwszDevId     = pCfgReq->enmDir == PDMAUDIODIR_IN ? pThis->pwszInputDevId : pThis->pwszOutputDevId;
    PRTUTF16 const pwszDevIdDesc = pwszDevId ? pwszDevId : pCfgReq->enmDir == PDMAUDIODIR_IN ? L"{Default-In}" : L"{Default-Out}";
    if (!pIDevice)
    {
        /** @todo we can eliminate this too...   */
        HRESULT hrc;
        if (pwszDevId)
            hrc = pThis->pIEnumerator->GetDevice(pwszDevId, &pIDevice);
        else
            hrc = pThis->pIEnumerator->GetDefaultAudioEndpoint(pCfgReq->enmDir == PDMAUDIODIR_IN ? eCapture : eRender,
                                                               eMultimedia, &pIDevice);
        LogFlowFunc(("Got device %p (%Rhrc)\n", pIDevice, hrc));
        if (FAILED(hrc))
        {
            LogRelMax(64, ("WasAPI: Failed to open audio %s device '%ls': %Rhrc\n", pszStreamType, pwszDevIdDesc, hrc));
            return VERR_AUDIO_STREAM_COULD_NOT_CREATE;
        }
    }

    /*
     * Ask the cache to retrieve or instantiate the requested configuration.
     */
    /** @todo make it return a status code too and retry if the default device
     *        was invalidated/changed while we where working on it here. */
    int rc = VERR_AUDIO_STREAM_COULD_NOT_CREATE;
    PDRVHOSTAUDIOWASCACHEDEVCFG pDevCfg = drvHostAudioWasCacheLookupOrCreate(pThis, pIDevice, pCfgReq);

    pIDevice->Release();
    pIDevice = NULL;

    if (pDevCfg)
    {
        pStreamWas->pDevCfg = pDevCfg;

        pCfgAcq->Props = pDevCfg->Props;
        pCfgAcq->Backend.cFramesBufferSize   = pDevCfg->cFramesBufferSize;
        pCfgAcq->Backend.cFramesPeriod       = pDevCfg->cFramesPeriod;
        pCfgAcq->Backend.cFramesPreBuffering = pCfgReq->Backend.cFramesPreBuffering * pDevCfg->cFramesBufferSize
                                             / RT_MAX(pCfgReq->Backend.cFramesBufferSize, 1);

        PDMAudioStrmCfgCopy(&pStreamWas->Cfg, pCfgAcq);

        /* Finally, the critical section. */
        int rc2 = RTCritSectInit(&pStreamWas->CritSect);
        if (RT_SUCCESS(rc2))
        {
            RTCritSectRwEnterExcl(&pThis->CritSectStreamList);
            RTListAppend(&pThis->StreamHead, &pStreamWas->ListEntry);
            RTCritSectRwLeaveExcl(&pThis->CritSectStreamList);

            LogFlowFunc(("returns VINF_SUCCESS\n", rc));
            return VINF_SUCCESS;
        }

        LogRelMax(64, ("WasAPI: Failed to create critical section for stream.\n"));
        drvHostAudioWasCachePutBack(pThis, pDevCfg);
        pStreamWas->pDevCfg = NULL;
    }
    else
        LogRelMax(64, ("WasAPI: Failed to setup %s on audio device '%ls'.\n", pszStreamType, pwszDevIdDesc));

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamDestroy(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, VERR_INVALID_POINTER);
    LogFlowFunc(("Stream '%s'\n", pStreamWas->Cfg.szName));
    HRESULT hrc;

    if (RTCritSectIsInitialized(&pStreamWas->CritSect))
    {
        RTCritSectRwEnterExcl(&pThis->CritSectStreamList);
        RTListNodeRemove(&pStreamWas->ListEntry);
        RTCritSectRwLeaveExcl(&pThis->CritSectStreamList);

        RTCritSectDelete(&pStreamWas->CritSect);
    }

    if (pStreamWas->fStarted && pStreamWas->pDevCfg && pStreamWas->pDevCfg->pIAudioClient)
    {
        hrc = pStreamWas->pDevCfg->pIAudioClient->Stop();
        LogFunc(("Stop('%s') -> %Rhrc\n", pStreamWas->Cfg.szName, hrc));
        pStreamWas->fStarted = false;
    }

    if (pStreamWas->cFramesCaptureToRelease)
    {
        hrc = pStreamWas->pDevCfg->pIAudioCaptureClient->ReleaseBuffer(0);
        Log4Func(("Releasing capture buffer (%#x frames): %Rhrc\n", pStreamWas->cFramesCaptureToRelease, hrc));
        pStreamWas->cFramesCaptureToRelease = 0;
        pStreamWas->pbCapture               = NULL;
        pStreamWas->cbCapture               = 0;
    }

    if (pStreamWas->pDevCfg)
    {
        drvHostAudioWasCachePutBack(pThis, pStreamWas->pDevCfg);
        pStreamWas->pDevCfg = NULL;
    }

    LogFlowFunc(("returns\n"));
    return VINF_SUCCESS;
}


/**
 * Wrapper for starting a stream.
 *
 * @returns VBox status code.
 * @param   pThis           The WASAPI host audio driver instance data.
 * @param   pStreamWas      The stream.
 * @param   pszOperation    The operation we're doing.
 */
static int drvHostAudioWasStreamStartWorker(PDRVHOSTAUDIOWAS pThis, PDRVHOSTAUDIOWASSTREAM pStreamWas, const char *pszOperation)
{
    HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->Start();
    LogFlow(("%s: Start(%s) returns %Rhrc\n", pszOperation, pStreamWas->Cfg.szName, hrc));
    AssertStmt(hrc != AUDCLNT_E_NOT_STOPPED, hrc = S_OK);
    if (SUCCEEDED(hrc))
    {
        pStreamWas->fStarted = true;
        return VINF_SUCCESS;
    }

    /** @todo try re-setup the stuff on AUDCLNT_E_DEVICEINVALIDATED.
     * Need some way of telling the caller (e.g. playback, capture) so they can
     * retry what they're doing */
    RT_NOREF(pThis);

    pStreamWas->fStarted = false;
    LogRelMax(64, ("WasAPI: Starting '%s' failed (%s): %Rhrc\n", pStreamWas->Cfg.szName, pszOperation, hrc));
    return VERR_AUDIO_STREAM_NOT_READY;
}


/**
 * @ interface_method_impl{PDMIHOSTAUDIO,pfnStreamEnable}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamEnable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    LogFlowFunc(("Stream '%s' {%s}\n", pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas)));
    HRESULT hrc;
    RTCritSectEnter(&pStreamWas->CritSect);

    Assert(!pStreamWas->fEnabled);
    Assert(!pStreamWas->fStarted);

    /*
     * We always reset the buffer before enabling the stream (normally never necessary).
     */
    if (pStreamWas->cFramesCaptureToRelease)
    {
        hrc = pStreamWas->pDevCfg->pIAudioCaptureClient->ReleaseBuffer(pStreamWas->cFramesCaptureToRelease);
        Log4Func(("Releasing capture buffer (%#x frames): %Rhrc\n", pStreamWas->cFramesCaptureToRelease, hrc));
        pStreamWas->cFramesCaptureToRelease = 0;
        pStreamWas->pbCapture               = NULL;
        pStreamWas->cbCapture               = 0;
    }

    hrc = pStreamWas->pDevCfg->pIAudioClient->Reset();
    if (FAILED(hrc))
        LogRelMax(64, ("WasAPI: Stream reset failed when enabling '%s': %Rhrc\n", pStreamWas->Cfg.szName, hrc));
    pStreamWas->offInternal      = 0;
    pStreamWas->fDraining        = false;
    pStreamWas->fEnabled         = true;
    pStreamWas->fRestartOnResume = false;

    /*
     * Input streams will start capturing, while output streams will only start
     * playing once we get some audio data to play.
     */
    int rc = VINF_SUCCESS;
    if (pStreamWas->Cfg.enmDir == PDMAUDIODIR_IN)
        rc = drvHostAudioWasStreamStartWorker(pThis, pStreamWas, "enable");
    else
        Assert(pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT);

    RTCritSectLeave(&pStreamWas->CritSect);
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}


/**
 * @ interface_method_impl{PDMIHOSTAUDIO,pfnStreamDisable}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamDisable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    PDRVHOSTAUDIOWASSTREAM pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    LogFlowFunc(("cMsLastTransfer=%RI64 ms, stream '%s' {%s} \n",
                 pStreamWas->msLastTransfer ? RTTimeMilliTS() - pStreamWas->msLastTransfer : -1,
                 pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas) ));
    RTCritSectEnter(&pStreamWas->CritSect);

    /*
     * We will not stop a draining output stream, otherwise the actions are the same here.
     */
    pStreamWas->fEnabled         = false;
    pStreamWas->fRestartOnResume = false;
    Assert(!pStreamWas->fDraining || pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT);

    int rc = VINF_SUCCESS;
    if (!pStreamWas->fDraining)
    {
        if (pStreamWas->fStarted)
        {
            HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->Stop();
            LogFlowFunc(("Stop(%s) returns %Rhrc\n", pStreamWas->Cfg.szName, hrc));
            if (FAILED(hrc))
            {
                LogRelMax(64, ("WasAPI: Stopping '%s' failed (disable): %Rhrc\n", pStreamWas->Cfg.szName, hrc));
                rc = VERR_GENERAL_FAILURE;
            }
            pStreamWas->fStarted = false;
        }
    }
    else
    {
        LogFunc(("Stream '%s' is still draining...\n", pStreamWas->Cfg.szName));
        Assert(pStreamWas->fStarted);
    }

    RTCritSectLeave(&pStreamWas->CritSect);
    LogFlowFunc(("returns %Rrc {%s}\n", rc, drvHostWasStreamStatusString(pStreamWas)));
    return rc;
}


/**
 * @ interface_method_impl{PDMIHOSTAUDIO,pfnStreamPause}
 *
 * @note    Basically the same as drvHostAudioWasHA_StreamDisable, just w/o the
 *          buffer resetting and fEnabled change.
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamPause(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    PDRVHOSTAUDIOWASSTREAM pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    LogFlowFunc(("cMsLastTransfer=%RI64 ms, stream '%s' {%s} \n",
                 pStreamWas->msLastTransfer ? RTTimeMilliTS() - pStreamWas->msLastTransfer : -1,
                 pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas) ));
    RTCritSectEnter(&pStreamWas->CritSect);

    /*
     * Unless we're draining the stream, stop it if it's started.
     */
    int rc = VINF_SUCCESS;
    if (pStreamWas->fStarted && !pStreamWas->fDraining)
    {
        pStreamWas->fRestartOnResume = true;

        HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->Stop();
        LogFlowFunc(("Stop(%s) returns %Rhrc\n", pStreamWas->Cfg.szName, hrc));
        if (FAILED(hrc))
        {
            LogRelMax(64, ("WasAPI: Stopping '%s' failed (pause): %Rhrc\n", pStreamWas->Cfg.szName, hrc));
            rc = VERR_GENERAL_FAILURE;
        }
        pStreamWas->fStarted = false;
    }
    else
    {
        pStreamWas->fRestartOnResume = false;
        if (pStreamWas->fDraining)
        {
            LogFunc(("Stream '%s' is draining\n", pStreamWas->Cfg.szName));
            Assert(pStreamWas->fStarted);
        }
    }

    RTCritSectLeave(&pStreamWas->CritSect);
    LogFlowFunc(("returns %Rrc {%s}\n", rc, drvHostWasStreamStatusString(pStreamWas)));
    return rc;
}


/**
 * @ interface_method_impl{PDMIHOSTAUDIO,pfnStreamResume}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamResume(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    LogFlowFunc(("Stream '%s' {%s}\n", pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas)));
    RTCritSectEnter(&pStreamWas->CritSect);

    /*
     * Resume according to state saved by drvHostAudioWasHA_StreamPause.
     */
    int rc;
    if (pStreamWas->fRestartOnResume)
        rc = drvHostAudioWasStreamStartWorker(pThis, pStreamWas, "resume");
    else
        rc = VINF_SUCCESS;
    pStreamWas->fRestartOnResume = false;

    RTCritSectLeave(&pStreamWas->CritSect);
    LogFlowFunc(("returns %Rrc {%s}\n", rc, drvHostWasStreamStatusString(pStreamWas)));
    return rc;
}


/**
 * This is used by the timer function as well as when arming the timer.
 *
 * @param   pThis       The DSound host audio driver instance data.
 * @param   msNow       A current RTTimeMilliTS() value.
 */
static void drvHostWasDrainTimerWorker(PDRVHOSTAUDIOWAS pThis, uint64_t msNow)
{
    /*
     * Go thru the stream list and look at draining streams.
     */
    uint64_t        msNext = UINT64_MAX;
    RTCritSectRwEnterShared(&pThis->CritSectStreamList);
    PDRVHOSTAUDIOWASSTREAM   pCur;
    RTListForEach(&pThis->StreamHead, pCur, DRVHOSTAUDIOWASSTREAM, ListEntry)
    {
        if (   pCur->fDraining
            && pCur->Cfg.enmDir == PDMAUDIODIR_OUT)
        {
            Assert(pCur->fStarted);
            uint64_t msCurDeadline = pCur->msDrainDeadline;
            if (msCurDeadline > 0 && msCurDeadline < msNext)
            {
                /* Take the lock and recheck: */
                RTCritSectEnter(&pCur->CritSect);
                msCurDeadline = pCur->msDrainDeadline;
                if (   pCur->fDraining
                    && msCurDeadline > 0
                    && msCurDeadline < msNext)
                {
                    if (msCurDeadline > msNow)
                        msNext = pCur->msDrainDeadline;
                    else
                    {
                        LogRel2(("WasAPI: Stopping draining of '%s' {%s} ...\n",
                                 pCur->Cfg.szName, drvHostWasStreamStatusString(pCur)));
                        HRESULT hrc = pCur->pDevCfg->pIAudioClient->Stop();
                        if (FAILED(hrc))
                            LogRelMax(64, ("WasAPI: Failed to stop draining stream '%s': %Rhrc\n", pCur->Cfg.szName, hrc));
                        pCur->fDraining = false;
                        pCur->fStarted  = false;
                    }
                }
                RTCritSectLeave(&pCur->CritSect);
            }
        }
    }

    /*
     * Re-arm the timer if necessary.
     */
    if (msNext != UINT64_MAX)
        PDMDrvHlpTimerSetMillies(pThis->pDrvIns, pThis->hDrainTimer, msNext - msNow);
    RTCritSectRwLeaveShared(&pThis->CritSectStreamList);
}


/**
 * @callback_method_impl{FNTMTIMERDRV,
 * This is to ensure that draining streams stop properly.}
 */
static DECLCALLBACK(void) drvHostWasDrainStopTimer(PPDMDRVINS pDrvIns, TMTIMERHANDLE hTimer, void *pvUser)
{
    PDRVHOSTAUDIOWAS pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTAUDIOWAS);
    RT_NOREF(hTimer, pvUser);
    drvHostWasDrainTimerWorker(pThis, RTTimeMilliTS());
}


/**
 * @ interface_method_impl{PDMIHOSTAUDIO,pfnStreamDrain}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamDrain(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertReturn(pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT, VERR_INVALID_PARAMETER);
    LogFlowFunc(("cMsLastTransfer=%RI64 ms, stream '%s' {%s} \n",
                 pStreamWas->msLastTransfer ? RTTimeMilliTS() - pStreamWas->msLastTransfer : -1,
                 pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas) ));

    /*
     * If the stram was started, calculate when the buffered data has finished
     * playing and switch to drain mode.  Use the drain timer callback worker
     * to re-arm the timer or to stop the playback.
     */
    RTCritSectEnter(&pStreamWas->CritSect);
    int rc = VINF_SUCCESS;
    if (pStreamWas->fStarted)
    {
        if (!pStreamWas->fDraining)
        {
            if (pStreamWas->fStarted)
            {
                uint64_t const msNow           = RTTimeMilliTS();
                uint64_t       msDrainDeadline = 0;
                UINT32         cFramesPending  = 0;
                HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->GetCurrentPadding(&cFramesPending);
                if (SUCCEEDED(hrc))
                    msDrainDeadline = msNow
                                    + PDMAudioPropsFramesToMilli(&pStreamWas->Cfg.Props,
                                                                 RT_MIN(cFramesPending,
                                                                        pStreamWas->Cfg.Backend.cFramesBufferSize * 2))
                                    + 1 /*fudge*/;
                else
                {
                    msDrainDeadline = msNow;
                    LogRelMax(64, ("WasAPI: GetCurrentPadding fail on '%s' when starting draining: %Rhrc\n",
                                   pStreamWas->Cfg.szName, hrc));
                }
                pStreamWas->msDrainDeadline = msDrainDeadline;
                pStreamWas->fDraining       = true;
            }
            else
                LogFlowFunc(("Drain requested for '%s', but not started playback...\n", pStreamWas->Cfg.szName));
        }
        else
            LogFlowFunc(("Already draining '%s' ...\n", pStreamWas->Cfg.szName));
    }
    else
        AssertStmt(!pStreamWas->fDraining, pStreamWas->fDraining = false);
    RTCritSectLeave(&pStreamWas->CritSect);

    /*
     * Always do drain timer processing to re-arm the timer or actually stop
     * this stream (and others).  (Must be done _after_ unlocking the stream.)
     */
    drvHostWasDrainTimerWorker(pThis, RTTimeMilliTS());

    LogFlowFunc(("returns %Rrc {%s}\n", rc, drvHostWasStreamStatusString(pStreamWas)));
    return rc;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamControl}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamControl(PPDMIHOSTAUDIO pInterface,
                                                       PPDMAUDIOBACKENDSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    /** @todo r=bird: I'd like to get rid of this pfnStreamControl method,
     *        replacing it with individual StreamXxxx methods.  That would save us
     *        potentally huge switches and more easily see which drivers implement
     *        which operations (grep for pfnStreamXxxx). */
    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
            return drvHostAudioWasHA_StreamEnable(pInterface, pStream);
        case PDMAUDIOSTREAMCMD_DISABLE:
            return drvHostAudioWasHA_StreamDisable(pInterface, pStream);
        case PDMAUDIOSTREAMCMD_PAUSE:
            return drvHostAudioWasHA_StreamPause(pInterface, pStream);
        case PDMAUDIOSTREAMCMD_RESUME:
            return drvHostAudioWasHA_StreamResume(pInterface, pStream);
        case PDMAUDIOSTREAMCMD_DRAIN:
            return drvHostAudioWasHA_StreamDrain(pInterface, pStream);

        case PDMAUDIOSTREAMCMD_END:
        case PDMAUDIOSTREAMCMD_32BIT_HACK:
        case PDMAUDIOSTREAMCMD_INVALID:
            /* no default*/
            break;
    }
    return VERR_NOT_SUPPORTED;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvHostAudioWasHA_StreamGetReadable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    PDRVHOSTAUDIOWASSTREAM pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, 0);
    Assert(pStreamWas->Cfg.enmDir == PDMAUDIODIR_IN);

    uint32_t cbReadable = 0;
    RTCritSectEnter(&pStreamWas->CritSect);

    if (pStreamWas->pDevCfg->pIAudioCaptureClient /* paranoia */)
    {
        UINT32  cFramesInNextPacket = 0;
        HRESULT hrc = pStreamWas->pDevCfg->pIAudioCaptureClient->GetNextPacketSize(&cFramesInNextPacket);
        if (SUCCEEDED(hrc))
            cbReadable = PDMAudioPropsFramesToBytes(&pStreamWas->Cfg.Props,
                                                    RT_MIN(cFramesInNextPacket,
                                                           pStreamWas->Cfg.Backend.cFramesBufferSize * 16 /* paranoia */));
        else
            LogRelMax(64, ("WasAPI: GetNextPacketSize failed on '%s': %Rhrc\n", pStreamWas->Cfg.szName, hrc));
    }

    RTCritSectLeave(&pStreamWas->CritSect);

    LogFlowFunc(("returns %#x (%u) {%s}\n", cbReadable, cbReadable, drvHostWasStreamStatusString(pStreamWas)));
    return cbReadable;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvHostAudioWasHA_StreamGetWritable(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    PDRVHOSTAUDIOWASSTREAM pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, 0);
    LogFlowFunc(("Stream '%s' {%s}\n", pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas)));
    Assert(pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT);

    uint32_t cbWritable = 0;
    RTCritSectEnter(&pStreamWas->CritSect);

    if (   pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT
        && pStreamWas->pDevCfg->pIAudioClient /* paranoia */)
    {
        UINT32  cFramesPending = 0;
        HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->GetCurrentPadding(&cFramesPending);
        if (SUCCEEDED(hrc))
        {
            if (cFramesPending < pStreamWas->Cfg.Backend.cFramesBufferSize)
                cbWritable = PDMAudioPropsFramesToBytes(&pStreamWas->Cfg.Props,
                                                        pStreamWas->Cfg.Backend.cFramesBufferSize - cFramesPending);
            else if (cFramesPending > pStreamWas->Cfg.Backend.cFramesBufferSize)
            {
                LogRelMax(64, ("WasAPI: Warning! GetCurrentPadding('%s') return too high: cFramesPending=%#x > cFramesBufferSize=%#x\n",
                               pStreamWas->Cfg.szName, cFramesPending, pStreamWas->Cfg.Backend.cFramesBufferSize));
                AssertMsgFailed(("cFramesPending=%#x > cFramesBufferSize=%#x\n",
                                 cFramesPending, pStreamWas->Cfg.Backend.cFramesBufferSize));
            }
        }
        else
            LogRelMax(64, ("WasAPI: GetCurrentPadding failed on '%s': %Rhrc\n", pStreamWas->Cfg.szName, hrc));
    }

    RTCritSectLeave(&pStreamWas->CritSect);

    LogFlowFunc(("returns %#x (%u) {%s}\n", cbWritable, cbWritable, drvHostWasStreamStatusString(pStreamWas)));
    return cbWritable;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetPending}
 */
static DECLCALLBACK(uint32_t) drvHostAudioWasHA_StreamGetPending(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    PDRVHOSTAUDIOWASSTREAM pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, 0);
    LogFlowFunc(("Stream '%s' {%s}\n", pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas)));
    AssertReturn(pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT, 0);

    uint32_t cbPending = 0;
    RTCritSectEnter(&pStreamWas->CritSect);

    if (   pStreamWas->Cfg.enmDir == PDMAUDIODIR_OUT
        && pStreamWas->pDevCfg->pIAudioClient /* paranoia */)
    {
        if (pStreamWas->fStarted)
        {
            UINT32  cFramesPending = 0;
            HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->GetCurrentPadding(&cFramesPending);
            if (SUCCEEDED(hrc))
            {
                AssertMsg(cFramesPending <= pStreamWas->Cfg.Backend.cFramesBufferSize,
                          ("cFramesPending=%#x cFramesBufferSize=%#x\n",
                           cFramesPending, pStreamWas->Cfg.Backend.cFramesBufferSize));
                cbPending = PDMAudioPropsFramesToBytes(&pStreamWas->Cfg.Props, RT_MIN(cFramesPending, VBOX_WASAPI_MAX_PADDING));
            }
            else
                LogRelMax(64, ("WasAPI: GetCurrentPadding failed on '%s': %Rhrc\n", pStreamWas->Cfg.szName, hrc));
        }
    }

    RTCritSectLeave(&pStreamWas->CritSect);

    LogFlowFunc(("returns %#x (%u) {%s}\n", cbPending, cbPending, drvHostWasStreamStatusString(pStreamWas)));
    return cbPending;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamGetStatus}
 */
static DECLCALLBACK(uint32_t) drvHostAudioWasHA_StreamGetStatus(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream)
{
    RT_NOREF(pInterface);
    PDRVHOSTAUDIOWASSTREAM pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, PDMAUDIOSTREAM_STS_NONE);

    uint32_t fStrmStatus = PDMAUDIOSTREAM_STS_INITIALIZED;
    if (pStreamWas->fEnabled)
        fStrmStatus |= PDMAUDIOSTREAM_STS_ENABLED;
    if (pStreamWas->fDraining)
        fStrmStatus |= PDMAUDIOSTREAM_STS_PENDING_DISABLE;
    if (pStreamWas->fRestartOnResume)
        fStrmStatus |= PDMAUDIOSTREAM_STS_PAUSED;

    LogFlowFunc(("returns %#x for '%s' {%s}\n", fStrmStatus, pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas)));
    return fStrmStatus;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamPlay(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                      const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten)
{
    PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbWritten, VERR_INVALID_POINTER);
    Assert(PDMAudioPropsIsSizeAligned(&pStreamWas->Cfg.Props, cbBuf));

    RTCritSectEnter(&pStreamWas->CritSect);
    if (pStreamWas->fEnabled)
    { /* likely */ }
    else
    {
        RTCritSectLeave(&pStreamWas->CritSect);
        *pcbWritten = 0;
        LogFunc(("Skipping %#x byte write to disabled stream {%s}\n", cbBuf, drvHostWasStreamStatusString(pStreamWas)));
        return VINF_SUCCESS;
    }
    Log4Func(("cbBuf=%#x stream '%s' {%s}\n", cbBuf, pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas) ));

    /*
     * Transfer loop.
     */
    int      rc        = VINF_SUCCESS;
    uint32_t cReInits  = 0;
    uint32_t cbWritten = 0;
    while (cbBuf > 0)
    {
        AssertBreakStmt(pStreamWas->pDevCfg && pStreamWas->pDevCfg->pIAudioRenderClient && pStreamWas->pDevCfg->pIAudioClient,
                        rc = VERR_AUDIO_STREAM_NOT_READY);

        /*
         * Figure out how much we can possibly write.
         */
        UINT32   cFramesPending = 0;
        uint32_t cbWritable = 0;
        HRESULT hrc = pStreamWas->pDevCfg->pIAudioClient->GetCurrentPadding(&cFramesPending);
        if (SUCCEEDED(hrc))
            cbWritable = PDMAudioPropsFramesToBytes(&pStreamWas->Cfg.Props,
                                                      pStreamWas->Cfg.Backend.cFramesBufferSize
                                                    - RT_MIN(cFramesPending, pStreamWas->Cfg.Backend.cFramesBufferSize));
        else
        {
            LogRelMax(64, ("WasAPI: GetCurrentPadding(%s) failed during playback: %Rhrc (@%#RX64)\n",
                           pStreamWas->Cfg.szName, hrc, pStreamWas->offInternal));
            /** @todo reinit on AUDCLNT_E_DEVICEINVALIDATED? */
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }
        if (cbWritable <= PDMAudioPropsFrameSize(&pStreamWas->Cfg.Props))
            break;

        uint32_t const cbToWrite      = PDMAudioPropsFloorBytesToFrame(&pStreamWas->Cfg.Props, RT_MIN(cbWritable, cbBuf));
        uint32_t const cFramesToWrite = PDMAudioPropsBytesToFrames(&pStreamWas->Cfg.Props, cbToWrite);
        Assert(PDMAudioPropsFramesToBytes(&pStreamWas->Cfg.Props, cFramesToWrite) == cbToWrite);
        Log3Func(("@%RX64: cFramesPending=%#x -> cbWritable=%#x cbToWrite=%#x cFramesToWrite=%#x {%s}\n",
                  pStreamWas->offInternal, cFramesPending, cbWritable, cbToWrite, cFramesToWrite,
                  drvHostWasStreamStatusString(pStreamWas) ));

        /*
         * Get the buffer, copy the data into it, and relase it back to the WAS machinery.
         */
        BYTE *pbData = NULL;
        hrc = pStreamWas->pDevCfg->pIAudioRenderClient->GetBuffer(cFramesToWrite, &pbData);
        if (SUCCEEDED(hrc))
        {
            memcpy(pbData, pvBuf, cbToWrite);
            hrc = pStreamWas->pDevCfg->pIAudioRenderClient->ReleaseBuffer(cFramesToWrite, 0 /*fFlags*/);
            if (SUCCEEDED(hrc))
            {
                /*
                 * Before we advance the buffer position (so we can resubmit it
                 * after re-init), make sure we've successfully started stream.
                 */
                if (pStreamWas->fStarted)
                { }
                else
                {
                    rc = drvHostAudioWasStreamStartWorker(pThis, pStreamWas, "play");
                    if (rc == VINF_SUCCESS)
                    { /* likely */ }
                    else if (RT_SUCCESS(rc) && ++cReInits < 5)
                        continue; /* re-submit buffer after re-init */
                    else
                        break;
                }

                /* advance. */
                pvBuf                    = (uint8_t *)pvBuf + cbToWrite;
                cbBuf                   -= cbToWrite;
                cbWritten               += cbToWrite;
                pStreamWas->offInternal += cbToWrite;
            }
            else
            {
                LogRelMax(64, ("WasAPI: ReleaseBuffer(%#x) failed on '%s' during playback: %Rhrc (@%#RX64)\n",
                               cFramesToWrite, pStreamWas->Cfg.szName, hrc, pStreamWas->offInternal));
                /** @todo reinit on AUDCLNT_E_DEVICEINVALIDATED? */
                rc = VERR_AUDIO_STREAM_NOT_READY;
                break;
            }
        }
        else
        {
            LogRelMax(64, ("WasAPI: GetBuffer(%#x) failed on '%s' during playback: %Rhrc (@%#RX64)\n",
                           cFramesToWrite, pStreamWas->Cfg.szName, hrc, pStreamWas->offInternal));
            /** @todo reinit on AUDCLNT_E_DEVICEINVALIDATED? */
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }
    }

    /*
     * Done.
     */
    uint64_t const msPrev = pStreamWas->msLastTransfer;
    uint64_t const msNow  = RTTimeMilliTS();
    if (cbWritten)
        pStreamWas->msLastTransfer = msNow;

    RTCritSectLeave(&pStreamWas->CritSect);

    *pcbWritten = cbWritten;
    if (RT_SUCCESS(rc) || !cbWritten)
    { }
    else
    {
        LogFlowFunc(("Suppressing %Rrc to report %#x bytes written\n", rc, cbWritten));
        rc = VINF_SUCCESS;
    }
    LogFlowFunc(("@%#RX64: cbWritten=%RU32 cMsDelta=%RU64 (%RU64 -> %RU64) {%s}\n", pStreamWas->offInternal, cbWritten,
                 msPrev ? msNow - msPrev : 0, msPrev, pStreamWas->msLastTransfer, drvHostWasStreamStatusString(pStreamWas) ));
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMIHOSTAUDIO,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvHostAudioWasHA_StreamCapture(PPDMIHOSTAUDIO pInterface, PPDMAUDIOBACKENDSTREAM pStream,
                                                         void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead)
{
    RT_NOREF(pInterface); //PDRVHOSTAUDIOWAS        pThis      = RT_FROM_MEMBER(pInterface, DRVHOSTAUDIOWAS, IHostAudio);
    PDRVHOSTAUDIOWASSTREAM  pStreamWas = (PDRVHOSTAUDIOWASSTREAM)pStream;
    AssertPtrReturn(pStreamWas, 0);
    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pcbRead, VERR_INVALID_POINTER);
    Assert(PDMAudioPropsIsSizeAligned(&pStreamWas->Cfg.Props, cbBuf));

    RTCritSectEnter(&pStreamWas->CritSect);
    if (pStreamWas->fEnabled)
    { /* likely */ }
    else
    {
        RTCritSectLeave(&pStreamWas->CritSect);
        *pcbRead = 0;
        LogFunc(("Skipping %#x byte read from disabled stream {%s}\n", cbBuf, drvHostWasStreamStatusString(pStreamWas)));
        return VINF_SUCCESS;
    }
    Log4Func(("cbBuf=%#x stream '%s' {%s}\n", cbBuf, pStreamWas->Cfg.szName, drvHostWasStreamStatusString(pStreamWas) ));


    /*
     * Transfer loop.
     */
    int            rc          = VINF_SUCCESS;
    //uint32_t       cReInits    = 0;
    uint32_t       cbRead      = 0;
    uint32_t const cbFrame     = PDMAudioPropsFrameSize(&pStreamWas->Cfg.Props);
    while (cbBuf > cbFrame)
    {
        AssertBreakStmt(pStreamWas->pDevCfg->pIAudioCaptureClient && pStreamWas->pDevCfg->pIAudioClient, rc = VERR_AUDIO_STREAM_NOT_READY);

        /*
         * Anything pending from last call?
         * (This is rather similar to the Pulse interface.)
         */
        if (pStreamWas->cFramesCaptureToRelease)
        {
            uint32_t const cbToCopy = RT_MIN(pStreamWas->cbCapture, cbBuf);
            memcpy(pvBuf, pStreamWas->pbCapture, cbToCopy);
            pvBuf                    = (uint8_t *)pvBuf + cbToCopy;
            cbBuf                   -= cbToCopy;
            cbRead                  += cbToCopy;
            pStreamWas->offInternal += cbToCopy;
            pStreamWas->pbCapture   += cbToCopy;
            pStreamWas->cbCapture   -= cbToCopy;
            if (!pStreamWas->cbCapture)
            {
                HRESULT hrc = pStreamWas->pDevCfg->pIAudioCaptureClient->ReleaseBuffer(pStreamWas->cFramesCaptureToRelease);
                Log4Func(("@%#RX64: Releasing capture buffer (%#x frames): %Rhrc\n",
                          pStreamWas->offInternal, pStreamWas->cFramesCaptureToRelease, hrc));
                if (SUCCEEDED(hrc))
                {
                    pStreamWas->cFramesCaptureToRelease = 0;
                    pStreamWas->pbCapture               = NULL;
                }
                else
                {
                    LogRelMax(64, ("WasAPI: ReleaseBuffer(%s) failed during capture: %Rhrc (@%#RX64)\n",
                                   pStreamWas->Cfg.szName, hrc, pStreamWas->offInternal));
                    /** @todo reinit on AUDCLNT_E_DEVICEINVALIDATED? */
                    rc = VERR_AUDIO_STREAM_NOT_READY;
                    break;
                }
            }
            if (cbBuf < cbFrame)
                break;
        }

        /*
         * Figure out if there is any data available to be read now. (Docs hint that we can not
         * skip this and go straight for GetBuffer or we risk getting unwritten buffer space back).
         */
        UINT32 cFramesCaptured = 0;
        HRESULT hrc = pStreamWas->pDevCfg->pIAudioCaptureClient->GetNextPacketSize(&cFramesCaptured);
        if (SUCCEEDED(hrc))
        {
            if (!cFramesCaptured)
                break;
        }
        else
        {
            LogRelMax(64, ("WasAPI: GetNextPacketSize(%s) failed during capture: %Rhrc (@%#RX64)\n",
                           pStreamWas->Cfg.szName, hrc, pStreamWas->offInternal));
            /** @todo reinit on AUDCLNT_E_DEVICEINVALIDATED? */
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }

        /*
         * Get the buffer.
         */
        cFramesCaptured     = 0;
        UINT64  uQpsNtTicks = 0;
        UINT64  offDevice   = 0;
        DWORD   fBufFlags   = 0;
        BYTE   *pbData      = NULL;
        hrc = pStreamWas->pDevCfg->pIAudioCaptureClient->GetBuffer(&pbData, &cFramesCaptured, &fBufFlags, &offDevice, &uQpsNtTicks);
        Log4Func(("@%#RX64: GetBuffer -> %Rhrc pbData=%p cFramesCaptured=%#x fBufFlags=%#x offDevice=%#RX64 uQpcNtTicks=%#RX64\n",
                  pStreamWas->offInternal, hrc, pbData, cFramesCaptured, fBufFlags, offDevice, uQpsNtTicks));
        if (SUCCEEDED(hrc))
        {
            Assert(cFramesCaptured < VBOX_WASAPI_MAX_PADDING);
            pStreamWas->pbCapture               = pbData;
            pStreamWas->cFramesCaptureToRelease = cFramesCaptured;
            pStreamWas->cbCapture               = PDMAudioPropsFramesToBytes(&pStreamWas->Cfg.Props, cFramesCaptured);
            /* Just loop and re-use the copying code above. Can optimize later. */
        }
        else
        {
            LogRelMax(64, ("WasAPI: GetBuffer() failed on '%s' during capture: %Rhrc (@%#RX64)\n",
                           pStreamWas->Cfg.szName, hrc, pStreamWas->offInternal));
            /** @todo reinit on AUDCLNT_E_DEVICEINVALIDATED? */
            rc = VERR_AUDIO_STREAM_NOT_READY;
            break;
        }
    }

    /*
     * Done.
     */
    uint64_t const msPrev = pStreamWas->msLastTransfer;
    uint64_t const msNow  = RTTimeMilliTS();
    if (cbRead)
        pStreamWas->msLastTransfer = msNow;

    RTCritSectLeave(&pStreamWas->CritSect);

    *pcbRead = cbRead;
    if (RT_SUCCESS(rc) || !cbRead)
    { }
    else
    {
        LogFlowFunc(("Suppressing %Rrc to report %#x bytes read\n", rc, cbRead));
        rc = VINF_SUCCESS;
    }
    LogFlowFunc(("@%#RX64: cbRead=%RU32 cMsDelta=%RU64 (%RU64 -> %RU64) {%s}\n", pStreamWas->offInternal, cbRead,
                 msPrev ? msNow - msPrev : 0, msPrev, pStreamWas->msLastTransfer, drvHostWasStreamStatusString(pStreamWas) ));
    return rc;
}


/*********************************************************************************************************************************
*   PDMDRVINS::IBase Interface                                                                                                   *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvHostAudioWasQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS          pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVHOSTAUDIOWAS    pThis   = PDMINS_2_DATA(pDrvIns, PDRVHOSTAUDIOWAS);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIHOSTAUDIO, &pThis->IHostAudio);
    return NULL;
}


/*********************************************************************************************************************************
*   PDMDRVREG Interface                                                                                                          *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNPDMDRVDESTRUCT, pfnDestruct}
 */
static DECLCALLBACK(void) drvHostAudioWasPowerOff(PPDMDRVINS pDrvIns)
{
    PDRVHOSTAUDIOWAS pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTAUDIOWAS);

    if (pThis->hWorkerThread != NIL_RTTHREAD)
    {
        BOOL fRc = PostThreadMessageW(pThis->idWorkerThread, WM_DRVHOSTAUDIOWAS_PURGE_CACHE, pThis->uWorkerThreadFixedParam, 0);
        LogFlowFunc(("Posted WM_DRVHOSTAUDIOWAS_PURGE_CACHE: %d\n", fRc));
        Assert(fRc); RT_NOREF(fRc);
    }
}


/**
 * @callback_method_impl{FNPDMDRVDESTRUCT, pfnDestruct}
 */
static DECLCALLBACK(void) drvHostAudioWasDestruct(PPDMDRVINS pDrvIns)
{
    PDRVHOSTAUDIOWAS pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTAUDIOWAS);
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    LogFlowFuncEnter();

    if (pThis->pNotifyClient)
    {
        pThis->pNotifyClient->notifyDriverDestroyed();
        pThis->pIEnumerator->UnregisterEndpointNotificationCallback(pThis->pNotifyClient);
        pThis->pNotifyClient->Release();
    }

    if (pThis->hWorkerThread != NIL_RTTHREAD)
    {
        BOOL fRc = PostThreadMessageW(pThis->idWorkerThread, WM_QUIT, 0, 0);
        Assert(fRc); RT_NOREF(fRc);

        int rc = RTThreadWait(pThis->hWorkerThread, RT_MS_15SEC, NULL);
        AssertRC(rc);
    }

    if (RTCritSectIsInitialized(&pThis->CritSectCache))
    {
        drvHostAudioWasCachePurge(pThis);
        RTCritSectDelete(&pThis->CritSectCache);
    }

    if (pThis->pIEnumerator)
    {
        uint32_t cRefs = pThis->pIEnumerator->Release(); RT_NOREF(cRefs);
        LogFlowFunc(("cRefs=%d\n", cRefs));
    }

    if (pThis->pIDeviceOutput)
    {
        pThis->pIDeviceOutput->Release();
        pThis->pIDeviceOutput = NULL;
    }

    if (pThis->pIDeviceInput)
    {
        pThis->pIDeviceInput->Release();
        pThis->pIDeviceInput = NULL;
    }


    if (RTCritSectRwIsInitialized(&pThis->CritSectStreamList))
        RTCritSectRwDelete(&pThis->CritSectStreamList);

    LogFlowFuncLeave();
}


/**
 * @callback_method_impl{FNPDMDRVCONSTRUCT, pfnConstruct}
 */
static DECLCALLBACK(int) drvHostAudioWasConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);
    PDRVHOSTAUDIOWAS pThis = PDMINS_2_DATA(pDrvIns, PDRVHOSTAUDIOWAS);
    RT_NOREF(fFlags, pCfg);

    /*
     * Init basic data members and interfaces.
     */
    pThis->pDrvIns                          = pDrvIns;
    pThis->hDrainTimer                      = NIL_TMTIMERHANDLE;
    pThis->hWorkerThread                    = NIL_RTTHREAD;
    pThis->idWorkerThread                   = 0;
    RTListInit(&pThis->StreamHead);
    RTListInit(&pThis->CacheHead);
    /* IBase */
    pDrvIns->IBase.pfnQueryInterface        = drvHostAudioWasQueryInterface;
    /* IHostAudio */
    pThis->IHostAudio.pfnGetConfig                  = drvHostAudioWasHA_GetConfig;
    pThis->IHostAudio.pfnGetDevices                 = drvHostAudioWasHA_GetDevices;
    pThis->IHostAudio.pfnGetStatus                  = drvHostAudioWasHA_GetStatus;
    pThis->IHostAudio.pfnStreamConfigHint           = drvHostAudioWasHA_StreamConfigHint;
    pThis->IHostAudio.pfnStreamCreate               = drvHostAudioWasHA_StreamCreate;
    pThis->IHostAudio.pfnStreamDestroy              = drvHostAudioWasHA_StreamDestroy;
    pThis->IHostAudio.pfnStreamNotifyDeviceChanged  = NULL;
    pThis->IHostAudio.pfnStreamControl              = drvHostAudioWasHA_StreamControl;
    pThis->IHostAudio.pfnStreamGetReadable          = drvHostAudioWasHA_StreamGetReadable;
    pThis->IHostAudio.pfnStreamGetWritable          = drvHostAudioWasHA_StreamGetWritable;
    pThis->IHostAudio.pfnStreamGetPending           = drvHostAudioWasHA_StreamGetPending;
    pThis->IHostAudio.pfnStreamGetStatus            = drvHostAudioWasHA_StreamGetStatus;
    pThis->IHostAudio.pfnStreamPlay                 = drvHostAudioWasHA_StreamPlay;
    pThis->IHostAudio.pfnStreamCapture              = drvHostAudioWasHA_StreamCapture;

    /*
     * Validate and read the configuration.
     */
    /** @todo We need a UUID for the session, while Pulse want some kind of name
     *        when creating the streams.  "StreamName" is confusing and a little
     *        misleading though, unless used only for Pulse.  Simply "VmName"
     *        would be a lot better and more generic.  */
    PDMDRV_VALIDATE_CONFIG_RETURN(pDrvIns, "VmName|VmUuid", "");
    /** @todo make it possible to override the default device selection. */

    AssertMsgReturn(PDMDrvHlpNoAttach(pDrvIns) == VERR_PDM_NO_ATTACHED_DRIVER,
                    ("Configuration error: Not possible to attach anything to this driver!\n"),
                    VERR_PDM_DRVINS_NO_ATTACH);

    /*
     * Initialize the critical sections early.
     */
    int rc = RTCritSectRwInit(&pThis->CritSectStreamList);
    AssertRCReturn(rc, rc);

    rc = RTCritSectInit(&pThis->CritSectCache);
    AssertRCReturn(rc, rc);

    /*
     * Create an enumerator instance that we can get the default devices from
     * as well as do enumeration thru.
     */
    HRESULT hrc = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                   (void **)&pThis->pIEnumerator);
    if (FAILED(hrc))
    {
        pThis->pIEnumerator = NULL;
        LogRel(("WasAPI: Failed to create an MMDeviceEnumerator object: %Rhrc\n", hrc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }
    AssertPtr(pThis->pIEnumerator);

    /*
     * Resolve the notification interface.
     */
    pThis->pIAudioNotifyFromHost = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIAUDIONOTIFYFROMHOST);
# ifdef VBOX_WITH_AUDIO_CALLBACKS
    AssertPtr(pThis->pIAudioNotifyFromHost);
# endif

    /*
     * Instantiate and register the notification client with the enumerator.
     *
     * Failure here isn't considered fatal at this time as we'll just miss
     * default device changes.
     */
    try
    {
        pThis->pNotifyClient = new DrvHostAudioWasMmNotifyClient(pThis);
    }
    catch (std::bad_alloc &)
    {
        return VERR_NO_MEMORY;
    }
    catch (int rcXcpt)
    {
        return rcXcpt;
    }
    hrc = pThis->pIEnumerator->RegisterEndpointNotificationCallback(pThis->pNotifyClient);
    AssertMsg(SUCCEEDED(hrc), ("%Rhrc\n", hrc));
    if (FAILED(hrc))
    {
        LogRel(("WasAPI: RegisterEndpointNotificationCallback failed: %Rhrc (ignored)\n"
                "WasAPI: Warning! Will not be able to detect default device changes!\n"));
        pThis->pNotifyClient->notifyDriverDestroyed();
        pThis->pNotifyClient->Release();
        pThis->pNotifyClient = NULL;
    }

    /*
     * Retrieve the input and output device.
     */
    IMMDevice *pIDeviceInput = NULL;
    if (pThis->pwszInputDevId)
        hrc = pThis->pIEnumerator->GetDevice(pThis->pwszInputDevId, &pIDeviceInput);
    else
        hrc = pThis->pIEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &pIDeviceInput);
    if (SUCCEEDED(hrc))
        LogFlowFunc(("pIDeviceInput=%p\n", pIDeviceInput));
    else
    {
        LogRel(("WasAPI: Failed to get audio input device '%ls': %Rhrc\n",
                pThis->pwszInputDevId ? pThis->pwszInputDevId : L"{Default}", hrc));
        pIDeviceInput = NULL;
    }

    IMMDevice *pIDeviceOutput = NULL;
    if (pThis->pwszOutputDevId)
        hrc = pThis->pIEnumerator->GetDevice(pThis->pwszOutputDevId, &pIDeviceOutput);
    else
        hrc = pThis->pIEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pIDeviceOutput);
    if (SUCCEEDED(hrc))
        LogFlowFunc(("pIDeviceOutput=%p\n", pIDeviceOutput));
    else
    {
        LogRel(("WasAPI: Failed to get audio output device '%ls': %Rhrc\n",
                pThis->pwszOutputDevId ? pThis->pwszOutputDevId : L"{Default}", hrc));
        pIDeviceOutput = NULL;
    }

    /* Carefully place them in the instance data:  */
    pThis->pNotifyClient->lockEnter();

    if (pThis->pIDeviceInput)
        pThis->pIDeviceInput->Release();
    pThis->pIDeviceInput = pIDeviceInput;

    if (pThis->pIDeviceOutput)
        pThis->pIDeviceOutput->Release();
    pThis->pIDeviceOutput = pIDeviceOutput;

    pThis->pNotifyClient->lockLeave();

    /*
     * We need a timer and a R/W critical section for draining streams.
     */
    rc = PDMDrvHlpTMTimerCreate(pDrvIns, TMCLOCK_REAL, drvHostWasDrainStopTimer, NULL /*pvUser*/, 0 /*fFlags*/,
                                "WasAPI drain", &pThis->hDrainTimer);
    AssertRCReturn(rc, rc);

    /*
     * Create the worker thread.  This thread has a message loop and will be
     * signalled by DrvHostAudioWasMmNotifyClient while the VM is paused/whatever,
     * so better make it a regular thread rather than PDM thread.
     */
    pThis->uWorkerThreadFixedParam = (WPARAM)RTRandU64();
    rc = RTThreadCreateF(&pThis->hWorkerThread, drvHostWasWorkerThread, pThis, 0 /*cbStack*/, RTTHREADTYPE_DEFAULT,
                         RTTHREADFLAGS_WAITABLE | RTTHREADFLAGS_COM_MTA, "WasWork%u", pDrvIns->iInstance);
    AssertRCReturn(rc, rc);

    rc = RTThreadUserWait(pThis->hWorkerThread, RT_MS_10SEC);
    AssertRC(rc);

    /*
     * Prime the cache.
     */
    drvHostAudioWasCacheFill(pThis);

    return VINF_SUCCESS;
}


/**
 * PDM driver registration for WasAPI.
 */
const PDMDRVREG g_DrvHostAudioWas =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "HostAudioWas",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Windows Audio Session API (WASAPI) host audio driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVHOSTAUDIOWAS),
    /* pfnConstruct */
    drvHostAudioWasConstruct,
    /* pfnDestruct */
    drvHostAudioWasDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    drvHostAudioWasPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};
