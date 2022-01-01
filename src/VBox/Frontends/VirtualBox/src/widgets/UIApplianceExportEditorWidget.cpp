/* $Id$ */
/** @file
 * VBox Qt GUI - UIApplianceExportEditorWidget class implementation.
 */

/*
 * Copyright (C) 2009-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QTextEdit>

/* GUI includes: */
#include "QITreeView.h"
#include "UIApplianceExportEditorWidget.h"
#include "UIMessageCenter.h"


/** UIApplianceSortProxyModel subclass for Export Appliance wizard. */
class ExportSortProxyModel : public UIApplianceSortProxyModel
{
public:

    /** Constructs proxy model passing @a pParent to the base-class. */
    ExportSortProxyModel(QObject *pParent = 0)
        : UIApplianceSortProxyModel(pParent)
    {
        m_aFilteredList
            << KVirtualSystemDescriptionType_OS
            << KVirtualSystemDescriptionType_CPU
            << KVirtualSystemDescriptionType_Memory
            << KVirtualSystemDescriptionType_Floppy
            << KVirtualSystemDescriptionType_CDROM
            << KVirtualSystemDescriptionType_USBController
            << KVirtualSystemDescriptionType_SoundCard
            << KVirtualSystemDescriptionType_NetworkAdapter
            << KVirtualSystemDescriptionType_HardDiskControllerIDE
            << KVirtualSystemDescriptionType_HardDiskControllerSATA
            << KVirtualSystemDescriptionType_HardDiskControllerSCSI
            << KVirtualSystemDescriptionType_HardDiskControllerSAS
            << KVirtualSystemDescriptionType_CloudProfileName;
    }
};


/*********************************************************************************************************************************
*   Class UIApplianceExportEditorWidget implementation.                                                                          *
*********************************************************************************************************************************/

UIApplianceExportEditorWidget::UIApplianceExportEditorWidget(QWidget *pParent /* = 0 */)
    : UIApplianceEditorWidget(pParent)
{
}

void UIApplianceExportEditorWidget::setAppliance(const CAppliance &comAppliance)
{
    /* Cleanup previous stuff: */
    clear();

    /* Call to base-class: */
    UIApplianceEditorWidget::setAppliance(comAppliance);

    /* Prepare model: */
    QVector<CVirtualSystemDescription> vsds = m_comAppliance.GetVirtualSystemDescriptions();
    m_pModel = new UIApplianceModel(vsds, m_pTreeViewSettings);
    if (m_pModel)
    {
        m_pModel->setVsdHints(m_listVsdHints);

        /* Create proxy model: */
        ExportSortProxyModel *pProxy = new ExportSortProxyModel(m_pModel);
        if (pProxy)
        {
            pProxy->setSourceModel(m_pModel);
            pProxy->sort(ApplianceViewSection_Description, Qt::DescendingOrder);

            /* Set our own model: */
            m_pTreeViewSettings->setModel(pProxy);
            /* Set our own delegate: */
            UIApplianceDelegate *pDelegate = new UIApplianceDelegate(pProxy);
            if (pDelegate)
                m_pTreeViewSettings->setItemDelegate(pDelegate);

            /* For now we hide the original column. This data is displayed as tooltip also. */
            m_pTreeViewSettings->setColumnHidden(ApplianceViewSection_OriginalValue, true);
            m_pTreeViewSettings->expandAll();
            /* Set model root index and make it current: */
            m_pTreeViewSettings->setRootIndex(pProxy->mapFromSource(m_pModel->root()));
            m_pTreeViewSettings->setCurrentIndex(pProxy->mapFromSource(m_pModel->root()));
        }
    }

    /* Check for warnings & if there are one display them: */
    const QVector<QString> warnings = m_comAppliance.GetWarnings();
    const bool fWarningsEnabled = warnings.size() > 0;
    foreach (const QString &strText, warnings)
        m_pTextEditWarning->append("- " + strText);
    m_pPaneWarning->setVisible(fWarningsEnabled);
}

void UIApplianceExportEditorWidget::prepareExport()
{
    if (m_comAppliance.isNotNull())
        m_pModel->putBack();
}
