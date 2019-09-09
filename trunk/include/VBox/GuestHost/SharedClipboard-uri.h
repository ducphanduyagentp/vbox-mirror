/* $Id$ */
/** @file
 * Shared Clipboard - Shared URI functions between host and guest.
 */

/*
 * Copyright (C) 2019 Oracle Corporation
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

#ifndef VBOX_INCLUDED_GuestHost_SharedClipboard_uri_h
#define VBOX_INCLUDED_GuestHost_SharedClipboard_uri_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <map>

#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/fs.h>
#include <iprt/list.h>

#include <iprt/cpp/list.h>
#include <iprt/cpp/ministring.h>

#include <VBox/GuestHost/SharedClipboard.h>

/** @name Shared Clipboard handles.
 *  @{
 */

/** A Shared Clipboard list handle. */
typedef uint64_t SHCLLISTHANDLE;
/** Pointer to a Shared Clipboard list handle. */
typedef SHCLLISTHANDLE *PSHCLLISTHANDLE;

/** Specifies an invalid Shared Clipboard list handle. */
#define SHCLLISTHANDLE_INVALID        ((SHCLLISTHANDLE)~0LL)

/** A Shared Clipboard object handle. */
typedef uint64_t SHCLOBJHANDLE;
/** Pointer to a Shared Clipboard object handle. */
typedef SHCLOBJHANDLE *PSHCLOBJHANDLE;

/** Specifies an invalid Shared Clipboard object handle. */
#define SHCLOBJHANDLE_INVALID         ((SHCLOBJHANDLE)~0LL)

/** @} */

/** @name Shared Clipboard open/create flags.
 *  @{
 */

/** No flags. Initialization value. */
#define SHCL_OBJ_CF_NONE                  (0x00000000)

/** Lookup only the object, do not return a handle. All other flags are ignored. */
#define SHCL_OBJ_CF_LOOKUP                (0x00000001)

/** Create/open a directory. */
#define SHCL_OBJ_CF_DIRECTORY             (0x00000004)

/** Open/create action to do if object exists
 *  and if the object does not exists.
 *  REPLACE file means atomically DELETE and CREATE.
 *  OVERWRITE file means truncating the file to 0 and
 *  setting new size.
 *  When opening an existing directory REPLACE and OVERWRITE
 *  actions are considered invalid, and cause returning
 *  FILE_EXISTS with NIL handle.
 */
#define SHCL_OBJ_CF_ACT_MASK_IF_EXISTS      (0x000000F0)
#define SHCL_OBJ_CF_ACT_MASK_IF_NEW         (0x00000F00)

/** What to do if object exists. */
#define SHCL_OBJ_CF_ACT_OPEN_IF_EXISTS      (0x00000000)
#define SHCL_OBJ_CF_ACT_FAIL_IF_EXISTS      (0x00000010)
#define SHCL_OBJ_CF_ACT_REPLACE_IF_EXISTS   (0x00000020)
#define SHCL_OBJ_CF_ACT_OVERWRITE_IF_EXISTS (0x00000030)

/** What to do if object does not exist. */
#define SHCL_OBJ_CF_ACT_CREATE_IF_NEW       (0x00000000)
#define SHCL_OBJ_CF_ACT_FAIL_IF_NEW         (0x00000100)

/** Read/write requested access for the object. */
#define SHCL_OBJ_CF_ACCESS_MASK_RW          (0x00003000)

/** No access requested. */
#define SHCL_OBJ_CF_ACCESS_NONE             (0x00000000)
/** Read access requested. */
#define SHCL_OBJ_CF_ACCESS_READ             (0x00001000)
/** Write access requested. */
#define SHCL_OBJ_CF_ACCESS_WRITE            (0x00002000)
/** Read/Write access requested. */
#define SHCL_OBJ_CF_ACCESS_READWRITE        (SHCL_OBJ_CF_ACCESS_READ | SHCL_OBJ_CF_ACCESS_WRITE)

/** Requested share access for the object. */
#define SHCL_OBJ_CF_ACCESS_MASK_DENY        (0x0000C000)

/** Allow any access. */
#define SHCL_OBJ_CF_ACCESS_DENYNONE         (0x00000000)
/** Do not allow read. */
#define SHCL_OBJ_CF_ACCESS_DENYREAD         (0x00004000)
/** Do not allow write. */
#define SHCL_OBJ_CF_ACCESS_DENYWRITE        (0x00008000)
/** Do not allow access. */
#define SHCL_OBJ_CF_ACCESS_DENYALL          (SHCL_OBJ_CF_ACCESS_DENYREAD | SHCL_OBJ_CF_ACCESS_DENYWRITE)

/** Requested access to attributes of the object. */
#define SHCL_OBJ_CF_ACCESS_MASK_ATTR        (0x00030000)

/** No access requested. */
#define SHCL_OBJ_CF_ACCESS_ATTR_NONE        (0x00000000)
/** Read access requested. */
#define SHCL_OBJ_CF_ACCESS_ATTR_READ        (0x00010000)
/** Write access requested. */
#define SHCL_OBJ_CF_ACCESS_ATTR_WRITE       (0x00020000)
/** Read/Write access requested. */
#define SHCL_OBJ_CF_ACCESS_ATTR_READWRITE   (SHCL_OBJ_CF_ACCESS_ATTR_READ | SHCL_OBJ_CF_ACCESS_ATTR_WRITE)

/** The file is opened in append mode. Ignored if SHCL_OBJ_CF_ACCESS_WRITE is not set. */
#define SHCL_OBJ_CF_ACCESS_APPEND           (0x00040000)

/** @} */

/** Result of an open/create request.
 *  Along with handle value the result code
 *  identifies what has happened while
 *  trying to open the object.
 */
typedef enum _SHCLCREATERESULT
{
    SHCL_CREATERESULT_NONE,
    /** Specified path does not exist. */
    SHCL_CREATERESULT_PATH_NOT_FOUND,
    /** Path to file exists, but the last component does not. */
    SHCL_CREATERESULT_FILE_NOT_FOUND,
    /** File already exists and either has been opened or not. */
    SHCL_CREATERESULT_FILE_EXISTS,
    /** New file was created. */
    SHCL_CREATERESULT_FILE_CREATED,
    /** Existing file was replaced or overwritten. */
    SHCL_CREATERESULT_FILE_REPLACED,
    /** Blow the type up to 32-bit. */
    SHCL_CREATERESULT_32BIT_HACK = 0x7fffffff
} SHCLCREATERESULT;
AssertCompile(SHCL_CREATERESULT_NONE == 0);
AssertCompileSize(SHCLCREATERESULT, 4);

