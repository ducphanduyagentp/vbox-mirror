/* $Id$ */
/** @file
 * VBox Qt GUI - UIVMLogViewerWidget class implementation.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
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
#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QFont>
#include <QMenu>
#include <QPalette>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStyle>
#include <QStylePainter>
#include <QStyleOptionTab>
#include <QTabBar>
#include <QTextBlock>
#include <QVBoxLayout>
#ifdef RT_OS_SOLARIS
# include <QFontDatabase>
#endif

/* GUI includes: */
#include "QIFileDialog.h"
#include "QITabWidget.h"
#include "UIActionPool.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UIMessageCenter.h"
#include "UIVirtualMachineItem.h"
#include "UIVMLogPage.h"
#include "UIVMLogViewerWidget.h"
#include "UIVMLogViewerBookmarksPanel.h"
#include "UIVMLogViewerFilterPanel.h"
#include "UIVMLogViewerSearchPanel.h"
#include "UIVMLogViewerOptionsPanel.h"
#include "QIToolBar.h"
#include "QIToolButton.h"
#include "UICommon.h"

/* COM includes: */
#include "CSystemProperties.h"

/** Limit the read string size to avoid bloated log viewer pages. */
const ULONG uAllowedLogSize = _256M;

/*********************************************************************************************************************************
*   UITabBar definition.                                                                                        *
*********************************************************************************************************************************/
/** A QTabBar extention to be able to override paintEvent for custom tab coloring. */
class UITabBar : public QTabBar
{

    Q_OBJECT;

public:

    UITabBar(QWidget *pParent = 0);
protected:

    virtual void paintEvent(QPaintEvent *pEvent) /* override */;

private:

    QVector<QColor> m_alternateColors;
};

/*********************************************************************************************************************************
*   UITabWidget definition.                                                                                        *
*********************************************************************************************************************************/

/** A QITabWidget used only for setTabBar since it is protected. */
class UITabWidget : public QITabWidget
{

    Q_OBJECT;

public:

    UITabWidget(QWidget *pParent = 0);
};

/*********************************************************************************************************************************
*   UITabBar implementation.                                                                                        *
*********************************************************************************************************************************/

UITabBar::UITabBar(QWidget *pParent /* = 0 */)
    :QTabBar(pParent)
{
    QStyleOptionTab opt;
    m_alternateColors << opt.palette.color(QPalette::Button).lighter(135);
    m_alternateColors << opt.palette.color(QPalette::Button).darker(135);
}

void UITabBar::paintEvent(QPaintEvent *pEvent)
{
    Q_UNUSED(pEvent);
    QStylePainter painter(this);
    QStyleOptionTab opt;

    for (int i = 0; i < count(); i++) {
        initStyleOption(&opt, i);
        int iColorIndex = tabData(i).toInt();
        if (iColorIndex >= 0 && iColorIndex <= m_alternateColors.size())
        {
            opt.palette.setColor(QPalette::Button, m_alternateColors[iColorIndex]);
        }

        painter.drawControl(QStyle::CE_TabBarTabShape, opt);
        painter.drawControl(QStyle::CE_TabBarTabLabel, opt);
    }
}

/*********************************************************************************************************************************
*   UITabWidget implementation.                                                                                        *
*********************************************************************************************************************************/

UITabWidget::UITabWidget(QWidget *pParent /* = 0 */)
    :QITabWidget(pParent)
{
    setTabBar(new UITabBar(this));
}


/*********************************************************************************************************************************
*   UIVMLogViewerWidget implementation.                                                                                          *
*********************************************************************************************************************************/

