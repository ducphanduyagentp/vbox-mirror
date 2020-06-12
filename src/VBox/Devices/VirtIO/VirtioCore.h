/* $Id$ */

/** @file
 * VirtioCore.h - Virtio Declarations
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_VirtIO_VirtioCore_h
#define VBOX_INCLUDED_SRC_VirtIO_VirtioCore_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/ctype.h>
#include <iprt/sg.h>

/** Pointer to the shared VirtIO state. */
typedef struct VIRTIOCORE *PVIRTIOCORE;
/** Pointer to the ring-3 VirtIO state. */
typedef struct VIRTIOCORER3 *PVIRTIOCORER3;
/** Pointer to the ring-0 VirtIO state. */
typedef struct VIRTIOCORER0 *PVIRTIOCORER0;
/** Pointer to the raw-mode VirtIO state. */
typedef struct VIRTIOCORERC *PVIRTIOCORERC;
/** Pointer to the instance data for the current context. */
typedef CTX_SUFF(PVIRTIOCORE) PVIRTIOCORECC;

typedef enum VIRTIOVMSTATECHANGED
{
    kvirtIoVmStateChangedInvalid = 0,
    kvirtIoVmStateChangedReset,
    kvirtIoVmStateChangedSuspend,
    kvirtIoVmStateChangedPowerOff,
    kvirtIoVmStateChangedResume,
    kvirtIoVmStateChangedFor32BitHack = 0x7fffffff
} VIRTIOVMSTATECHANGED;

/**
 * Important sizing and bounds params for this impl. of VirtIO 1.0 PCI device
 */
 /**
  * TEMPORARY NOTE: Some of these values are experimental during development and will likely change.
  */
#define VIRTIO_MAX_VIRTQ_NAME_SIZE          32                   /**< Maximum length of a queue name           */
#define VIRTQ_MAX_ENTRIES                   1024                 /**< Max size (# desc elements) of a virtq    */
#define VIRTQ_MAX_CNT                       24                   /**< Max queues we allow guest to create      */
#define VIRTIO_NOTIFY_OFFSET_MULTIPLIER     2                    /**< VirtIO Notify Cap. MMIO config param     */
#define VIRTIO_REGION_PCI_CAP               2                    /**< BAR for VirtIO Cap. MMIO (impl specific) */
#define VIRTIO_REGION_MSIX_CAP              0                    /**< Bar for MSI-X handling                   */

#ifdef LOG_ENABLED
# define VIRTIO_HEX_DUMP(logLevel, pv, cb, base, title) \
    do { \
        if (LogIsItEnabled(logLevel, LOG_GROUP)) \
            virtioCoreHexDump((pv), (cb), (base), (title)); \
    } while (0)
#else
# define VIRTIO_HEX_DUMP(logLevel, pv, cb, base, title) do { } while (0)
#endif

typedef struct VIRTIOSGSEG                                      /**< An S/G entry                              */
{
    RTGCPHYS GCPhys;                                            /**< Pointer to the segment buffer             */
    size_t  cbSeg;                                              /**< Size of the segment buffer                */
} VIRTIOSGSEG;

typedef VIRTIOSGSEG *PVIRTIOSGSEG;
typedef const VIRTIOSGSEG *PCVIRTIOSGSEG;
typedef PVIRTIOSGSEG *PPVIRTIOSGSEG;

typedef struct VIRTIOSGBUF
{
    PVIRTIOSGSEG paSegs;                                        /**< Pointer to the scatter/gather array       */
    unsigned  cSegs;                                            /**< Number of segments                        */
    unsigned  idxSeg;                                           /**< Current segment we are in                 */
    RTGCPHYS  GCPhysCur;                                        /**< Ptr to byte within the current seg        */
    size_t    cbSegLeft;                                        /**< # of bytes left in the current segment    */
} VIRTIOSGBUF;

typedef VIRTIOSGBUF *PVIRTIOSGBUF;
typedef const VIRTIOSGBUF *PCVIRTIOSGBUF;
typedef PVIRTIOSGBUF *PPVIRTIOSGBUF;

/**
 * VirtIO buffers are actually descriptor chains. VirtIO's scatter-gather architecture
 * defines a head descriptor (index into ring of descriptors), which is chained to 0 or more
 * other descriptors that can optionally continue the chain.  This structure is VirtualBox's
 * Virtq buffer representation, which contains a reference to the head desc chain idx and
 * context for working with virtq buffers.
 */
typedef struct VIRTQBUF
{
    uint32_t            u32Magic;                                   /**< Magic value, VIRTQBUF_MAGIC.    */
    uint32_t volatile   cRefs;                                      /**< Reference counter. */
    uint32_t            uHeadIdx;                                   /**< Head idx of associated desc chain        */
    size_t              cbPhysSend;                                 /**< Total size of src buffer                 */
    PVIRTIOSGBUF        pSgPhysSend;                                /**< Phys S/G/ buf for data from guest        */
    size_t              cbPhysReturn;                               /**< Total size of dst buffer                 */
    PVIRTIOSGBUF        pSgPhysReturn;                              /**< Phys S/G buf to store result for guest   */

    /** @name Internal (bird combined 5 allocations into a single), fingers off.
     * @{ */
    VIRTIOSGBUF         SgBufIn;
    VIRTIOSGBUF         SgBufOut;
    VIRTIOSGSEG         aSegsIn[VIRTQ_MAX_ENTRIES];
    VIRTIOSGSEG         aSegsOut[VIRTQ_MAX_ENTRIES];
    /** @} */
} VIRTQBUF_T;