/**
 * The available additional information in a SHCLFSOBJATTR object.
 */
typedef enum _SHCLFSOBJATTRADD
{
    /** No additional information is available / requested. */
    SHCLFSOBJATTRADD_NOTHING = 1,
    /** The additional unix attributes (SHCLFSOBJATTR::u::Unix) are
     *  available / requested. */
    SHCLFSOBJATTRADD_UNIX,
    /** The additional extended attribute size (SHCLFSOBJATTR::u::EASize) is
     *  available / requested. */
    SHCLFSOBJATTRADD_EASIZE,
    /** The last valid item (inclusive).
     * The valid range is SHCLFSOBJATTRADD_NOTHING thru
     * SHCLFSOBJATTRADD_LAST. */
    SHCLFSOBJATTRADD_LAST = SHCLFSOBJATTRADD_EASIZE,

    /** The usual 32-bit hack. */
    SHCLFSOBJATTRADD_32BIT_SIZE_HACK = 0x7fffffff
} SHCLFSOBJATTRADD;


/* Assert sizes of the IRPT types we're using below. */
AssertCompileSize(RTFMODE,      4);
AssertCompileSize(RTFOFF,       8);
AssertCompileSize(RTINODE,      8);
AssertCompileSize(RTTIMESPEC,   8);
AssertCompileSize(RTDEV,        4);
AssertCompileSize(RTUID,        4);

/**
 * Shared Clipboard filesystem object attributes.
 */
#pragma pack(1)
typedef struct _SHCLFSOBJATTR
{
    /** Mode flags (st_mode). RTFS_UNIX_*, RTFS_TYPE_*, and RTFS_DOS_*.
     * @remarks We depend on a number of RTFS_ defines to remain unchanged.
     *          Fortuntately, these are depending on windows, dos and unix
     *          standard values, so this shouldn't be much of a pain. */
    RTFMODE          fMode;

    /** The additional attributes available. */
    SHCLFSOBJATTRADD enmAdditional;

    /**
     * Additional attributes.
     *
     * Unless explicitly specified to an API, the API can provide additional
     * data as it is provided by the underlying OS.
     */
    union SHCLFSOBJATTRUNION
    {
        /** Additional Unix Attributes
         * These are available when SHCLFSOBJATTRADD is set in fUnix.
         */
         struct SHCLFSOBJATTRUNIX
         {
            /** The user owning the filesystem object (st_uid).
             * This field is ~0U if not supported. */
            RTUID           uid;

            /** The group the filesystem object is assigned (st_gid).
             * This field is ~0U if not supported. */
            RTGID           gid;

            /** Number of hard links to this filesystem object (st_nlink).
             * This field is 1 if the filesystem doesn't support hardlinking or
             * the information isn't available.
             */
            uint32_t        cHardlinks;

            /** The device number of the device which this filesystem object resides on (st_dev).
             * This field is 0 if this information is not available. */
            RTDEV           INodeIdDevice;

            /** The unique identifier (within the filesystem) of this filesystem object (st_ino).
             * Together with INodeIdDevice, this field can be used as a OS wide unique id
             * when both their values are not 0.
             * This field is 0 if the information is not available. */
            RTINODE         INodeId;

            /** User flags (st_flags).
             * This field is 0 if this information is not available. */
            uint32_t        fFlags;

            /** The current generation number (st_gen).
             * This field is 0 if this information is not available. */
            uint32_t        GenerationId;

            /** The device number of a character or block device type object (st_rdev).
             * This field is 0 if the file isn't of a character or block device type and
             * when the OS doesn't subscribe to the major+minor device idenfication scheme. */
            RTDEV           Device;
        } Unix;

        /**
         * Extended attribute size.
         */
        struct SHCLFSOBJATTREASIZE
        {
            /** Size of EAs. */
            RTFOFF          cb;
        } EASize;
    } u;
} SHCLFSOBJATTR;
#pragma pack()
AssertCompileSize(SHCLFSOBJATTR, 44);
/** Pointer to a Shared Clipboard filesystem object attributes structure. */
typedef SHCLFSOBJATTR *PSHCLFSOBJATTR;
/** Pointer to a const Shared Clipboard filesystem object attributes structure. */
typedef const SHCLFSOBJATTR *PCSHCLFSOBJATTR;

/**
 * Shared Clipboard file system object information structure.
 */
#pragma pack(1)
typedef struct _SHCLFSOBJINFO
{
   /** Logical size (st_size).
    * For normal files this is the size of the file.
    * For symbolic links, this is the length of the path name contained
    * in the symbolic link.
    * For other objects this fields needs to be specified.
    */
   RTFOFF       cbObject;

   /** Disk allocation size (st_blocks * DEV_BSIZE). */
   RTFOFF       cbAllocated;

   /** Time of last access (st_atime).
    * @remarks  Here (and other places) we depend on the IPRT timespec to
    *           remain unchanged. */
   RTTIMESPEC   AccessTime;

   /** Time of last data modification (st_mtime). */
   RTTIMESPEC   ModificationTime;

   /** Time of last status change (st_ctime).
    * If not available this is set to ModificationTime.
    */
   RTTIMESPEC   ChangeTime;

   /** Time of file birth (st_birthtime).
    * If not available this is set to ChangeTime.
    */
   RTTIMESPEC   BirthTime;

   /** Attributes. */
   SHCLFSOBJATTR Attr;

} SHCLFSOBJINFO;
#pragma pack()
AssertCompileSize(SHCLFSOBJINFO, 92);
/** Pointer to a Shared Clipboard filesystem object information structure. */
typedef SHCLFSOBJINFO *PSHCLFSOBJINFO;
/** Pointer to a const Shared Clipboard filesystem object information
 *  structure. */
typedef const SHCLFSOBJINFO *PCSHCLFSOBJINFO;

#pragma pack(1)
/**
 * Structure for keeping object open/create parameters.
 */