UIVMLogViewerWidget::UIVMLogViewerWidget(EmbedTo enmEmbedding,
                                         UIActionPool *pActionPool,
                                         bool fShowToolbar /* = true */,
                                         const CMachine &comMachine /* = CMachine() */,
                                         QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_pActionPool(pActionPool)
    , m_fShowToolbar(fShowToolbar)
    , m_fIsPolished(false)
    , m_pTabWidget(0)
    , m_pSearchPanel(0)
    , m_pFilterPanel(0)
    , m_pBookmarksPanel(0)
    , m_pOptionsPanel(0)
    , m_pMainLayout(0)
    , m_pToolBar(0)
    , m_bShowLineNumbers(true)
    , m_bWrapLines(false)
    , m_font(QFontDatabase::systemFont(QFontDatabase::FixedFont))
    , m_pCornerButton(0)
    , m_pMachineSelectionMenu(0)
{
    /* Prepare VM Log-Viewer: */
    prepare();
    restorePanelVisibility();
    if (!comMachine.isNull())
        setMachines(QVector<QUuid>(1, comMachine.GetId()));
}

UIVMLogViewerWidget::~UIVMLogViewerWidget()
{
}

int UIVMLogViewerWidget::defaultLogPageWidth() const
{
    if (!m_pTabWidget)
        return 0;

    QWidget *pContainer = m_pTabWidget->currentWidget();
    if (!pContainer)
        return 0;

    QPlainTextEdit *pBrowser = pContainer->findChild<QPlainTextEdit*>();
    if (!pBrowser)
        return 0;
    /* Compute a width for 132 characters plus scrollbar and frame width: */
    int iDefaultWidth = pBrowser->fontMetrics().width(QChar('x')) * 132 +
                        pBrowser->verticalScrollBar()->width() +
                        pBrowser->frameWidth() * 2;

    return iDefaultWidth;
}

QMenu *UIVMLogViewerWidget::menu() const
{
    return m_pActionPool->action(UIActionIndex_M_LogWindow)->menu();
}

void UIVMLogViewerWidget::setSelectedVMListItems(const QList<UIVirtualMachineItem*> &items)
{
    QVector<QUuid> selectedMachines;

    foreach (const UIVirtualMachineItem *item, items)
    {
        if (!item)
            continue;
        selectedMachines << item->id();
    }
    setMachines(selectedMachines);
}

void UIVMLogViewerWidget::setMachines(const QVector<QUuid> &machineIDs)
{
    /* List of machines that are newly added to selected machine list: */
    QVector<QUuid> newSelections;
    QVector<QUuid> unselectedMachines(m_machines);

    foreach (const QUuid &id, machineIDs)
    {
        unselectedMachines.removeAll(id);
        if (!m_machines.contains(id))
            newSelections << id;
    }
    m_machines = machineIDs;

    m_pTabWidget->hide();
    /* Read logs and create pages/tabs for newly selected machines: */
    createLogViewerPages(newSelections);
    /* Remove the log pages/tabs of unselected machines from the tab widget: */
    removeLogViewerPages(unselectedMachines);
    /* Assign color indexes to tabs based on machines. We use two alternating colors to indicate different machine logs. */
    setTabColorPerMachine();
    m_pTabWidget->show();
}

void UIVMLogViewerWidget::setTabColorPerMachine()
{
    if (!m_pTabWidget || !m_pTabWidget->tabBar() || m_pTabWidget->tabBar()->count() == 0)
        return;
    QTabBar *pTabBar = m_pTabWidget->tabBar();
    int iColorIndex = 0;
    pTabBar->setTabData(0, iColorIndex);

    for (int i = 1; i < pTabBar->count(); ++i)
    {
        UIVMLogPage *pLogPage = logPage(i);
        UIVMLogPage *pLogPagePrev = logPage(i - 1);
        if (!pLogPage || !pLogPagePrev)
            continue;
        /* We the machine if the tab is different than that of previous alternate the color index. */
        if (pLogPage->machineId() != pLogPagePrev->machineId())
            ++iColorIndex;
        pTabBar->setTabData(i, iColorIndex % 2);
    }
}

QString UIVMLogViewerWidget::readLogFile(CMachine &comMachine, int iLogFileId)
{
    QString strLogFileContent;
    ULONG uOffset = 0;

    while (true)
    {
        QVector<BYTE> data = comMachine.ReadLog(iLogFileId, uOffset, _1M);
        if (data.size() == 0)
            break;
        strLogFileContent.append(QString::fromUtf8((char*)data.data(), data.size()));
        uOffset += data.size();
        /* Don't read futher if we have reached the allowed size limit: */
        if (uOffset >= uAllowedLogSize)
        {
            strLogFileContent.append("\n=========Log file has been truncated as it is too large.======");
            break;
        }
    }
    return strLogFileContent;
}

QFont UIVMLogViewerWidget::currentFont() const
{
    const UIVMLogPage* logPage = currentLogPage();
    if (!logPage)
        return QFont();
    return logPage->currentFont();
}

bool UIVMLogViewerWidget::shouldBeMaximized() const
{
    return gEDataManager->logWindowShouldBeMaximized();
}

void UIVMLogViewerWidget::sltSaveOptions()
{
    /* Save a list of currently visible panels: */
    QStringList strNameList;
    foreach(UIDialogPanel* pPanel, m_visiblePanelsList)
        strNameList.append(pPanel->panelName());
    gEDataManager->setLogViewerVisiblePanels(strNameList);
    gEDataManager->setLogViweverOptions(m_font, m_bWrapLines, m_bShowLineNumbers);
}

void UIVMLogViewerWidget::sltRefresh()
{
    if (!m_pTabWidget)
        return;

    UIVMLogPage *pCurrentPage = currentLogPage();
    if (!pCurrentPage || pCurrentPage->logFileId() == -1)
        return;

    CMachine comMachine = uiCommon().virtualBox().FindMachine(pCurrentPage->machineId().toString());
    if (comMachine.isNull())
        return;

    QString strLogContent = readLogFile(comMachine, pCurrentPage->logFileId());
    pCurrentPage->setLogContent(strLogContent, false);

    if (m_pSearchPanel && m_pSearchPanel->isVisible())
        m_pSearchPanel->refresh();

    /* Re-Apply the filter settings: */
    if (m_pFilterPanel)
        m_pFilterPanel->applyFilter();
}

void UIVMLogViewerWidget::sltReload()
{
    if (!m_pTabWidget)
        return;

    m_pTabWidget->blockSignals(true);
    m_pTabWidget->hide();

    removeAllLogPages();
    createLogViewerPages(m_machines);

    /* re-Apply the filter settings: */
    if (m_pFilterPanel)
        m_pFilterPanel->applyFilter();

    m_pTabWidget->blockSignals(false);
    setTabColorPerMachine();
    m_pTabWidget->show();
}

void UIVMLogViewerWidget::sltSave()
{
    UIVMLogPage *pLogPage = currentLogPage();
    if (!pLogPage)
        return;

    CMachine comMachine = uiCommon().virtualBox().FindMachine(pLogPage->machineId().toString());
    if (comMachine.isNull())
        return;

    const QString& fileName = pLogPage->logFileName();
    if (fileName.isEmpty())
        return;
    /* Prepare "save as" dialog: */
    const QFileInfo fileInfo(fileName);
    /* Prepare default filename: */
    const QDateTime dtInfo = fileInfo.lastModified();
    const QString strDtString = dtInfo.toString("yyyy-MM-dd-hh-mm-ss");
    const QString strDefaultFileName = QString("%1-%2.log").arg(comMachine.GetName()).arg(strDtString);
    const QString strDefaultFullName = QDir::toNativeSeparators(QDir::home().absolutePath() + "/" + strDefaultFileName);

    const QString strNewFileName = QIFileDialog::getSaveFileName(strDefaultFullName,
                                                                 "",
                                                                 this,
                                                                 tr("Save VirtualBox Log As"),
                                                                 0 /* selected filter */,
                                                                 true /* resolve symlinks */,
                                                                 true /* confirm overwrite */);
    /* Make sure file-name is not empty: */
    if (!strNewFileName.isEmpty())
    {
        /* Delete the previous file if already exists as user already confirmed: */
        if (QFile::exists(strNewFileName))
            QFile::remove(strNewFileName);
        /* Copy log into the file: */
        QFile::copy(comMachine.QueryLogFilename(m_pTabWidget->currentIndex()), strNewFileName);
    }
}

void UIVMLogViewerWidget::sltDeleteBookmark(int index)
{
    UIVMLogPage* pLogPage = currentLogPage();
    if (!pLogPage)
        return;
    pLogPage->deleteBookmark(index);
    if (m_pBookmarksPanel)
        m_pBookmarksPanel->updateBookmarkList(pLogPage->bookmarkVector());
}

void UIVMLogViewerWidget::sltDeleteAllBookmarks()
{
    UIVMLogPage* pLogPage = currentLogPage();
    if (!pLogPage)
        return;
    pLogPage->deleteAllBookmarks();

    if (m_pBookmarksPanel)
        m_pBookmarksPanel->updateBookmarkList(pLogPage->bookmarkVector());
}

void UIVMLogViewerWidget::sltUpdateBookmarkPanel()
{
    if (!currentLogPage() || !m_pBookmarksPanel)
        return;
    m_pBookmarksPanel->updateBookmarkList(currentLogPage()->bookmarkVector());
}

void UIVMLogViewerWidget::gotoBookmark(int bookmarkIndex)
{
    if (!currentLogPage())
        return;
    currentLogPage()->scrollToBookmark(bookmarkIndex);
}

void UIVMLogViewerWidget::sltPanelActionToggled(bool fChecked)
{
    QAction *pSenderAction = qobject_cast<QAction*>(sender());
    if (!pSenderAction)
        return;
    UIDialogPanel* pPanel = 0;
    /* Look for the sender() within the m_panelActionMap's values: */
    for (QMap<UIDialogPanel*, QAction*>::const_iterator iterator = m_panelActionMap.begin();
        iterator != m_panelActionMap.end(); ++iterator)
    {
        if (iterator.value() == pSenderAction)
            pPanel = iterator.key();
    }
    if (!pPanel)
        return;
    if (fChecked)
        showPanel(pPanel);
    else
        hidePanel(pPanel);
}

void UIVMLogViewerWidget::sltSearchResultHighLigting()
{
    if (!m_pSearchPanel || !currentLogPage())
        return;
    currentLogPage()->setScrollBarMarkingsVector(m_pSearchPanel->matchLocationVector());
}

void UIVMLogViewerWidget::sltHandleSearchUpdated()
{
    if (!m_pSearchPanel || !currentLogPage())
        return;
}

void UIVMLogViewerWidget::sltCurrentTabChanged(int tabIndex)
{
    Q_UNUSED(tabIndex);

    /* Dont refresh the search here as it is refreshed by the filtering mechanism
       which is updated as tab current index changes (see sltFilterApplied): */
    if (m_pFilterPanel)
        m_pFilterPanel->applyFilter();

    /* We keep a separate QVector<LogBookmark> for each log page: */
    if (m_pBookmarksPanel && currentLogPage())
        m_pBookmarksPanel->updateBookmarkList(currentLogPage()->bookmarkVector());
}

void UIVMLogViewerWidget::sltFilterApplied()
{
    /* Reapply the search to get highlighting etc. correctly */
    if (m_pSearchPanel)
        m_pSearchPanel->refresh();
}

void UIVMLogViewerWidget::sltLogPageFilteredChanged(bool isFiltered)
{
    /* Disable bookmark panel since bookmarks are stored as line numbers within
       the original log text and does not mean much in a reduced/filtered one. */
    if (m_pBookmarksPanel)
        m_pBookmarksPanel->disableEnableBookmarking(!isFiltered);
}

void UIVMLogViewerWidget::sltHandleHidePanel(UIDialogPanel *pPanel)
{
    hidePanel(pPanel);
}

void UIVMLogViewerWidget::sltHandleShowPanel(UIDialogPanel *pPanel)
{
    showPanel(pPanel);
}

void UIVMLogViewerWidget::sltShowLineNumbers(bool bShowLineNumbers)
{
    if (m_bShowLineNumbers == bShowLineNumbers)
        return;

    m_bShowLineNumbers = bShowLineNumbers;
    /* Set all log page instances. */
    for (int i = 0; m_pTabWidget && (i <  m_pTabWidget->count()); ++i)
    {
        UIVMLogPage* pLogPage = logPage(i);
        if (pLogPage)
            pLogPage->setShowLineNumbers(m_bShowLineNumbers);
    }
}

void UIVMLogViewerWidget::sltWrapLines(bool bWrapLines)
{
    if (m_bWrapLines == bWrapLines)
        return;

    m_bWrapLines = bWrapLines;
    /* Set all log page instances. */
    for (int i = 0; m_pTabWidget && (i <  m_pTabWidget->count()); ++i)
    {
        UIVMLogPage* pLogPage = logPage(i);
        if (pLogPage)
            pLogPage->setWrapLines(m_bWrapLines);
    }
}

void UIVMLogViewerWidget::sltFontSizeChanged(int fontSize)
{
    if (m_font.pointSize() == fontSize)
        return;
    m_font.setPointSize(fontSize);
    for (int i = 0; m_pTabWidget && (i <  m_pTabWidget->count()); ++i)
    {
        UIVMLogPage* pLogPage = logPage(i);
        if (pLogPage)
            pLogPage->setCurrentFont(m_font);
    }
}

void UIVMLogViewerWidget::sltChangeFont(QFont font)
{
    if (m_font == font)
        return;
    m_font = font;
    for (int i = 0; m_pTabWidget && (i <  m_pTabWidget->count()); ++i)
    {
        UIVMLogPage* pLogPage = logPage(i);
        if (pLogPage)
            pLogPage->setCurrentFont(m_font);
    }
}

void UIVMLogViewerWidget::sltResetOptionsToDefault()
{
    sltShowLineNumbers(true);
    sltWrapLines(false);
    sltChangeFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    if (m_pOptionsPanel)
    {
        m_pOptionsPanel->setShowLineNumbers(true);
        m_pOptionsPanel->setWrapLines(false);
        m_pOptionsPanel->setFontSizeInPoints(m_font.pointSize());
    }
}

void UIVMLogViewerWidget::prepare()
{
    /* Load options: */
    loadOptions();

    /* Prepare stuff: */
    prepareActions();
    /* Prepare widgets: */
    prepareWidgets();

    /* Apply language settings: */
    retranslateUi();

    /* Setup escape shortcut: */
    manageEscapeShortCut();
    uiCommon().setHelpKeyword(this, "collect-debug-info");
}

void UIVMLogViewerWidget::prepareActions()
{
    /* First of all, add actions which has smaller shortcut scope: */
    addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Find));
    addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Filter));
    addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Bookmark));
    addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Options));
    addAction(m_pActionPool->action(UIActionIndex_M_Log_S_Refresh));
    addAction(m_pActionPool->action(UIActionIndex_M_Log_S_Save));

    /* Connect actions: */
    connect(m_pActionPool->action(UIActionIndex_M_Log_T_Find), &QAction::toggled,
            this, &UIVMLogViewerWidget::sltPanelActionToggled);
    connect(m_pActionPool->action(UIActionIndex_M_Log_T_Filter), &QAction::toggled,
            this, &UIVMLogViewerWidget::sltPanelActionToggled);
    connect(m_pActionPool->action(UIActionIndex_M_Log_T_Bookmark), &QAction::toggled,
            this, &UIVMLogViewerWidget::sltPanelActionToggled);
    connect(m_pActionPool->action(UIActionIndex_M_Log_T_Options), &QAction::toggled,
            this, &UIVMLogViewerWidget::sltPanelActionToggled);
    connect(m_pActionPool->action(UIActionIndex_M_Log_S_Refresh), &QAction::triggered,
            this, &UIVMLogViewerWidget::sltRefresh);
    connect(m_pActionPool->action(UIActionIndex_M_Log_S_Reload), &QAction::triggered,
            this, &UIVMLogViewerWidget::sltReload);
    connect(m_pActionPool->action(UIActionIndex_M_Log_S_Save), &QAction::triggered,
            this, &UIVMLogViewerWidget::sltSave);
}