/** Pointers to a Virtio descriptor chain. */
typedef VIRTQBUF_T *PVIRTQBUF, **PPVIRTQBUF;
/** Magic value for VIRTQBUF_T::u32Magic. */
#define VIRTQBUF_MAGIC             UINT32_C(0x19600219)

typedef struct VIRTIOPCIPARAMS
{
    uint16_t  uDeviceId;                                         /**< PCI Cfg Device ID                        */
    uint16_t  uClassBase;                                        /**< PCI Cfg Base Class                       */
    uint16_t  uClassSub;                                         /**< PCI Cfg Subclass                         */
    uint16_t  uClassProg;                                        /**< PCI Cfg Programming Interface Class      */
    uint16_t  uSubsystemId;                                      /**< PCI Cfg Card Manufacturer Vendor ID      */
    uint16_t  uInterruptLine;                                    /**< PCI Cfg Interrupt line                   */
    uint16_t  uInterruptPin;                                     /**< PCI Cfg Interrupt pin                    */
} VIRTIOPCIPARAMS, *PVIRTIOPCIPARAMS;

#define VIRTIO_F_VERSION_1                  RT_BIT_64(32)        /**< Required feature bit for 1.0 devices      */

#define VIRTIO_F_INDIRECT_DESC              RT_BIT_64(28)        /**< Allow descs to point to list of descs     */
#define VIRTIO_F_EVENT_IDX                  RT_BIT_64(29)        /**< Allow notification disable for n elems    */
#define VIRTIO_F_RING_INDIRECT_DESC         RT_BIT_64(28)        /**< Doc bug: Goes under two names in spec     */
#define VIRTIO_F_RING_EVENT_IDX             RT_BIT_64(29)        /**< Doc bug: Goes under two names in spec     */

#define VIRTIO_DEV_INDEPENDENT_FEATURES_OFFERED ( 0 )            /**< TBD: Add VIRTIO_F_INDIRECT_DESC     */

#define VIRTIO_ISR_VIRTQ_INTERRUPT           RT_BIT_32(0)        /**< Virtq interrupt bit of ISR register       */
#define VIRTIO_ISR_DEVICE_CONFIG             RT_BIT_32(1)        /**< Device configuration changed bit of ISR   */
#define DEVICE_PCI_VENDOR_ID_VIRTIO                0x1AF4        /**< Guest driver locates dev via (mandatory)  */
#define DEVICE_PCI_REVISION_ID_VIRTIO                   1        /**< VirtIO 1.0 non-transitional drivers >= 1  */

/** Reserved (*negotiated*) Feature Bits (e.g. device independent features, VirtIO 1.0 spec,section 6) */

#define VIRTIO_MSI_NO_VECTOR                       0xffff        /**< Vector value to disable MSI for queue     */

/** Device Status field constants (from Virtio 1.0 spec) */
#define VIRTIO_STATUS_ACKNOWLEDGE                    0x01        /**< Guest driver: Located this VirtIO device  */
#define VIRTIO_STATUS_DRIVER                         0x02        /**< Guest driver: Can drive this VirtIO dev.  */
#define VIRTIO_STATUS_DRIVER_OK                      0x04        /**< Guest driver: Driver set-up and ready     */
#define VIRTIO_STATUS_FEATURES_OK                    0x08        /**< Guest driver: Feature negotiation done    */
#define VIRTIO_STATUS_FAILED                         0x80        /**< Guest driver: Fatal error, gave up        */
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET             0x40        /**< Device experienced unrecoverable error    */

/** @def Virtio Device PCI Capabilities type codes */
#define VIRTIO_PCI_CAP_COMMON_CFG                       1        /**< Common configuration PCI capability ID    */
#define VIRTIO_PCI_CAP_NOTIFY_CFG                       2        /**< Notification area PCI capability ID       */
#define VIRTIO_PCI_CAP_ISR_CFG                          3        /**< ISR PCI capability id                     */
#define VIRTIO_PCI_CAP_DEVICE_CFG                       4        /**< Device-specific PCI cfg capability ID     */
#define VIRTIO_PCI_CAP_PCI_CFG                          5        /**< PCI CFG capability ID                     */

#define VIRTIO_PCI_CAP_ID_VENDOR                     0x09        /**< Vendor-specific PCI CFG Device Cap. ID    */

/**
 * The following is the PCI capability struct common to all VirtIO capability types
 */