typedef struct _SHCLOBJOPENCREATEPARMS
{
    /** Path to object to open / create. */
    char                       *pszPath;
    /** Size (in bytes) of path to to object. */
    uint32_t                    cbPath;
    /** SHCL_OBJ_CF_* */
    uint32_t                    fCreate;
    /**
     * Attributes of object to open/create and
     * returned actual attributes of opened/created object.
     */
    SHCLFSOBJINFO    ObjInfo;
} SHCLOBJOPENCREATEPARMS, *PSHCLOBJOPENCREATEPARMS;
#pragma pack()

/**
 * Structure for keeping a reply message.
 */
typedef struct _SHCLREPLY
{
    /** Message type of type VBOX_SHCL_REPLYMSGTYPE_XXX. */
    uint32_t uType;
    /** IPRT result of overall operation. Note: int vs. uint32! */
    uint32_t rc;
    union
    {
        struct
        {
            SHCLLISTHANDLE uHandle;
        } ListOpen;
        struct
        {
            SHCLOBJHANDLE uHandle;
        } ObjOpen;
        struct
        {
            SHCLOBJHANDLE uHandle;
        } ObjClose;
    } u;
    /** Pointer to optional payload. */
    void    *pvPayload;
    /** Payload size (in bytes). */
    uint32_t cbPayload;
} SHCLREPLY, *PSHCLREPLY;

struct _SHCLLISTENTRY;
typedef _SHCLLISTENTRY SHCLLISTENTRY;

/** Defines a single root list entry. Currently the same as a regular list entry. */
typedef SHCLLISTENTRY SHCLROOTLISTENTRY;
/** Defines a pointer to a single root list entry. Currently the same as a regular list entry pointer. */
typedef SHCLROOTLISTENTRY *PSHCLROOTLISTENTRY;

/**
 * Structure for keeping Shared Clipboard root list headers.
 */
typedef struct _SHCLROOTLISTHDR
{
    /** Roots listing flags; unused at the moment. */
    uint32_t                fRoots;
    /** Number of root list entries. */
    uint32_t                cRoots;
} SHCLROOTLISTHDR, *PSHCLROOTLISTHDR;

/**
 * Structure for maintaining a Shared Clipboard root list.
 */
typedef struct _SHCLROOTLIST
{
    /** Root list header. */
    SHCLROOTLISTHDR    Hdr;
    /** Root list entries. */
    SHCLROOTLISTENTRY *paEntries;
} SHCLROOTLIST, *PSHCLROOTLIST;

/**
 * Structure for maintaining Shared Clipboard list open paramters.
 */
typedef struct _SHCLLISTOPENPARMS
{
    /** Listing flags (see VBOX_SHCL_LIST_FLAG_XXX). */
    uint32_t fList;
    /** Size (in bytes) of the filter string. */
    uint32_t cbFilter;
    /** Filter string. DOS wilcard-style. */
    char    *pszFilter;
    /** Size (in bytes) of the listing path. */
    uint32_t cbPath;
    /** Listing path (absolute). If empty or NULL the listing's root path will be opened. */
    char    *pszPath;
} SHCLLISTOPENPARMS, *PSHCLLISTOPENPARMS;

/**
 * Structure for keeping a Shared Clipboard list header.
 */
typedef struct _SHCLLISTHDR
{
    /** Feature flag(s). Not being used atm. */
    uint32_t fFeatures;
    /** Total objects returned. */
    uint64_t cTotalObjects;
    /** Total size (in bytes) returned. */
    uint64_t cbTotalSize;
} SHCLLISTHDR, *PSHCLLISTHDR;

/**
 * Structure for a Shared Clipboard list entry.
 */
typedef struct _SHCLLISTENTRY
{
    /** Entry name. */
    char    *pszName;
    /** Size (in bytes) of entry name. */
    uint32_t cbName;
    /** Information flag(s). */
    uint32_t fInfo;
    /** Size (in bytes) of the actual list entry. */
    uint32_t cbInfo;
    /** Data of the actual list entry. */
    void    *pvInfo;
} SHCLLISTENTRY, *PSHCLLISTENTRY;

/** Maximum length (in UTF-8 characters) of a list entry name. */
#define SHCLLISTENTRY_MAX_NAME     RTPATH_MAX /** @todo Improve this to be more dynamic. */

/**
 * Structure for maintaining a Shared Clipboard list.
 */
typedef struct _SHCLLIST
{
    /** List header. */
    SHCLLISTHDR        Hdr;
    /** List entries. */
    SHCLROOTLISTENTRY *paEntries;
} SHCLLIST, *PSHCLLIST;

/**
 * Structure for keeping a Shared Clipboard object data chunk.
 */
typedef struct _SHCLOBJDATACHUNK
{
    /** Handle of object this data chunk is related to. */
    uint64_t  uHandle;
    /** Pointer to actual data chunk. */
    void     *pvData;
    /** Size (in bytes) of data chunk. */
    uint32_t  cbData;
} SHCLOBJDATACHUNK, *PSHCLOBJDATACHUNK;

/**
 * Enumeration for specifying a clipboard area object type.
 */
typedef enum _SHCLAREAOBJTYPE
{
    /** Unknown object type; do not use. */
    SHCLAREAOBJTYPE_UNKNOWN = 0,
    /** Object is a directory. */
    SHCLAREAOBJTYPE_DIR,
    /** Object is a file. */
    SHCLAREAOBJTYPE_FILE,
    /** Object is a symbolic link. */
    SHCLAREAOBJTYPE_SYMLINK,
    /** The usual 32-bit hack. */
    SHCLAREAOBJTYPE_32Bit_Hack = 0x7fffffff
} SHCLAREAOBJTYPE;

/** Clipboard area ID. A valid area is >= 1.
 *  If 0 is specified, the last (most recent) area is meant.
 *  Set to UINT32_MAX if not initialized. */
typedef uint32_t SHCLAREAID;

/** Defines a non-initialized (nil) clipboard area. */
#define NIL_SHCLAREAID       UINT32_MAX

/** SharedClipboardArea open flags. */
typedef uint32_t SHCLAREAOPENFLAGS;

/** No clipboard area open flags specified. */
#define SHCLAREA_OPEN_FLAGS_NONE               0
/** The clipboard area must not exist yet. */
#define SHCLAREA_OPEN_FLAGS_MUST_NOT_EXIST     RT_BIT(0)
/** Mask of all valid clipboard area open flags.  */
#define SHCLAREA_OPEN_FLAGS_VALID_MASK         0x1

/** Defines a clipboard area object state. */
typedef uint32_t SHCLAREAOBJSTATE;