void UIVMLogViewerWidget::prepareWidgets()
{
    /* Create main layout: */
    m_pMainLayout = new QVBoxLayout(this);
    if (m_pMainLayout)
    {
        /* Configure layout: */
        m_pMainLayout->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
        m_pMainLayout->setSpacing(10);
#else
        m_pMainLayout->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2);
#endif

        /* Prepare toolbar, if requested: */
        if (m_fShowToolbar)
            prepareToolBar();

        /* Create VM Log-Viewer container: */
        m_pTabWidget = new UITabWidget;
        if (m_pTabWidget)
        {
            /* Add into layout: */
            m_pMainLayout->addWidget(m_pTabWidget);
            connect(m_pTabWidget, &QITabWidget::currentChanged, this, &UIVMLogViewerWidget::sltCurrentTabChanged);
        }

        /* Create VM Log-Viewer search-panel: */
        m_pSearchPanel = new UIVMLogViewerSearchPanel(0, this);
        if (m_pSearchPanel)
        {
            /* Configure panel: */
            installEventFilter(m_pSearchPanel);
            m_pSearchPanel->hide();
            connect(m_pSearchPanel, &UIVMLogViewerSearchPanel::sigHighlightingUpdated,
                    this, &UIVMLogViewerWidget::sltSearchResultHighLigting);
            connect(m_pSearchPanel, &UIVMLogViewerSearchPanel::sigSearchUpdated,
                    this, &UIVMLogViewerWidget::sltHandleSearchUpdated);
            connect(m_pSearchPanel, &UIVMLogViewerSearchPanel::sigHidePanel,
                    this, &UIVMLogViewerWidget::sltHandleHidePanel);
            connect(m_pSearchPanel, &UIVMLogViewerSearchPanel::sigShowPanel,
                    this, &UIVMLogViewerWidget::sltHandleShowPanel);
            m_panelActionMap.insert(m_pSearchPanel, m_pActionPool->action(UIActionIndex_M_Log_T_Find));

            /* Add into layout: */
            m_pMainLayout->addWidget(m_pSearchPanel);
        }

        /* Create VM Log-Viewer filter-panel: */
        m_pFilterPanel = new UIVMLogViewerFilterPanel(0, this);
        if (m_pFilterPanel)
        {
            /* Configure panel: */
            installEventFilter(m_pFilterPanel);
            m_pFilterPanel->hide();
            connect(m_pFilterPanel, &UIVMLogViewerFilterPanel::sigFilterApplied,
                    this, &UIVMLogViewerWidget::sltFilterApplied);
            connect(m_pFilterPanel, &UIVMLogViewerFilterPanel::sigHidePanel,
                    this, &UIVMLogViewerWidget::sltHandleHidePanel);
           connect(m_pFilterPanel, &UIVMLogViewerFilterPanel::sigShowPanel,
                    this, &UIVMLogViewerWidget::sltHandleShowPanel);
            m_panelActionMap.insert(m_pFilterPanel, m_pActionPool->action(UIActionIndex_M_Log_T_Filter));

            /* Add into layout: */
            m_pMainLayout->addWidget(m_pFilterPanel);
        }

        /* Create VM Log-Viewer bookmarks-panel: */
        m_pBookmarksPanel = new UIVMLogViewerBookmarksPanel(0, this);
        if (m_pBookmarksPanel)
        {
            /* Configure panel: */
            m_pBookmarksPanel->hide();
            connect(m_pBookmarksPanel, &UIVMLogViewerBookmarksPanel::sigDeleteBookmark,
                    this, &UIVMLogViewerWidget::sltDeleteBookmark);
            connect(m_pBookmarksPanel, &UIVMLogViewerBookmarksPanel::sigDeleteAllBookmarks,
                    this, &UIVMLogViewerWidget::sltDeleteAllBookmarks);
            connect(m_pBookmarksPanel, &UIVMLogViewerBookmarksPanel::sigBookmarkSelected,
                    this, &UIVMLogViewerWidget::gotoBookmark);
            m_panelActionMap.insert(m_pBookmarksPanel, m_pActionPool->action(UIActionIndex_M_Log_T_Bookmark));
            connect(m_pBookmarksPanel, &UIVMLogViewerBookmarksPanel::sigHidePanel,
                    this, &UIVMLogViewerWidget::sltHandleHidePanel);
            connect(m_pBookmarksPanel, &UIVMLogViewerBookmarksPanel::sigShowPanel,
                    this, &UIVMLogViewerWidget::sltHandleShowPanel);
            /* Add into layout: */
            m_pMainLayout->addWidget(m_pBookmarksPanel);
        }

        /* Create VM Log-Viewer options-panel: */
        m_pOptionsPanel = new UIVMLogViewerOptionsPanel(0, this);
        if (m_pOptionsPanel)
        {
            /* Configure panel: */
            m_pOptionsPanel->hide();
            m_pOptionsPanel->setShowLineNumbers(m_bShowLineNumbers);
            m_pOptionsPanel->setWrapLines(m_bWrapLines);
            m_pOptionsPanel->setFontSizeInPoints(m_font.pointSize());
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigShowLineNumbers, this, &UIVMLogViewerWidget::sltShowLineNumbers);
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigWrapLines, this, &UIVMLogViewerWidget::sltWrapLines);
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigChangeFontSizeInPoints, this, &UIVMLogViewerWidget::sltFontSizeChanged);
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigChangeFont, this, &UIVMLogViewerWidget::sltChangeFont);
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigResetToDefaults, this, &UIVMLogViewerWidget::sltResetOptionsToDefault);
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigHidePanel, this, &UIVMLogViewerWidget::sltHandleHidePanel);
            connect(m_pOptionsPanel, &UIVMLogViewerOptionsPanel::sigShowPanel, this, &UIVMLogViewerWidget::sltHandleShowPanel);

            m_panelActionMap.insert(m_pOptionsPanel, m_pActionPool->action(UIActionIndex_M_Log_T_Options));

            /* Add into layout: */
            m_pMainLayout->addWidget(m_pOptionsPanel);
        }
    }
}