typedef struct virtio_pci_cap
{
    /* All little-endian */
    uint8_t   uCapVndr;                                          /**< Generic PCI field: PCI_CAP_ID_VNDR        */
    uint8_t   uCapNext;                                          /**< Generic PCI field: next ptr.              */
    uint8_t   uCapLen;                                           /**< Generic PCI field: capability length      */
    uint8_t   uCfgType;                                          /**< Identifies the structure.                 */
    uint8_t   uBar;                                              /**< Where to find it.                         */
    uint8_t   uPadding[3];                                       /**< Pad to full dword.                        */
    uint32_t  uOffset;                                           /**< Offset within bar.  (L.E.)                */
    uint32_t  uLength;                                           /**< Length of struct, in bytes. (L.E.)        */
}  VIRTIO_PCI_CAP_T, *PVIRTIO_PCI_CAP_T;

/**
 * Local implementation's usage context of a queue (e.g. not part of VirtIO specification)
 */
typedef struct VIRTQSTATE
{
    uint16_t  uVirtqNbr;                                          /**< Index of this queue                       */
    char      szVirtqName[32];                                   /**< Dev-specific name of queue                */
    uint16_t  uAvailIdxShadow;                                   /**< Consumer's position in avail ring         */
    uint16_t  uUsedIdxShadow;                                    /**< Consumer's position in used ring          */
    bool      fVirtqRingEventThreshold;                          /**< Don't lose track while queueing ahead     */
} VIRTQSTATE, *PVIRTQSTATE;

/**
 * VirtIO 1.0 Capabilities' related MMIO-mapped structs:
 *
 * Note: virtio_pci_device_cap is dev-specific, implemented by client. Definition unknown here.
 */
typedef struct virtio_pci_common_cfg
{
    /* Per device fields */
    uint32_t  uDeviceFeaturesSelect;                             /**< RW (driver selects device features)       */
    uint32_t  uDeviceFeatures;                                   /**< RO (device reports features to driver)    */
    uint32_t  uDriverFeaturesSelect;                             /**< RW (driver selects driver features)       */
    uint32_t  uDriverFeatures;                                   /**< RW (driver-accepted device features)      */
    uint16_t  uMsixConfig;                                       /**< RW (driver sets MSI-X config vector)      */
    uint16_t  uNumVirtqs;                                        /**< RO (device specifies max queues)          */
    uint8_t   uDeviceStatus;                                     /**< RW (driver writes device status, 0=reset) */
    uint8_t   uConfigGeneration;                                 /**< RO (device changes when changing configs) */

    /* Per virtqueue fields (as determined by uVirtqSelect) */
    uint16_t  uVirtqSelect;                                      /**< RW (selects queue focus for these fields) */
    uint16_t  uVirtqSize;                                        /**< RW (queue size, 0 - 2^n)                  */
    uint16_t  uVirtqMsixVector;                                  /**< RW (driver selects MSI-X queue vector)    */
    uint16_t  uVirtqEnable;                                      /**< RW (driver controls usability of queue)   */
    uint16_t  uVirtqNotifyOff;                                   /**< RO (offset uto virtqueue; see spec)       */
    uint64_t  aGCPhysVirtqDesc;                                  /**< RW (driver writes desc table phys addr)   */
    uint64_t  aGCPhysVirtqAvail;                                 /**< RW (driver writes avail ring phys addr)   */
    uint64_t  aGCPhysVirtqUsed;                                  /**< RW (driver writes used ring  phys addr)   */
} VIRTIO_PCI_COMMON_CFG_T, *PVIRTIO_PCI_COMMON_CFG_T;

typedef struct virtio_pci_notify_cap
{
    struct virtio_pci_cap pciCap;                                /**< Notification MMIO mapping capability      */
    uint32_t uNotifyOffMultiplier;                               /**< notify_off_multiplier                     */
} VIRTIO_PCI_NOTIFY_CAP_T, *PVIRTIO_PCI_NOTIFY_CAP_T;

typedef struct virtio_pci_cfg_cap
{
    struct virtio_pci_cap pciCap;                                /**< Cap. defines the BAR/off/len to access    */
    uint8_t uPciCfgData[4];                                      /**< I/O buf for above cap.                    */
} VIRTIO_PCI_CFG_CAP_T, *PVIRTIO_PCI_CFG_CAP_T;

/**
 * PCI capability data locations (PCI CFG and MMIO).
 */
typedef struct VIRTIO_PCI_CAP_LOCATIONS_T
{
    uint16_t        offMmio;
    uint16_t        cbMmio;
    uint16_t        offPci;
    uint16_t        cbPci;
} VIRTIO_PCI_CAP_LOCATIONS_T;

/**
 * The core/common state of the VirtIO PCI devices, shared edition.
 */
