/* $Id$ */

/** @file
 * VBox WDDM Miniport driver
 */

/*
 * Copyright (C) 2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___VBoxMPCr_h__
#define ___VBoxMPCr_h__

#include <VBox/VBoxGuestLib.h>
#include <VBoxGuestR0LibCrOgl.h>

int VBoxMpCrCtlConConnect(struct _VBOXMP_DEVEXT *pDevExt,
        uint32_t crVersionMajor, uint32_t crVersionMinor,
        uint32_t *pu32ClientID);
int VBoxMpCrCtlConDisconnect(struct _VBOXMP_DEVEXT *pDevExt, uint32_t u32ClientID);
int VBoxMpCrCtlConCall(struct _VBOXMP_DEVEXT *pDevExt, struct VBoxGuestHGCMCallInfo *pData, uint32_t cbData);
int VBoxMpCrCtlConCallUserData(struct _VBOXMP_DEVEXT *pDevExt, struct VBoxGuestHGCMCallInfo *pData, uint32_t cbData);

#endif /* #ifndef ___VBoxMPCr_h__ */