void UIVMLogViewerWidget::prepareToolBar()
{
    /* Create toolbar: */
    m_pToolBar = new QIToolBar(parentWidget());
    if (m_pToolBar)
    {
        /* Configure toolbar: */
        const int iIconMetric = (int)(QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize));
        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        /* Add toolbar actions: */
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_S_Save));
        m_pToolBar->addSeparator();
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Find));
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Filter));
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Bookmark));
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_T_Options));
        m_pToolBar->addSeparator();
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_S_Refresh));
        m_pToolBar->addAction(m_pActionPool->action(UIActionIndex_M_Log_S_Reload));

#ifdef VBOX_WS_MAC
        /* Check whether we are embedded into a stack: */
        if (m_enmEmbedding == EmbedTo_Stack)
        {
            /* Add into layout: */
            m_pMainLayout->addWidget(m_pToolBar);
        }
#else
        /* Add into layout: */
        m_pMainLayout->addWidget(m_pToolBar);
#endif
    }
}

void UIVMLogViewerWidget::loadOptions()
{
    m_bWrapLines = gEDataManager->logViewerWrapLines();
    m_bShowLineNumbers = gEDataManager->logViewerShowLineNumbers();
    QFont loadedFont = gEDataManager->logViewerFont();
    if (loadedFont != QFont())
        m_font = loadedFont;
    connect(&uiCommon(), &UICommon::sigAskToCommitData,
            this, &UIVMLogViewerWidget::sltSaveOptions);
}