typedef struct VIRTIOCORE
{
    char                        szInstance[16];                     /**< Instance name, e.g. "VIRTIOSCSI0"         */
    PPDMDEVINS                  pDevInsR0;                          /**< Client device instance                    */
    PPDMDEVINS                  pDevInsR3;                          /**< Client device instance                    */
    RTGCPHYS                    aGCPhysVirtqDesc[VIRTQ_MAX_CNT];    /**< (MMIO) PhysAdr per-Q desc structs   GUEST */
    RTGCPHYS                    aGCPhysVirtqAvail[VIRTQ_MAX_CNT];   /**< (MMIO) PhysAdr per-Q avail structs  GUEST */
    RTGCPHYS                    aGCPhysVirtqUsed[VIRTQ_MAX_CNT];    /**< (MMIO) PhysAdr per-Q used structs   GUEST */
    uint16_t                    uVirtqNotifyOff[VIRTQ_MAX_CNT];     /**< (MMIO) per-Q notify offset           HOST */
    uint16_t                    uVirtqMsixVector[VIRTQ_MAX_CNT];    /**< (MMIO) Per-queue vector for MSI-X   GUEST */
    uint16_t                    uVirtqEnable[VIRTQ_MAX_CNT];        /**< (MMIO) Per-queue enable             GUEST */
    uint16_t                    uVirtqSize[VIRTQ_MAX_CNT];          /**< (MMIO) Per-queue size          HOST/GUEST */
    uint16_t                    uVirtqSelect;                       /**< (MMIO) queue selector               GUEST */
    uint16_t                    padding;
    uint64_t                    uDeviceFeatures;                    /**< (MMIO) Host features offered         HOST */
    uint64_t                    uDriverFeatures;                    /**< (MMIO) Host features accepted       GUEST */
    uint32_t                    uDeviceFeaturesSelect;              /**< (MMIO) hi/lo select uDeviceFeatures GUEST */
    uint32_t                    uDriverFeaturesSelect;              /**< (MMIO) hi/lo select uDriverFeatures GUEST */
    uint32_t                    uMsixConfig;                        /**< (MMIO) MSI-X vector                 GUEST */
    uint8_t                     uDeviceStatus;                      /**< (MMIO) Device Status                GUEST */
    uint8_t                     uPrevDeviceStatus;                  /**< (MMIO) Prev Device Status           GUEST */
    uint8_t                     uConfigGeneration;                  /**< (MMIO) Device config sequencer       HOST */
    VIRTQSTATE                  aVirtqState[VIRTQ_MAX_CNT];         /**< Local impl-specific queue context         */

    /** @name The locations of the capability structures in PCI config space and the BAR.
     * @{ */
    VIRTIO_PCI_CAP_LOCATIONS_T  LocPciCfgCap;                      /**< VIRTIO_PCI_CFG_CAP_T                       */
    VIRTIO_PCI_CAP_LOCATIONS_T  LocNotifyCap;                      /**< VIRTIO_PCI_NOTIFY_CAP_T                    */
    VIRTIO_PCI_CAP_LOCATIONS_T  LocCommonCfgCap;                   /**< VIRTIO_PCI_CAP_T                           */
    VIRTIO_PCI_CAP_LOCATIONS_T  LocIsrCap;                         /**< VIRTIO_PCI_CAP_T                           */
    VIRTIO_PCI_CAP_LOCATIONS_T  LocDeviceCap;                      /**< VIRTIO_PCI_CAP_T + custom data.            */
    /** @} */

    bool                        fGenUpdatePending;                 /**< If set, update cfg gen after driver reads  */
    uint8_t                     uPciCfgDataOff;                    /**< Offset to PCI configuration data area      */
    uint8_t                     uISR;                              /**< Interrupt Status Register.                 */
    uint8_t                     fMsiSupport;                       /**< Flag set if using MSI instead of ISR       */

    /** The MMIO handle for the PCI capability region (\#2). */
    IOMMMIOHANDLE               hMmioPciCap;

    /** @name Statistics
     * @{ */
    STAMCOUNTER                 StatDescChainsAllocated;
    STAMCOUNTER                 StatDescChainsFreed;
    STAMCOUNTER                 StatDescChainsSegsIn;
    STAMCOUNTER                 StatDescChainsSegsOut;
    /** @} */
} VIRTIOCORE;

#define MAX_NAME 64


/**
 * The core/common state of the VirtIO PCI devices, ring-3 edition.
 */