/** No object state set. */
#define SHCLAREAOBJSTATE_NONE                0
/** The object is considered as being complete (e.g. serialized). */
#define SHCLAREAOBJSTATE_COMPLETE            RT_BIT(0)

/**
 * Lightweight structure to keep a clipboard area object's state.
 *
 * Note: We don't want to use the ClipboardURIObject class here, as this
 *       is too heavy for this purpose.
 */
typedef struct _SHCLAREAOBJ
{
    SHCLAREAOBJTYPE  enmType;
    SHCLAREAOBJSTATE fState;
} SHCLAREAOBJ, *PSHCLAREAOBJ;

/**
 * Class for maintaining a Shared Clipboard area
 * on the host or guest. This will contain all received files & directories
 * for a single Shared Clipboard operation.
 *
 * In case of a failed Shared Clipboard operation this class can also
 * perform a gentle rollback if required.
 */
class SharedClipboardArea
{
public:

    SharedClipboardArea(void);
    SharedClipboardArea(const char *pszPath, SHCLAREAID uID = NIL_SHCLAREAID,
                        SHCLAREAOPENFLAGS fFlags = SHCLAREA_OPEN_FLAGS_NONE);
    virtual ~SharedClipboardArea(void);

public:

    uint32_t AddRef(void);
    uint32_t Release(void);

    int Lock(void);
    int Unlock(void);

    int AddObject(const char *pszPath, const SHCLAREAOBJ &Obj);
    int GetObject(const char *pszPath, PSHCLAREAOBJ pObj);

    int Close(void);
    bool IsOpen(void) const;
    int OpenEx(const char *pszPath, SHCLAREAID uID = NIL_SHCLAREAID,
               SHCLAREAOPENFLAGS fFlags = SHCLAREA_OPEN_FLAGS_NONE);
    int OpenTemp(SHCLAREAID uID = NIL_SHCLAREAID,
                 SHCLAREAOPENFLAGS fFlags = SHCLAREA_OPEN_FLAGS_NONE);
    SHCLAREAID GetID(void) const;
    const char *GetDirAbs(void) const;
    uint32_t GetRefCount(void);
    int Reopen(void);
    int Reset(bool fDeleteContent);
    int Rollback(void);

public:

    static int PathConstruct(const char *pszBase, SHCLAREAID uID, char *pszPath, size_t cbPath);

protected:

    int initInternal(void);
    int destroyInternal(void);
    int closeInternal(void);

protected:

    typedef std::map<RTCString, SHCLAREAOBJ> SharedClipboardAreaFsObjMap;

    /** Creation timestamp (in ms). */
    uint64_t                     m_tsCreatedMs;
    /** Number of references to this instance. */
    volatile uint32_t            m_cRefs;
    /** Critical section for serializing access. */
    RTCRITSECT                   m_CritSect;
    /** Open flags. */
    uint32_t                     m_fOpen;
    /** Directory handle for root clipboard directory. */
    RTDIR                        m_hDir;
    /** Absolute path to root clipboard directory. */
    RTCString                    m_strPathAbs;
    /** List for holding created directories in the case of a rollback. */
    SharedClipboardAreaFsObjMap  m_mapObj;
    /** Associated clipboard area ID. */
    SHCLAREAID        m_uID;
};

int SharedClipboardPathSanitizeFilename(char *pszPath, size_t cbPath);
int SharedClipboardPathSanitize(char *pszPath, size_t cbPath);

PSHCLROOTLIST SharedClipboardURIRootListAlloc(void);
void SharedClipboardURIRootListFree(PSHCLROOTLIST pRootList);

PSHCLROOTLISTHDR SharedClipboardURIRootListHdrDup(PSHCLROOTLISTHDR pRoots);
int SharedClipboardURIRootListHdrInit(PSHCLROOTLISTHDR pRoots);
void SharedClipboardURIRootListHdrDestroy(PSHCLROOTLISTHDR pRoots);

int SharedClipboardURIRootListEntryCopy(PSHCLROOTLISTENTRY pDst, PSHCLROOTLISTENTRY pSrc);
PSHCLROOTLISTENTRY SharedClipboardURIRootListEntryDup(PSHCLROOTLISTENTRY pRootListEntry);
void SharedClipboardURIRootListEntryDestroy(PSHCLROOTLISTENTRY pRootListEntry);

int SharedClipboardURIListHdrAlloc(PSHCLLISTHDR *ppListHdr);
void SharedClipboardURIListHdrFree(PSHCLLISTHDR pListHdr);
PSHCLLISTHDR SharedClipboardURIListHdrDup(PSHCLLISTHDR pListHdr);
int SharedClipboardURIListHdrInit(PSHCLLISTHDR pListHdr);
void SharedClipboardURIListHdrDestroy(PSHCLLISTHDR pListHdr);
void SharedClipboardURIListHdrFree(PSHCLLISTHDR pListHdr);
void SharedClipboardURIListHdrReset(PSHCLLISTHDR pListHdr);
bool SharedClipboardURIListHdrIsValid(PSHCLLISTHDR pListHdr);

int SharedClipboardURIListOpenParmsCopy(PSHCLLISTOPENPARMS pDst, PSHCLLISTOPENPARMS pSrc);
PSHCLLISTOPENPARMS SharedClipboardURIListOpenParmsDup(PSHCLLISTOPENPARMS pParms);
int SharedClipboardURIListOpenParmsInit(PSHCLLISTOPENPARMS pParms);
void SharedClipboardURIListOpenParmsDestroy(PSHCLLISTOPENPARMS pParms);

int SharedClipboardURIListEntryAlloc(PSHCLLISTENTRY *ppListEntry);
void SharedClipboardURIListEntryFree(PSHCLLISTENTRY pListEntry);
int SharedClipboardURIListEntryCopy(PSHCLLISTENTRY pDst, PSHCLLISTENTRY pSrc);
PSHCLLISTENTRY SharedClipboardURIListEntryDup(PSHCLLISTENTRY pListEntry);
int SharedClipboardURIListEntryInit(PSHCLLISTENTRY pListEntry);
void SharedClipboardURIListEntryDestroy(PSHCLLISTENTRY pListEntry);
bool SharedClipboardURIListEntryIsValid(PSHCLLISTENTRY pListEntry);

/**
 * Enumeration specifying an URI transfer direction.
 */
