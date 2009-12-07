/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * VBoxWarningPane class declaration
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef __VBoxWarningPane_h__
#define __VBoxWarningPane_h__

/* Global includes */
#include <QWidget>
#include <QLabel>

class VBoxWarningPane: public QWidget
{
    Q_OBJECT;

public:

    VBoxWarningPane(QWidget *pParent = 0);

    void setWarningPixmap(const QPixmap &imgPixmap);
    void setWarningText(const QString &strText);

private:

    QLabel m_icon;
    QLabel m_label;
};

#endif // __VBoxWarningPane_h__