void UIVMLogViewerWidget::restorePanelVisibility()
{
    /** Reset the action states first: */
    foreach(QAction* pAction, m_panelActionMap.values())
    {
        pAction->blockSignals(true);
        pAction->setChecked(false);
        pAction->blockSignals(false);
    }

    /* Load the visible panel list and show them: */
    QStringList strNameList = gEDataManager->logViewerVisiblePanels();
    foreach(const QString strName, strNameList)
    {
        foreach(UIDialogPanel* pPanel, m_panelActionMap.keys())
        {
            if (strName == pPanel->panelName())
            {
                showPanel(pPanel);
                break;
            }
        }
    }
}

void UIVMLogViewerWidget::retranslateUi()
{
    /* Translate toolbar: */
#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text. */
    if (m_pToolBar)
        m_pToolBar->updateLayout();
#endif
    if (m_pCornerButton)
        m_pCornerButton->setToolTip(tr("Select machines to show their log"));
}

void UIVMLogViewerWidget::showEvent(QShowEvent *pEvent)
{
    QWidget::showEvent(pEvent);

    /* One may think that QWidget::polish() is the right place to do things
     * below, but apparently, by the time when QWidget::polish() is called,
     * the widget style & layout are not fully done, at least the minimum
     * size hint is not properly calculated. Since this is sometimes necessary,
     * we provide our own "polish" implementation: */

    if (m_fIsPolished)
        return;

    m_fIsPolished = true;
}

