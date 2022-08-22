/* $Id$ */
/** @file
 * VBox Qt GUI - UIMachineWindowScale class declaration.
 */

/*
 * Copyright (C) 2010-2022 Oracle and/or its affiliates.
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

#ifndef FEQT_INCLUDED_SRC_runtime_scale_UIMachineWindowScale_h
#define FEQT_INCLUDED_SRC_runtime_scale_UIMachineWindowScale_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIMachineWindow.h"

/** UIMachineWindow reimplementation,
  * providing GUI with machine-window for the scale mode. */
class UIMachineWindowScale : public UIMachineWindow
{
    Q_OBJECT;

protected:

    /** Constructor, passes @a pMachineLogic and @a uScreenId to the UIMachineWindow constructor. */
    UIMachineWindowScale(UIMachineLogic *pMachineLogic, ulong uScreenId);

private:

    /** Prepare main-layout routine. */
    void prepareMainLayout();
    /** Prepare notification-center routine. */
    void prepareNotificationCenter();
#ifdef VBOX_WS_MAC
    /** Prepare visual-state routine. */
    void prepareVisualState();
#endif /* VBOX_WS_MAC */
    /** Load settings routine. */
    void loadSettings();

#ifdef VBOX_WS_MAC
    /** Cleanup visual-state routine. */
    void cleanupVisualState();
#endif /* VBOX_WS_MAC */
    /** Cleanup notification-center routine. */
    void cleanupNotificationCenter();

    /** Updates visibility according to visual-state. */
    void showInNecessaryMode();

    /** Restores cached window geometry. */
    virtual void restoreCachedGeometry() RT_OVERRIDE;

    /** Performs window geometry normalization according to guest-size and host's available geometry.
      * @param  fAdjustPosition        Determines whether is it necessary to adjust position as well.
      * @param  fResizeToGuestDisplay  Determines whether is it necessary to resize the window to fit to guest display size. */
    virtual void normalizeGeometry(bool fAdjustPosition, bool fResizeToGuestDisplay) RT_OVERRIDE;

    /** Common @a pEvent handler. */
    bool event(QEvent *pEvent);

    /** Returns whether this window is maximized. */
    bool isMaximizedChecked();

    /** Holds the current window geometry. */
    QRect  m_geometry;
    /** Holds the geometry save timer ID. */
    int  m_iGeometrySaveTimerId;

    /** Factory support. */
    friend class UIMachineWindow;
};

#endif /* !FEQT_INCLUDED_SRC_runtime_scale_UIMachineWindowScale_h */