typedef struct VIRTIOCORER3
{
    /** @name Callbacks filled by the device before calling virtioCoreR3Init.
     * @{  */
    /**
     * Implementation-specific client callback to notify client of significant device status
     * changes.
     *
     * @param   pVirtio    Pointer to the shared virtio state.
     * @param   pVirtioCC  Pointer to the ring-3 virtio state.
     * @param   fDriverOk  True if guest driver is okay (thus queues, etc... are
     *                     valid)
     */
    DECLCALLBACKMEMBER(void, pfnStatusChanged)(PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC, uint32_t fDriverOk);

    /**
     * Implementation-specific client callback to access VirtIO Device-specific capabilities
     * (other VirtIO capabilities and features are handled in VirtIO implementation)
     *
     * @param   pDevIns    The device instance.
     * @param   offCap     Offset within device specific capabilities struct.
     * @param   pvBuf      Buffer in which to save read data.
     * @param   cbToRead   Number of bytes to read.
     */
    DECLCALLBACKMEMBER(int,  pfnDevCapRead)(PPDMDEVINS pDevIns, uint32_t offCap, void *pvBuf, uint32_t cbToRead);

    /**
     * Implementation-specific client ballback to access VirtIO Device-specific capabilities
     * (other VirtIO capabilities and features are handled in VirtIO implementation)
     *
     * @param   pDevIns    The device instance.
     * @param   offCap     Offset within device specific capabilities struct.
     * @param   pvBuf      Buffer with the bytes to write.
     * @param   cbToWrite  Number of bytes to write.
     */
    DECLCALLBACKMEMBER(int,  pfnDevCapWrite)(PPDMDEVINS pDevIns, uint32_t offCap, const void *pvBuf, uint32_t cbWrite);


    /**
     * When guest-to-host queue notifications are enabled, the guest driver notifies the host
     * that the avail queue has buffers, and this callback informs the client.
     *
     * @param   pVirtio    Pointer to the shared virtio state.
     * @param   pVirtioCC  Pointer to the ring-3 virtio state.
     * @param   uVirtqNbr   Index of the notified queue
     */
    DECLCALLBACKMEMBER(void, pfnVirtqNotified)(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr);

    /** @} */


    R3PTRTYPE(PVIRTIO_PCI_CFG_CAP_T)    pPciCfgCap;                 /**< Pointer to struct in the PCI configuration area. */
    R3PTRTYPE(PVIRTIO_PCI_NOTIFY_CAP_T) pNotifyCap;                 /**< Pointer to struct in the PCI configuration area. */
    R3PTRTYPE(PVIRTIO_PCI_CAP_T)        pCommonCfgCap;              /**< Pointer to struct in the PCI configuration area. */
    R3PTRTYPE(PVIRTIO_PCI_CAP_T)        pIsrCap;                    /**< Pointer to struct in the PCI configuration area. */
    R3PTRTYPE(PVIRTIO_PCI_CAP_T)        pDeviceCap;                 /**< Pointer to struct in the PCI configuration area. */

    uint32_t                    cbDevSpecificCfg;                   /**< Size of client's dev-specific config data */
    R3PTRTYPE(uint8_t *)        pbDevSpecificCfg;                   /**< Pointer to client's struct                */
    R3PTRTYPE(uint8_t *)        pbPrevDevSpecificCfg;               /**< Previous read dev-specific cfg of client  */
    bool                        fGenUpdatePending;                  /**< If set, update cfg gen after driver reads */
    char                        pcszMmioName[MAX_NAME];             /**< MMIO mapping name                         */
} VIRTIOCORER3;


/**
 * The core/common state of the VirtIO PCI devices, ring-0 edition.
 */
typedef struct VIRTIOCORER0
{

    /**
     * When guest-to-host queue notifications are enabled, the guest driver notifies the host
     * that the avail queue has buffers, and this callback informs the client.
     *
     * @param   pVirtio    Pointer to the shared virtio state.
     * @param   pVirtioCC  Pointer to the ring-3 virtio state.
     * @param   uVirtqNbr   Index of the notified queue
     */
    DECLCALLBACKMEMBER(void, pfnVirtqNotified)(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr);

} VIRTIOCORER0;


/**
 * The core/common state of the VirtIO PCI devices, raw-mode edition.
 */
typedef struct VIRTIOCORERC
{
    uint64_t                    uUnusedAtTheMoment;
} VIRTIOCORERC;


/** @typedef VIRTIOCORECC
 * The instance data for the current context. */
typedef CTX_SUFF(VIRTIOCORE) VIRTIOCORECC;


/** @name API for VirtIO parent device
 * @{ */


/**
 * Initiate orderly reset procedure. This is an exposed API for clients that might need it.
 * Invoked by client to reset the device and driver (see VirtIO 1.0 section 2.1.1/2.1.2)
 *
 * @param   pVirtio     Pointer to the virtio state.
 */
void     virtioCoreResetAll(PVIRTIOCORE pVirtio);

/**
 * 'Attaches' the inheriting device-specific code's queue state to the VirtIO core
 * queue management, informing the core of the name of the queue and number. The VirtIO core
 * allocates the queue state information so it can handle all the core VirtiIO queue operations
 * and dispatch callbacks, etc...
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr    Virtq number
 * @param   pcszName    Name to give queue
 *
 * @returns VBox status code.
 */
int      virtioCoreR3VirtqAttach(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, const char *pcszName);

/**
 * Enables or disables a virtq
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr   Virtq number
 * @param   fEnable     Flags whether to enable or disable the virtq
 *
 */

void     virtioCoreVirtqEnable(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, bool fEnable);

/**
 * Enable or Disable notification for the specified queue
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr    Virtq number
 * @param   fEnable    Selects notification mode (enabled or disabled)
 */
void     virtioCoreVirtqEnableNotify(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, bool fEnable);