typedef enum _SHCLURITRANSFERDIR
{
    /** Unknown transfer directory. */
    SHCLURITRANSFERDIR_UNKNOWN = 0,
    /** Read transfer (from source). */
    SHCLURITRANSFERDIR_READ,
    /** Write transfer (to target). */
    SHCLURITRANSFERDIR_WRITE,
    /** The usual 32-bit hack. */
    SHCLURITRANSFERDIR_32BIT_HACK = 0x7fffffff
} SHCLURITRANSFERDIR;

struct _SHCLURITRANSFER;
typedef struct _SHCLURITRANSFER SHCLURITRANSFER;

/**
 * Structure for handling a single URI object context.
 */
typedef struct _SHCLCLIENTURIOBJCTX
{
    SHCLURITRANSFER *pTransfer;
    SHCLOBJHANDLE    uHandle;
} SHCLCLIENTURIOBJCTX, *PSHCLCLIENTURIOBJCTX;

typedef struct _SHCLURITRANSFEROBJSTATE
{
    uint64_t                    cbProcessed;
} SHCLURITRANSFEROBJSTATE, *PSHCLURITRANSFEROBJSTATE;

typedef struct _SHCLURITRANSFEROBJ
{
    SHCLOBJHANDLE           uHandle;
    char                              *pszPathAbs;
    SHCLFSOBJINFO           objInfo;
    SHCLSOURCE              enmSource;
    SHCLURITRANSFEROBJSTATE State;
} SHCLURITRANSFEROBJ, *PSHCLURITRANSFEROBJ;

/** No status set. */
#define SHCLURITRANSFERSTATUS_NONE           0
/** The transfer has been announced but is not running yet. */
#define SHCLURITRANSFERSTATUS_READY          1
/** The transfer is active and running. */
#define SHCLURITRANSFERSTATUS_RUNNING        2
/** The transfer has been completed. */
#define SHCLURITRANSFERSTATUS_COMPLETED      3
/** The transfer has been canceled. */
#define SHCLURITRANSFERSTATUS_CANCELED       4
/** The transfer ran into an unrecoverable error. */
#define SHCLURITRANSFERSTATUS_ERROR          5

/** Defines a transfer status. */
typedef uint32_t SHCLURITRANSFERSTATUS;

/**
 * Enumeration for specifying a Shared Clipboard object type.
 */
typedef enum _SHCLURIOBJTYPE
{
    /** Invalid object type. */
    SHCLURIOBJTYPE_INVALID = 0,
    /** Object is a directory. */
    SHCLURIOBJTYPE_DIRECTORY,
    /** Object is a file. */
    SHCLURIOBJTYPE_FILE,
    /** Object is a symbolic link. */
    SHCLURIOBJTYPE_SYMLINK,
    /** The usual 32-bit hack. */
    SHCLURIOBJTYPE_32BIT_SIZE_HACK = 0x7fffffff
} SHCLURIOBJTYPE;

/**
 * Structure for keeping URI list handle information.
 * This is using to map own (local) handles to the underlying file system.
 */
typedef struct _SHCLURILISTHANDLEINFO
{
    /** The list node. */
    RTLISTNODE                    Node;
    /** The list's handle. */
    SHCLLISTHANDLE     hList;
    /** Type of list handle. */
    SHCLURIOBJTYPE     enmType;
    /** Absolute local path of the list object. */
    char                         *pszPathLocalAbs;
    union
    {
        /** Local data, based on enmType. */
        struct
        {
            union
            {
                RTDIR  hDir;
                RTFILE hFile;
            };
        } Local;
    } u;
} SHCLURILISTHANDLEINFO, *PSHCLURILISTHANDLEINFO;

/**
 * Structure for keeping URI object handle information.
 * This is using to map own (local) handles to the underlying file system.
 */
typedef struct _SHCLURIOBJHANDLEINFO
{
    /** The list node. */
    RTLISTNODE                Node;
    /** The object's handle. */
    SHCLOBJHANDLE  hObj;
    /** Type of object handle. */
    SHCLURIOBJTYPE enmType;
    /** Absolute local path of the object. */
    char                     *pszPathLocalAbs;
    union
    {
        /** Local data, based on enmType. */
        struct
        {
            union
            {
                RTDIR  hDir;
                RTFILE hFile;
            };
        } Local;
    } u;
} SHCLURIOBJHANDLEINFO, *PSHCLURIOBJHANDLEINFO;

/**
 * Structure for keeping a single root list entry.
 */
typedef struct _SHCLURILISTROOT
{
    /** The list node. */
    RTLISTNODE          Node;
    /** Absolute path of entry. */
    char               *pszPathAbs;
} SHCLURILISTROOT, *PSHCLURILISTROOT;

/**
 * Structure for maintaining an URI transfer state.
 * Everything in here will be part of a saved state (later).
 */
typedef struct _SHCLURITRANSFERSTATE
{
    /** The transfer's (local) ID. */
    uint16_t                            uID;
    /** The transfer's current status. */
    SHCLURITRANSFERSTATUS    enmStatus;
    /** The transfer's direction. */
    SHCLURITRANSFERDIR       enmDir;
    /** The transfer's source. */
    SHCLSOURCE               enmSource;
} SHCLURITRANSFERSTATE, *PSHCLURITRANSFERSTATE;

struct _SHCLURITRANSFER;
typedef struct _SHCLURITRANSFER *PSHCLURITRANSFER;

/**
 * Structure maintaining URI clipboard provider context data.
 * This is handed in to the provider implementation callbacks.
 */
    typedef struct _SHCLPROVIDERCTX
{
    /** Pointer to the related URI transfer. */
    PSHCLURITRANSFER pTransfer;
    /** User-defined data pointer. Can be NULL if not needed. */
    void                       *pvUser;
} SHCLPROVIDERCTX, *PSHCLPROVIDERCTX;

/** Defines an URI clipboard provider function declaration with additional parameters. */
#define SHCLPROVIDERFUNCDECL(a_Name, ...) \
    typedef DECLCALLBACK(int) RT_CONCAT(FNSHCLPROVIDER, a_Name)(PSHCLPROVIDERCTX, __VA_ARGS__); \
    typedef RT_CONCAT(FNSHCLPROVIDER, a_Name) RT_CONCAT(*PFNSHCLPROVIDER, a_Name);