void UIVMLogViewerWidget::keyPressEvent(QKeyEvent *pEvent)
{
    /* Depending on key pressed: */
    switch (pEvent->key())
    {
        /* Process Back key as switch to previous tab: */
        case Qt::Key_Back:
        {
            if (m_pTabWidget->currentIndex() > 0)
            {
                m_pTabWidget->setCurrentIndex(m_pTabWidget->currentIndex() - 1);
                return;
            }
            break;
        }
        /* Process Forward key as switch to next tab: */
        case Qt::Key_Forward:
        {
            if (m_pTabWidget->currentIndex() < m_pTabWidget->count())
            {
                m_pTabWidget->setCurrentIndex(m_pTabWidget->currentIndex() + 1);
                return;
            }
            break;
        }
        default:
            break;
    }
    QWidget::keyPressEvent(pEvent);
}

QPlainTextEdit* UIVMLogViewerWidget::logPage(int pIndex) const
{
    if (m_pTabWidget)
        return 0;
    QWidget* pContainer = m_pTabWidget->widget(pIndex);
    if (!pContainer)
        return 0;
    QPlainTextEdit *pBrowser = pContainer->findChild<QPlainTextEdit*>();
    return pBrowser;
}

QVector<UIVMLogPage*> UIVMLogViewerWidget::logPages()
{
    QVector<UIVMLogPage*> pages;
    if (m_pTabWidget)
        return pages;
    for (int i = 0; i < m_pTabWidget->count(); ++i)
    {
        UIVMLogPage *pPage = logPage(i);
        if (pPage)
            pages << pPage;
    }
    return pages;
}