/*
 * Notifies guest (via ISR or MSI-X) of device configuration change
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 */
void     virtioCoreNotifyConfigChanged(PVIRTIOCORE pVirtio);

/*
 * Displays the VirtIO spec-related features offered by the core component,
 * as well as which features have been negotiated and accepted or declined by the guest driver,
 * providing a summary view of the configuration the device is operating with.
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   pHlp        Pointer to the debug info hlp struct
 */
void     virtioCorePrintFeatures(VIRTIOCORE *pVirtio, PCDBGFINFOHLP pHlp);

/*
 * Debuging assist feature displays the state of the VirtIO core code, which includes
 * an overview of the state of all of the queues.
 *
 * This can be invoked when running the VirtualBox debugger, or from the command line
 * using the command: "VboxManage debugvm <VM name or id> info <device name> [args]"
 *
 * Example:  VBoxManage debugvm myVnetVm info "virtio-net" all
 *
 * This is implemented currently to be invoked by the inheriting device-specific code
 * (see DevVirtioNet for an example, which receives the debugvm command directly).
 * That devices lists the available sub-options if no arguments are provided. In that
 * example this virtq info related function is invoked hierarchically when virtio-net
 * displays its device-specific queue info.
 *
 * @param   pDevIns     The device instance.
 * @param   pHlp        Pointer to the debug info hlp struct
 * @param   pszArgs     Arguments to function
 */
void     virtioCoreR3VirtqInfo(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs, int uVirtqNbr);

/*
 * Returns the number of avail bufs in the virtq.
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr   Virtqueue to return the count of buffers available for.
 */
uint16_t virtioCoreVirtqAvailCount(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr);

/**
 * Fetches descriptor chain using avail ring of indicated queue and converts the descriptor
 * chain into its OUT (to device) and IN to guest components, but does NOT remove it from
 * the 'avail' queue. I.e. doesn't advance the index.  This can be used with virtioVirtqSkip(),
 * which *does* advance the avail index. Together they facilitate a mechanism that allows
 * work with a queue element (descriptor chain) to be aborted if necessary, by not advancing
 * the pointer, or, upon success calling the skip function (above) to move to the next element.
 *
 * Additionally it converts the OUT desc chain data to a contiguous virtual
 * memory buffer for easy consumption by the caller. The caller must return the
 * descriptor chain pointer via virtioCoreR3VirtqBufPut() and then call virtioCoreVirtqSync()
 * at some point to return the data to the guest and complete the transaction.
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr   Virtq number
 * @param   ppVirtqBuf Address to store pointer to descriptor chain that contains the
 *                      pre-processed transaction information pulled from the virtq.
 *
 * @returns VBox status code:
 * @retval  VINF_SUCCESS         Success
 * @retval  VERR_INVALID_STATE   VirtIO not in ready state (asserted).
 * @retval  VERR_NOT_AVAILABLE   If the queue is empty.
 */
int      virtioCoreR3VirtqBufPeek(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                                  PPVIRTQBUF ppVirtqBuf);

/**
 * Fetches the next descriptor chain using avail ring of indicated queue and converts the descriptor
 * chain into its OUT (to device) and IN to guest components.
 *
 * Additionally it converts the OUT desc chain data to a contiguous virtual
 * memory buffer for easy consumption by the caller. The caller must return the
 * descriptor chain pointer via virtioCoreR3VirtqBufPut() and then call virtioCoreVirtqSync()
 * at some point to return the data to the guest and complete the transaction.
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr    Virtq number
 * @param   ppVirtqBuf Address to store pointer to descriptor chain that contains the
 *                      pre-processed transaction information pulled from the virtq.
 *                      Returned reference must be released by calling
 *                      virtioCoreR3VirtqBufRelease().
 * @param   fRemove     flags whether to remove desc chain from queue (false = peek)
 *
 * @returns VBox status code:
 * @retval  VINF_SUCCESS         Success
 * @retval  VERR_INVALID_STATE   VirtIO not in ready state (asserted).
 * @retval  VERR_NOT_AVAILABLE   If the queue is empty.
 */
int      virtioCoreR3VirtqBufGet(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                                 PPVIRTQBUF ppVirtqBuf, bool fRemove);

/**
 * Fetches a specific descriptor chain using avail ring of indicated queue and converts the descriptor
 * chain into its OUT (to device) and IN to guest components.
 *
 * Additionally it converts the OUT desc chain data to a contiguous virtual
 * memory buffer for easy consumption by the caller. The caller must return the
 * descriptor chain pointer via virtioCoreR3VirtqBufPut() and then call virtioCoreVirtqSync()
 * at some point to return the data to the guest and complete the transaction.
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr    Virtq number
 * @param   ppVirtqBuf Address to store pointer to descriptor chain that contains the
 *                      pre-processed transaction information pulled from the virtq.
 *                      Returned reference must be released by calling
 *                      virtioCoreR3VirtqBufRelease().
 * @param   fRemove     flags whether to remove desc chain from queue (false = peek)
 *
 * @returns VBox status code:
 * @retval  VINF_SUCCESS         Success
 * @retval  VERR_INVALID_STATE   VirtIO not in ready state (asserted).
 * @retval  VERR_NOT_AVAILABLE   If the queue is empty.
 */