/** Defines an URI clipboard provider function declaration with additional parameters. */
#define SHCLPROVIDERFUNCDECLRET(a_Ret, a_Name, ...) \
    typedef DECLCALLBACK(a_Ret) RT_CONCAT(FNSHCLPROVIDER, a_Name)(PSHCLPROVIDERCTX, __VA_ARGS__); \
    typedef RT_CONCAT(FNSHCLPROVIDER, a_Name) RT_CONCAT(*PFNSHCLPROVIDER, a_Name);

/** Defines an URI clipboard provider function declaration (no additional parameters). */
#define SHCLPROVIDERFUNCDECLVOID(a_Name) \
    typedef DECLCALLBACK(int) RT_CONCAT(FNSHCLPROVIDER, a_Name)(PSHCLPROVIDERCTX); \
    typedef RT_CONCAT(FNSHCLPROVIDER, a_Name) RT_CONCAT(*PFNSHCLPROVIDER, a_Name);

/** Declares a URI clipboard provider function member. */
#define SHCLPROVIDERFUNCMEMBER(a_Name, a_Member) \
    RT_CONCAT(PFNSHCLPROVIDER, a_Name) a_Member;

SHCLPROVIDERFUNCDECLVOID(TRANSFEROPEN)
SHCLPROVIDERFUNCDECLVOID(TRANSFERCLOSE)
SHCLPROVIDERFUNCDECL(GETROOTS, PSHCLROOTLIST *ppRootList)
SHCLPROVIDERFUNCDECL(LISTOPEN, PSHCLLISTOPENPARMS pOpenParms, PSHCLLISTHANDLE phList)
SHCLPROVIDERFUNCDECL(LISTCLOSE, SHCLLISTHANDLE hList)
SHCLPROVIDERFUNCDECL(LISTHDRREAD, SHCLLISTHANDLE hList, PSHCLLISTHDR pListHdr)
SHCLPROVIDERFUNCDECL(LISTHDRWRITE, SHCLLISTHANDLE hList, PSHCLLISTHDR pListHdr)
SHCLPROVIDERFUNCDECL(LISTENTRYREAD, SHCLLISTHANDLE hList, PSHCLLISTENTRY pEntry)
SHCLPROVIDERFUNCDECL(LISTENTRYWRITE, SHCLLISTHANDLE hList, PSHCLLISTENTRY pEntry)
SHCLPROVIDERFUNCDECL(OBJOPEN, PSHCLOBJOPENCREATEPARMS pCreateParms, PSHCLOBJHANDLE phObj)
SHCLPROVIDERFUNCDECL(OBJCLOSE, SHCLOBJHANDLE hObj)
SHCLPROVIDERFUNCDECL(OBJREAD, SHCLOBJHANDLE hObj, void *pvData, uint32_t cbData, uint32_t fFlags, uint32_t *pcbRead)
SHCLPROVIDERFUNCDECL(OBJWRITE, SHCLOBJHANDLE hObj, void *pvData, uint32_t cbData, uint32_t fFlags, uint32_t *pcbWritten)

/**
 * Shared Clipboard URI provider interface table.
 */
typedef struct _SHCLPROVIDERINTERFACE
{
    SHCLPROVIDERFUNCMEMBER(TRANSFEROPEN, pfnTransferOpen)
    SHCLPROVIDERFUNCMEMBER(TRANSFERCLOSE, pfnTransferClose)
    SHCLPROVIDERFUNCMEMBER(GETROOTS, pfnGetRoots)
    SHCLPROVIDERFUNCMEMBER(LISTOPEN, pfnListOpen)
    SHCLPROVIDERFUNCMEMBER(LISTCLOSE, pfnListClose)
    SHCLPROVIDERFUNCMEMBER(LISTHDRREAD, pfnListHdrRead)
    SHCLPROVIDERFUNCMEMBER(LISTHDRWRITE, pfnListHdrWrite)
    SHCLPROVIDERFUNCMEMBER(LISTENTRYREAD, pfnListEntryRead)
    SHCLPROVIDERFUNCMEMBER(LISTENTRYWRITE, pfnListEntryWrite)
    SHCLPROVIDERFUNCMEMBER(OBJOPEN, pfnObjOpen)
    SHCLPROVIDERFUNCMEMBER(OBJCLOSE, pfnObjClose)
    SHCLPROVIDERFUNCMEMBER(OBJREAD, pfnObjRead)
    SHCLPROVIDERFUNCMEMBER(OBJWRITE, pfnObjWrite)
} SHCLPROVIDERINTERFACE, *PSHCLPROVIDERINTERFACE;

/**
 * Structure for the Shared Clipboard provider creation context.
 */
typedef struct _SHCLPROVIDERCREATIONCTX
{
    /** Specifies what the source of the provider is. */
    SHCLSOURCE             enmSource;
    /** The provider interface table. */
    SHCLPROVIDERINTERFACE  Interface;
    /** Provider callback data. */
    void                             *pvUser;
} SHCLPROVIDERCREATIONCTX, *PSHCLPROVIDERCREATIONCTX;

struct _SHCLURITRANSFER;
typedef _SHCLURITRANSFER *PSHCLURITRANSFER;

/**
 * Structure for storing URI transfer callback data.
 */
typedef struct _SHCLURITRANSFERCALLBACKDATA
{
    /** Pointer to related URI transfer. */
    PSHCLURITRANSFER pTransfer;
    /** Saved user pointer. */
    void                       *pvUser;
} SHCLURITRANSFERCALLBACKDATA, *PSHCLURITRANSFERCALLBACKDATA;

#define SHCLTRANSFERCALLBACKDECLVOID(a_Name) \
    typedef DECLCALLBACK(void) RT_CONCAT(FNSHCLCALLBACK, a_Name)(PSHCLURITRANSFERCALLBACKDATA pData); \
    typedef RT_CONCAT(FNSHCLCALLBACK, a_Name) RT_CONCAT(*PFNSHCLCALLBACK, a_Name);

#define SHCLTRANSFERCALLBACKDECL(a_Name, ...) \
    typedef DECLCALLBACK(void) RT_CONCAT(FNSHCLCALLBACK, a_Name)(PSHCLURITRANSFERCALLBACKDATA pData, __VA_ARGS__); \
    typedef RT_CONCAT(FNSHCLCALLBACK, a_Name) RT_CONCAT(*PFNSHCLCALLBACK, a_Name);

#define SHCLTRANSFERCALLBACKMEMBER(a_Name, a_Member) \
    RT_CONCAT(PFNSHCLCALLBACK, a_Name) a_Member;

