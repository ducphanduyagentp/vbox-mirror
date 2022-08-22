/* $Id$ */
/** @file
 * IPRT - Initialization & Termination.
 */

/*
 * Copyright (C) 2006-2022 Oracle and/or its affiliates.
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

#ifndef IPRT_INCLUDED_INTERNAL_initterm_h
#define IPRT_INCLUDED_INTERNAL_initterm_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>

RT_C_DECLS_BEGIN

#ifdef IN_RING0

/**
 * Platform specific initialization.
 *
 * @returns IPRT status code.
 */
DECLHIDDEN(int)  rtR0InitNative(void);

/**
 * Platform specific termination.
 */
DECLHIDDEN(void) rtR0TermNative(void);

# ifdef RT_OS_LINUX
/* in alloc-r0drv0-linux.c */
DECLHIDDEN(void) rtR0MemExecCleanup(void);
# endif

#endif /* IN_RING0 */

RT_C_DECLS_END

#endif /* !IPRT_INCLUDED_INTERNAL_initterm_h */