int      virtioCoreR3VirtqBufGet(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                                  uint16_t uHeadIdx, PPVIRTQBUF ppVirtqBuf);

/**
 * Returns data to the guest to complete a transaction initiated by virtVirtqGet().
 *
 * The caller passes in a pointer to a scatter-gather buffer of virtual memory segments
 * and a pointer to the descriptor chain context originally derived from the pulled
 * queue entry, and this function will write the virtual memory s/g buffer into the
 * guest's physical memory free the descriptor chain. The caller handles the freeing
 * (as needed) of the virtual memory buffer.
 *
 * @note This does a write-ahead to the used ring of the guest's queue. The data
 *       written won't be seen by the guest until the next call to virtioCoreVirtqSync()
 *
 *
 * @param   pDevIns         The device instance (for reading).
 * @param   pVirtio         Pointer to the shared virtio state.
 * @param   uVirtqNbr        Virtq number
 *
 * @param   pSgVirtReturn   Points to scatter-gather buffer of virtual memory
 *                          segments the caller is returning to the guest.
 *
 * @param   pVirtqBuf      This contains the context of the scatter-gather
 *                          buffer originally pulled from the queue.
 *
 * @param   fFence          If true, put up copy fence (memory barrier) after
 *                          copying to guest phys. mem.
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS       Success
 * @retval  VERR_INVALID_STATE VirtIO not in ready state
 * @retval  VERR_NOT_AVAILABLE Virtq is empty
 *
 * @note    This function will not release any reference to pVirtqBuf.  The
 *          caller must take care of that.
 */
int      virtioCoreR3VirtqBufPut(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, PRTSGBUF pSgVirtReturn,
                                 PVIRTQBUF pVirtqBuf, bool fFence);

/**
 * Skip the next entry in the specified queue (typically used with virtioCoreR3VirtqBufPeek())
 *
 * @param   pVirtio     Pointer to the virtio state.
 * @param   uVirtqNbr    Index of queue
 */
int      virtioCoreR3VirtqBufSkip(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr);

/**
 * Updates the indicated virtq's "used ring" descriptor index to match the
 * current write-head index, thus exposing the data added to the used ring by all
 * virtioCoreR3VirtqBufPut() calls since the last sync. This should be called after one or
 * more virtioCoreR3VirtqBufPut() calls to inform the guest driver there is data in the queue.
 * Explicit notifications (e.g. interrupt or MSI-X) will be sent to the guest,
 * depending on VirtIO features negotiated and conditions, otherwise the guest
 * will detect the update by polling. (see VirtIO 1.0 specification, Section 2.4 "Virtqueues").
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr   Virtq number
 *
 * @returns VBox status code.
 * @retval  VINF_SUCCESS       Success
 * @retval  VERR_INVALID_STATE VirtIO not in ready state
 */
int      virtioCoreVirtqSync(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr);

/**
 * Retains a reference to the given descriptor chain.
 *
 * @returns New reference count.
 * @retval  UINT32_MAX on invalid parameter.
 * @param   pVirtqBuf      The descriptor chain to reference.
 */
uint32_t virtioCoreR3VirtqBufRetain(PVIRTQBUF pVirtqBuf);

/**
 * Releases a reference to the given descriptor chain.
 *
 * @returns New reference count.
 * @retval  0 if freed or invalid parameter.
 * @param   pVirtio         Pointer to the shared virtio state.
 * @param   pVirtqBuf       The descriptor chain to reference.  NULL is quietly
 *                          ignored (returns 0).
 */
uint32_t virtioCoreR3VirtqBufRelease(PVIRTIOCORE pVirtio, PVIRTQBUF pVirtqBuf);

/**
 * Return queue enable state
 *
 * @param   pVirtio     Pointer to the virtio state.
 * @param   uVirtqNbr    Virtq number.
 * @returns true or false indicating whether to enable queue or not
 */
DECLINLINE(bool) virtioCoreIsVirtqEnabled(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    return pVirtio->uVirtqEnable[uVirtqNbr] != 0;
}

/**
 * Get name of queue, by uVirtqNbr, assigned at virtioCoreR3VirtqAttach()
 *
 * @param   pVirtio     Pointer to the virtio state.
 * @param   uVirtqNbr    Virtq number.
 *
 * @returns Pointer to read-only queue name.
 */