SHCLTRANSFERCALLBACKDECLVOID(TRANSFERPREPARE)
SHCLTRANSFERCALLBACKDECLVOID(TRANSFERSTARTED)
SHCLTRANSFERCALLBACKDECLVOID(LISTHEADERCOMPLETE)
SHCLTRANSFERCALLBACKDECLVOID(LISTENTRYCOMPLETE)
SHCLTRANSFERCALLBACKDECL    (TRANSFERCOMPLETE, int rc)
SHCLTRANSFERCALLBACKDECLVOID(TRANSFERCANCELED)
SHCLTRANSFERCALLBACKDECL    (TRANSFERERROR, int rc)

/**
 * Structure acting as a function callback table for URI transfers.
 * All callbacks are optional and therefore can be NULL.
 */
typedef struct _SHCLURITRANSFERCALLBACKS
{
    /** Saved user pointer. */
    void                                  *pvUser;
    /** Function pointer, called when the transfer is going to be prepared. */
    SHCLTRANSFERCALLBACKMEMBER(TRANSFERPREPARE, pfnTransferPrepare)
    /** Function pointer, called when the transfer has been started. */
    SHCLTRANSFERCALLBACKMEMBER(TRANSFERSTARTED, pfnTransferStarted)
    /** Function pointer, called when reading / writing the list header is complete. */
    SHCLTRANSFERCALLBACKMEMBER(LISTHEADERCOMPLETE, pfnListHeaderComplete)
    /** Function pointer, called when reading / writing a list entry is complete. */
    SHCLTRANSFERCALLBACKMEMBER(LISTENTRYCOMPLETE, pfnListEntryComplete)
    /** Function pointer, called when the transfer is complete. */
    SHCLTRANSFERCALLBACKMEMBER(TRANSFERCOMPLETE, pfnTransferComplete)
    /** Function pointer, called when the transfer has been canceled. */
    SHCLTRANSFERCALLBACKMEMBER(TRANSFERCANCELED, pfnTransferCanceled)
    /** Function pointer, called when transfer resulted in an unrecoverable error. */
    SHCLTRANSFERCALLBACKMEMBER(TRANSFERERROR, pfnTransferError)
} SHCLURITRANSFERCALLBACKS, *PSHCLURITRANSFERCALLBACKS;

/**
 * Structure for thread-related members for a single URI transfer.
 */
typedef struct _SHCLURITRANSFERTHREAD
{
    /** Thread handle for the reading / writing thread.
     *  Can be NIL_RTTHREAD if not being used. */
    RTTHREAD                    hThread;
    /** Thread started indicator. */
    volatile bool               fStarted;
    /** Thread stop flag. */
    volatile bool               fStop;
    /** Thread cancelled flag / indicator. */
    volatile bool               fCancelled;
} SHCLURITRANSFERTHREAD, *PSHCLURITRANSFERTHREAD;

/**
 * Structure for maintaining a single URI transfer.
 *
 ** @todo Not yet thread safe.
 */
typedef struct _SHCLURITRANSFER
{
    /** The node member for using this struct in a RTList. */
    RTLISTNODE                          Node;
    /** Critical section for serializing access. */
    RTCRITSECT                          CritSect;
    /** The transfer's state (for SSM, later). */
    SHCLURITRANSFERSTATE     State;
    /** Timeout (in ms) for waiting of events. Default is 30s. */
    RTMSINTERVAL                        uTimeoutMs;
    /** Absolute path to root entries. */
    char                               *pszPathRootAbs;
    /** Maximum data chunk size (in bytes) to transfer. Default is 64K. */
    uint32_t                            cbMaxChunkSize;
    /** The transfer's own event source. */
    SHCLEVENTSOURCE          Events;
    /** Next upcoming list handle. */
    SHCLLISTHANDLE           uListHandleNext;
    /** List of all list handles elated to this transfer. */
    RTLISTANCHOR                        lstList;
    /** Number of root entries in list. */
    uint64_t                            cRoots;
    /** List of root entries of this transfer. */
    RTLISTANCHOR                        lstRoots;
    /** Next upcoming object handle. */
    SHCLOBJHANDLE            uObjHandleNext;
    /** Map of all objects handles related to this transfer. */
    RTLISTANCHOR                        lstObj;
    /** The transfer's own (local) area, if any (can be NULL if not needed).
     *  The area itself has a clipboard area ID assigned.
     *  On the host this area ID gets shared (maintained / locked) across all VMs via VBoxSVC. */
    SharedClipboardArea                *pArea;
    SHCLPROVIDERCTX          ProviderCtx;
    /** The transfer's provider interface. */
    SHCLPROVIDERINTERFACE    ProviderIface;
    /** The transfer's (optional) callback table. */
    SHCLURITRANSFERCALLBACKS Callbacks;
    /** Opaque pointer to implementation-specific parameters. */
    void                               *pvUser;
    /** Size (in bytes) of implementation-specific parameters. */
    size_t                              cbUser;
    /** Contains thread-related attributes. */
    SHCLURITRANSFERTHREAD    Thread;
} SHCLURITRANSFER, *PSHCLURITRANSFER;

/**
 * Structure for keeping URI clipboard information around.
 */
typedef struct _SHCLURICTX
{
    /** Critical section for serializing access. */
    RTCRITSECT                  CritSect;
    /** List of transfers. */
    RTLISTANCHOR                List;
    /** Number of running (concurrent) transfers.
     *  At the moment we only support only one transfer per client at a time. */
    uint32_t                    cRunning;
    /** Maximum Number of running (concurrent) transfers.
     *  At the moment we only support only one transfer per client at a time. */
    uint32_t                    cMaxRunning;
    /** Number of total transfers (in list). */
    uint32_t                    cTransfers;
} SHCLURICTX, *PSHCLURICTX;

int SharedClipboardURIObjCtxInit(PSHCLCLIENTURIOBJCTX pObjCtx);
void SharedClipboardURIObjCtxDestroy(PSHCLCLIENTURIOBJCTX pObjCtx);
bool SharedClipboardURIObjCtxIsValid(PSHCLCLIENTURIOBJCTX pObjCtx);

int SharedClipboardURIObjectOpenParmsInit(PSHCLOBJOPENCREATEPARMS pParms);
int SharedClipboardURIObjectOpenParmsCopy(PSHCLOBJOPENCREATEPARMS pParmsDst, PSHCLOBJOPENCREATEPARMS pParmsSrc);
void SharedClipboardURIObjectOpenParmsDestroy(PSHCLOBJOPENCREATEPARMS pParms);

