/* $Id$ */
/** @file
 * BS3Kit - Bs3SlabListAdd
 */

/*
 * Copyright (C) 2007-2022 Oracle and/or its affiliates.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL), a copy of it is provided in the "COPYING.CDDL" file included
 * in the VirtualBox distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * SPDX-License-Identifier: GPL-3.0-only OR CDDL-1.0
 */

#include "bs3kit-template-header.h"


#undef Bs3SlabListAdd
BS3_CMN_DEF(void, Bs3SlabListAdd,(PBS3SLABHEAD pHead, PBS3SLABCTL pSlabCtl))
{
    BS3_ASSERT(pHead->cbChunk == pSlabCtl->cbChunk);
    BS3_ASSERT(BS3_XPTR_IS_NULL(BS3SLABHEAD, pSlabCtl->pNext));

    BS3_XPTR_SET_FLAT(BS3SLABCTL, pSlabCtl->pNext, BS3_XPTR_GET_FLAT(BS3SLABCTL, pHead->pFirst));
    BS3_XPTR_SET(BS3SLABCTL, pHead->pFirst, pSlabCtl);

    pHead->cSlabs      += 1;
    pHead->cChunks     += pSlabCtl->cChunks;
    pHead->cFreeChunks += pSlabCtl->cFreeChunks;
}