void UIVMLogViewerWidget::createLogPage(const QString &strFileName, const QString &strMachineName,
                                        const QUuid &machineId, int iLogFileId,
                                        const QString &strLogContent, bool noLogsToShow)
{
    if (!m_pTabWidget)
        return;

    /* Create page-container: */
    UIVMLogPage* pLogPage = new UIVMLogPage(this);
    if (pLogPage)
    {
        connect(pLogPage, &UIVMLogPage::sigBookmarksUpdated, this, &UIVMLogViewerWidget::sltUpdateBookmarkPanel);
        connect(pLogPage, &UIVMLogPage::sigLogPageFilteredChanged, this, &UIVMLogViewerWidget::sltLogPageFilteredChanged);
        /* Initialize setting for this log page */
        pLogPage->setShowLineNumbers(m_bShowLineNumbers);
        pLogPage->setWrapLines(m_bWrapLines);
        pLogPage->setCurrentFont(m_font);
        pLogPage->setMachineId(machineId);
        pLogPage->setLogFileId(iLogFileId);
        /* Set the file name only if we really have log file to read. */
        if (!noLogsToShow)
            pLogPage->setLogFileName(strFileName);

        /* Add page-container to viewer-container in stacked mode (manager UI case): */
        bool fTitleWithMachineName = m_enmEmbedding == EmbedTo_Stack;
        QString strTabTitle;
        if (fTitleWithMachineName)
        {
            strTabTitle.append(strMachineName);
            strTabTitle.append(" - ");
        }
        strTabTitle.append(QFileInfo(strFileName).fileName());
        m_pTabWidget->addTab(pLogPage, strTabTitle);

        pLogPage->setLogContent(strLogContent, noLogsToShow);
        pLogPage->setScrollBarMarkingsVector(m_pSearchPanel->matchLocationVector());
    }
}

const UIVMLogPage *UIVMLogViewerWidget::currentLogPage() const
{
    if (!m_pTabWidget)
        return 0;
    return qobject_cast<const UIVMLogPage*>(m_pTabWidget->currentWidget());
}

UIVMLogPage *UIVMLogViewerWidget::currentLogPage()
{
    if (!m_pTabWidget)
        return 0;
    return qobject_cast<UIVMLogPage*>(m_pTabWidget->currentWidget());
}

UIVMLogPage *UIVMLogViewerWidget::logPage(int iIndex)
{
    if (!m_pTabWidget)
        return 0;
    return qobject_cast<UIVMLogPage*>(m_pTabWidget->widget(iIndex));
}