int SharedClipboardURIObjectOpen(PSHCLURITRANSFER pTransfer, PSHCLOBJOPENCREATEPARMS pOpenCreateParms,
                                 PSHCLOBJHANDLE phObj);
int SharedClipboardURIObjectClose(PSHCLURITRANSFER pTransfer, SHCLOBJHANDLE hObj);
int SharedClipboardURIObjectRead(PSHCLURITRANSFER pTransfer, SHCLOBJHANDLE hObj, void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead, uint32_t fFlags);
int SharedClipboardURIObjectWrite(PSHCLURITRANSFER pTransfer, SHCLOBJHANDLE hObj, void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten, uint32_t fFlags);

PSHCLOBJDATACHUNK SharedClipboardURIObjectDataChunkDup(PSHCLOBJDATACHUNK pDataChunk);
void SharedClipboardURIObjectDataChunkDestroy(PSHCLOBJDATACHUNK pDataChunk);
void SharedClipboardURIObjectDataChunkFree(PSHCLOBJDATACHUNK pDataChunk);

int SharedClipboardURITransferCreate(SHCLURITRANSFERDIR enmDir, SHCLSOURCE enmSource,
                                     PSHCLURITRANSFER *ppTransfer);
int SharedClipboardURITransferDestroy(PSHCLURITRANSFER pTransfer);

int SharedClipboardURITransferOpen(PSHCLURITRANSFER pTransfer);
int SharedClipboardURITransferClose(PSHCLURITRANSFER pTransfer);

int SharedClipboardURITransferListOpen(PSHCLURITRANSFER pTransfer, PSHCLLISTOPENPARMS pOpenParms,
                                       PSHCLLISTHANDLE phList);
int SharedClipboardURITransferListClose(PSHCLURITRANSFER pTransfer, SHCLLISTHANDLE hList);
int SharedClipboardURITransferListGetHeader(PSHCLURITRANSFER pTransfer, SHCLLISTHANDLE hList,
                                            PSHCLLISTHDR pHdr);
PSHCLURITRANSFEROBJ SharedClipboardURITransferListGetObj(PSHCLURITRANSFER pTransfer,
                                                                    SHCLLISTHANDLE hList, uint64_t uIdx);
int SharedClipboardURITransferListRead(PSHCLURITRANSFER pTransfer, SHCLLISTHANDLE hList,
                                       PSHCLLISTENTRY pEntry);
int SharedClipboardURITransferListWrite(PSHCLURITRANSFER pTransfer, SHCLLISTHANDLE hList,
                                        PSHCLLISTENTRY pEntry);
bool SharedClipboardURITransferListHandleIsValid(PSHCLURITRANSFER pTransfer, SHCLLISTHANDLE hList);

int SharedClipboardURITransferPrepare(PSHCLURITRANSFER pTransfer);
int SharedClipboardURITransferSetInterface(PSHCLURITRANSFER pTransfer,
                                           PSHCLPROVIDERCREATIONCTX pCreationCtx);
int SharedClipboardURILTransferSetRoots(PSHCLURITRANSFER pTransfer, const char *pszRoots, size_t cbRoots);
void SharedClipboardURITransferReset(PSHCLURITRANSFER pTransfer);
SharedClipboardArea *SharedClipboardURITransferGetArea(PSHCLURITRANSFER pTransfer);

uint32_t SharedClipboardURILTransferRootsCount(PSHCLURITRANSFER pTransfer);
int SharedClipboardURILTransferRootsEntry(PSHCLURITRANSFER pTransfer, uint64_t uIndex, PSHCLROOTLISTENTRY pEntry);
int SharedClipboardURILTransferRootsAsList(PSHCLURITRANSFER pTransfer, PSHCLROOTLIST *ppRootList);

SHCLSOURCE SharedClipboardURITransferGetSource(PSHCLURITRANSFER pTransfer);
SHCLURITRANSFERSTATUS SharedClipboardURITransferGetStatus(PSHCLURITRANSFER pTransfer);
int SharedClipboardURITransferHandleReply(PSHCLURITRANSFER pTransfer, PSHCLREPLY pReply);
int SharedClipboardURITransferRun(PSHCLURITRANSFER pTransfer, PFNRTTHREAD pfnThreadFunc, void *pvUser);
void SharedClipboardURITransferSetCallbacks(PSHCLURITRANSFER pTransfer,
                                            PSHCLURITRANSFERCALLBACKS pCallbacks);

int SharedClipboardURITransferRead(PSHCLURITRANSFER pTransfer);
int SharedClipboardURITransferReadObjects(PSHCLURITRANSFER pTransfer);

int SharedClipboardURITransferWrite(PSHCLURITRANSFER pTransfer);
int SharedClipboardURITransferWriteObjects(PSHCLURITRANSFER pTransfer);

int SharedClipboardURICtxInit(PSHCLURICTX pURI);
void SharedClipboardURICtxDestroy(PSHCLURICTX pURI);
void SharedClipboardURICtxReset(PSHCLURICTX pURI);
PSHCLURITRANSFER SharedClipboardURICtxGetTransfer(PSHCLURICTX pURI, uint32_t uIdx);
uint32_t SharedClipboardURICtxGetRunningTransfers(PSHCLURICTX pURI);
uint32_t SharedClipboardURICtxGetTotalTransfers(PSHCLURICTX pURI);
void SharedClipboardURICtxTransfersCleanup(PSHCLURICTX pURI);
bool SharedClipboardURICtxTransfersMaximumReached(PSHCLURICTX pURI);
int SharedClipboardURICtxTransferAdd(PSHCLURICTX pURI, PSHCLURITRANSFER pTransfer);
int SharedClipboardURICtxTransferRemove(PSHCLURICTX pURI, PSHCLURITRANSFER pTransfer);

void SharedClipboardFsObjFromIPRT(PSHCLFSOBJINFO pDst, PCRTFSOBJINFO pSrc);

bool SharedClipboardMIMEHasFileURLs(const char *pcszFormat, size_t cchFormatMax);
bool SharedClipboardMIMENeedsCache(const char *pcszFormat, size_t cchFormatMax);

#endif /* !VBOX_INCLUDED_GuestHost_SharedClipboard_uri_h */