DECLINLINE(const char *) virtioCoreVirtqGetName(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    Assert((size_t)uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    return pVirtio->aVirtqState[uVirtqNbr].szVirtqName;
}

/**
 * Get the features VirtIO is running withnow.
 *
 * @returns Features the guest driver has accepted, finalizing the operational features
 */
DECLINLINE(uint64_t) virtioCoreGetNegotiatedFeatures(PVIRTIOCORE pVirtio)
{
    return pVirtio->uDriverFeatures;
}

/**
 * Calculate the length of a GCPhys s/g buffer by tallying the size of each segment.
 *
 * @param   pGcSgBuf        GC S/G buffer to calculate length of
 */
DECLINLINE(size_t) virtioCoreSgBufCalcTotalLength(PCVIRTIOSGBUF pGcSgBuf)
{
    size_t   cb = 0;
    unsigned i  = pGcSgBuf->cSegs;
    while (i-- > 0)
        cb += pGcSgBuf->paSegs[i].cbSeg;
    return cb;
}

/**
 * Log memory-mapped I/O input or output value.
 *
 * This is to be invoked by macros that assume they are invoked in functions with
 * the relevant arguments. (See Virtio_1_0.cpp).
 *
 * It is exposed via the API so inheriting device-specific clients can provide similar
 * logging capabilities for a consistent look-and-feel.
 *
 * @param   pszFunc     To avoid displaying this function's name via __FUNCTION__ or LogFunc()
 * @param   pszMember   Name of struct member
 * @param   pv          pointer to value
 * @param   cb          size of value
 * @param   uOffset     offset into member where value starts
 * @param   fWrite      True if write I/O
 * @param   fHasIndex   True if the member is indexed
 * @param   idx         The index if fHasIndex
 */
const char *virtioCoreGetStateChangeText(VIRTIOVMSTATECHANGED enmState);void virtioCoreLogMappedIoValue(const char *pszFunc, const char *pszMember, uint32_t uMemberSize,
                                const void *pv, uint32_t cb, uint32_t uOffset,
                                int fWrite, int fHasIndex, uint32_t idx);

/**
 * Debug assist for any consumer device code that inherits VIRTIOCORE
 *
 * Does a formatted hex dump using Log(()), recommend using VIRTIO_HEX_DUMP() macro to
 * control enabling of logging efficiently.
 *
 * @param   pv          pointer to buffer to dump contents of
 * @param   cb          count of characters to dump from buffer
 * @param   uBase       base address of per-row address prefixing of hex output
 * @param   pszTitle    Optional title. If present displays title that lists
 *                      provided text with value of cb to indicate size next to it.
 */
void virtioCoreHexDump(uint8_t *pv, uint32_t cb, uint32_t uBase, const char *pszTitle);

/**
 * Debug assist for any consumer device code that inherits VIRTIOCORE
&
 * Do a hex dump of memory in guest physical context
 *
 * @param   GCPhys      pointer to buffer to dump contents of
 * @param   cb          count of characters to dump from buffer
 * @param   uBase       base address of per-row address prefixing of hex output
 * @param   pszTitle    Optional title. If present displays title that lists
 *                      provided text with value of cb to indicate size next to it.
 */
void virtioCoreGCPhysHexDump(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint16_t cb, uint32_t uBase, const char *pszTitle);

/** The following virtioCoreSgBuf*() functions mimic the functionality of the related RT s/g functions,
 *  except they work with the data type GCPhys rather than void *
 */
void     virtioCoreSgBufInit(PVIRTIOSGBUF pGcSgBuf, PVIRTIOSGSEG paSegs, size_t cSegs);
void     virtioCoreSgBufReset(PVIRTIOSGBUF pGcSgBuf);
RTGCPHYS virtioCoreSgBufGetNextSegment(PVIRTIOSGBUF pGcSgBuf, size_t *pcbSeg);
RTGCPHYS virtioCoreSgBufAdvance(PVIRTIOSGBUF pGcSgBuf, size_t cbAdvance);
void     virtioCoreSgBufInit(PVIRTIOSGBUF pSgBuf, PVIRTIOSGSEG paSegs, size_t cSegs);
size_t   virtioCoreSgBufCalcTotalLength(PCVIRTIOSGBUF pGcSgBuf);
void     virtioCoreSgBufReset(PVIRTIOSGBUF pGcSgBuf);
size_t   virtioCoreSgBufCalcTotalLength(PVIRTIOSGBUF pGcSgBuf);

/** Misc VM and PDM boilerplate */
int      virtioCoreR3SaveExec(PVIRTIOCORE pVirtio, PCPDMDEVHLPR3 pHlp, PSSMHANDLE pSSM);
int      virtioCoreR3LoadExec(PVIRTIOCORE pVirtio, PCPDMDEVHLPR3 pHlp, PSSMHANDLE pSSM);
void     virtioCoreR3VmStateChanged(PVIRTIOCORE pVirtio, VIRTIOVMSTATECHANGED enmState);
void     virtioCoreR3Term(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC);
int      virtioCoreR3Init(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC, PVIRTIOPCIPARAMS pPciParams,
                      const char *pcszInstance, uint64_t fDevSpecificFeatures, void *pvDevSpecificCfg, uint16_t cbDevSpecificCfg);
int      virtioCoreRZInit(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio);


const char *virtioCoreGetStateChangeText(VIRTIOVMSTATECHANGED enmState);

/** @} */


#endif /* !VBOX_INCLUDED_SRC_VirtIO_VirtioCore_h */