void UIVMLogViewerWidget::createLogViewerPages(const QVector<QUuid> &machineList)
{
    const CSystemProperties &sys = uiCommon().virtualBox().GetSystemProperties();
    unsigned cMaxLogs = sys.GetLogHistoryCount() + 1 /*VBox.log*/ + 1 /*VBoxHardening.log*/; /** @todo Add api for getting total possible log count! */
    foreach (const QUuid &machineId, machineList)
    {
        CMachine comMachine = uiCommon().virtualBox().FindMachine(machineId.toString());
        if (comMachine.isNull())
            continue;
        bool fNoLogFileForMachine = true;

        QUuid uMachineId = comMachine.GetId();
        QString strMachineName = comMachine.GetName();
        for (unsigned iLogFileId = 0; iLogFileId < cMaxLogs; ++iLogFileId)
        {
            QString strLogContent = readLogFile(comMachine, iLogFileId);
            if (!strLogContent.isEmpty())
            {
                fNoLogFileForMachine = false;
                createLogPage(comMachine.QueryLogFilename(iLogFileId),
                              strMachineName, uMachineId, iLogFileId,
                              strLogContent, false);
            }
        }
        if (fNoLogFileForMachine)
        {
            QString strDummyTabText = QString(tr("<p>No log files for the machine %1 found. Press the "
                                                 "<b>Rescan</b> button to rescan the log folder "
                                                 "<nobr><b>%2</b></nobr>.</p>")
                                              .arg(strMachineName).arg(comMachine.GetLogFolder()));
            createLogPage(tr("NoLogFile"), strMachineName, uMachineId, -1 /* iLogFileId */, strDummyTabText, true);
        }
    }
}

void UIVMLogViewerWidget::removeLogViewerPages(const QVector<QUuid> &machineList)
{
    /* Nothing to do: */
    if (machineList.isEmpty() || !m_pTabWidget)
        return;
    m_pTabWidget->blockSignals(true);
    /* Cache log page pointers and tab titles: */
    QVector<QPair<UIVMLogPage*, QString> > logPages;
    for (int i = 0; i < m_pTabWidget->count(); ++i)
    {
        UIVMLogPage *pTab = logPage(i);
        if (pTab)
            logPages << QPair<UIVMLogPage*, QString>(pTab, m_pTabWidget->tabText(i));
    }
    /* Remove all the tabs from tab widget, note that this does not delete tab widgets: */
    m_pTabWidget->clear();
    QVector<UIVMLogPage*> pagesToRemove;
    /* Add tab widgets (log pages) back as long as machine id is not in machineList: */
    for (int i = 0; i < logPages.size(); ++i)
    {
        if (!logPages[i].first)
            continue;
        const QUuid &id = logPages[i].first->machineId();

        if (machineList.contains(id))
            pagesToRemove << logPages[i].first;
        else
            m_pTabWidget->addTab(logPages[i].first, logPages[i].second);
    }
    /* Delete all the other pages: */
    qDeleteAll(pagesToRemove.begin(), pagesToRemove.end());
    m_pTabWidget->blockSignals(false);
}

void UIVMLogViewerWidget::removeAllLogPages()
{
    if (!m_pTabWidget)
        return;

    QVector<UIVMLogPage*> pagesToRemove = logPages();
    m_pTabWidget->clear();
    qDeleteAll(pagesToRemove.begin(), pagesToRemove.end());
}

void UIVMLogViewerWidget::resetHighlighthing()
{
    /* Undo the document changes to remove highlighting: */
    UIVMLogPage* logPage = currentLogPage();
    if (!logPage)
        return;
    logPage->documentUndo();
    logPage->clearScrollBarMarkingsVector();
}

void UIVMLogViewerWidget::hidePanel(UIDialogPanel* panel)
{
    if (!panel)
        return;
    if (panel->isVisible())
        panel->setVisible(false);
    QMap<UIDialogPanel*, QAction*>::iterator iterator = m_panelActionMap.find(panel);
    if (iterator != m_panelActionMap.end())
    {
        if (iterator.value() && iterator.value()->isChecked())
            iterator.value()->setChecked(false);
    }
    m_visiblePanelsList.removeOne(panel);
    manageEscapeShortCut();
}

void UIVMLogViewerWidget::showPanel(UIDialogPanel* panel)
{
    if (panel && panel->isHidden())
        panel->setVisible(true);
    QMap<UIDialogPanel*, QAction*>::iterator iterator = m_panelActionMap.find(panel);
    if (iterator != m_panelActionMap.end())
    {
        if (!iterator.value()->isChecked())
            iterator.value()->setChecked(true);
    }
    if (!m_visiblePanelsList.contains(panel))
        m_visiblePanelsList.push_back(panel);
    manageEscapeShortCut();
}

void UIVMLogViewerWidget::manageEscapeShortCut()
{
    /* if there is no visible panels give the escape shortcut to parent dialog: */
    if (m_visiblePanelsList.isEmpty())
    {
        emit sigSetCloseButtonShortCut(QKeySequence(Qt::Key_Escape));
        return;
    }
    /* Take the escape shortcut from the dialog: */
    emit sigSetCloseButtonShortCut(QKeySequence());
    /* Just loop thru the visible panel list and set the esc key to the
       panel which made visible latest */
    for (int i = 0; i < m_visiblePanelsList.size() - 1; ++i)
    {
        m_visiblePanelsList[i]->setCloseButtonShortCut(QKeySequence());
    }
    m_visiblePanelsList.back()->setCloseButtonShortCut(QKeySequence(Qt::Key_Escape));
}

#include "UIVMLogViewerWidget.moc"
