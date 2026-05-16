#include "mainwindow.h"
#include "helpers.h"
#include "searchworker.h"
#include "filepropertiesdialog.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QtConcurrent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>
#include <QLocale>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QObject>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSize>
#include <QStandardPaths>
#include <QStringList>
#include <QStringView>
#include <QThread>
#include <QUrl>
#include <utility> // Für std::as_const

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

MainWindow::MainWindow(const QString &targetDirectory, QWidget *parent)
    : QMainWindow(parent), m_currentDirectory(targetDirectory)
{
    setWindowTitle(QDir::toNativeSeparators(m_currentDirectory));
    setWindowIcon(QIcon(":/icons/res/app.ico"));
    resize(728, 545);

    m_centralWidget = new QWidget(this);
    setCentralWidget(m_centralWidget);

    m_mainLayout = new QVBoxLayout(m_centralWidget);

    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout1 = new QVBoxLayout();
    //topControlsVBoxLayout1->setSpacing(15);
    topControlsVBoxLayout1->setContentsMargins(5, 5, 5, 5);

    m_LineEdit1 = new QLineEdit();
    topControlsVBoxLayout1->addWidget(m_LineEdit1);

    m_LineEdit2 = new QLineEdit();
    topControlsVBoxLayout1->addWidget(m_LineEdit2);

    if (m_settings.showPlaceholderText == true) {
        m_LineEdit1->setPlaceholderText(tr("(filename search terms)"));
        m_LineEdit2->setPlaceholderText(tr("(content search terms)"));
    }

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout2 = new QVBoxLayout();
    //topControlsVBoxLayout2->setSpacing(15);
    topControlsVBoxLayout2->setContentsMargins(5, 5, 5, 5);

    m_CheckboxRegExName = new QCheckBox("RegEx");
    m_CheckboxRegExName->setChecked(false);
    topControlsVBoxLayout2->addWidget(m_CheckboxRegExName);

    m_CheckboxRegExContent = new QCheckBox("RegEx");
    m_CheckboxRegExContent->setChecked(false);
    topControlsVBoxLayout2->addWidget(m_CheckboxRegExContent);

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout3 = new QVBoxLayout();
    //topControlsVBoxLayout3->setSpacing(15);
    topControlsVBoxLayout3->setContentsMargins(5, 5, 5, 5);

    m_CheckboxNameCaseSense = new QCheckBox(tr("CaseSense"));
    m_CheckboxNameCaseSense->setChecked(false);
    topControlsVBoxLayout3->addWidget(m_CheckboxNameCaseSense);

    m_CheckboxContentCaseSense = new QCheckBox(tr("CaseSense"));
    m_CheckboxContentCaseSense->setChecked(false);
    topControlsVBoxLayout3->addWidget(m_CheckboxContentCaseSense);

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout4 = new QVBoxLayout();
    //topControlsVBoxLayout4->setSpacing(15);
    topControlsVBoxLayout4->setContentsMargins(5, 5, 25, 5);

    m_CheckboxDirectories = new QCheckBox(tr("Directories"));
    m_CheckboxDirectories->setChecked(false);
    m_CheckboxDirectories->setTristate(true);
    topControlsVBoxLayout4->addWidget(m_CheckboxDirectories);

    m_CheckboxCRC = new QCheckBox(tr("CRC"));
    m_CheckboxCRC->setChecked(false);
    topControlsVBoxLayout4->addWidget(m_CheckboxCRC);

    // --------------------------------------------------------------------

    // Container for editbox and checkbox controls

    m_topControlsContainerWidget = new QWidget();
    QHBoxLayout *topControlsHBoxLayout = new QHBoxLayout(m_topControlsContainerWidget);
    topControlsHBoxLayout->setContentsMargins(0, 0, 0, 0);
    topControlsHBoxLayout->setSpacing(5);

    topControlsHBoxLayout->addLayout(topControlsVBoxLayout1);
    topControlsHBoxLayout->addLayout(topControlsVBoxLayout2);
    topControlsHBoxLayout->addLayout(topControlsVBoxLayout3);
    topControlsHBoxLayout->addLayout(topControlsVBoxLayout4);

    // --------------------------------------------------------------------
    // ListView

    m_tableWidget = new Custom_QTableWidget();
    m_tableWidget->setItemDelegate(new CutDelegate(this));
    m_tableWidget->setEditTriggers(QAbstractItemView::EditKeyPressed);    // QAbstractItemView::NoEditTriggers
    m_tableWidget->setStyleSheet("QTableWidget { border: none; }");
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->verticalHeader()->setMinimumSectionSize(0);
    m_tableWidget->verticalHeader()->setDefaultSectionSize(18);
    m_tableWidget->setColumnCount(8);
    m_tableWidget->setHorizontalHeaderLabels({tr("Name"), tr("Subfolder"), tr("Size"), tr("Changed"), tr("Type"), tr("Rating"), tr("Count"), tr("CRC")});

    m_tableWidget->setColumnWidth(eColName, 160);
    m_tableWidget->setColumnWidth(eColPath, 160);
    m_tableWidget->setColumnWidth(eColSize, 60);
    m_tableWidget->setColumnWidth(eColDate, 118);
    m_tableWidget->setColumnWidth(eColType, 46);
    m_tableWidget->setColumnWidth(eColQuality, 45);
    m_tableWidget->setColumnWidth(eColCount, 42);
    m_tableWidget->setColumnWidth(eColCRC, 80);

    m_tableWidget->horizontalHeaderItem(eColName)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColPath)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColSize)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColDate)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColType)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColQuality)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColCount)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    m_tableWidget->horizontalHeaderItem(eColCRC)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    m_tableWidget->horizontalHeader()->setSectionsMovable(true);
    m_tableWidget->horizontalHeader()->setHighlightSections(false);

    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    m_tableWidget->setAlternatingRowColors(m_settings.alternatingRowColors);
    m_tableWidget->setShowGrid(m_settings.showGrid);
    m_tableWidget->setColumnHidden(eColCRC, true);

    m_tableWidget->resizeColumnToContents(eColSize);
    m_tableWidget->resizeColumnToContents(eColDate);
    m_tableWidget->resizeColumnToContents(eColType);
    m_tableWidget->resizeColumnToContents(eColQuality);
    m_tableWidget->resizeColumnToContents(eColCount);

    m_tableWidget->setColumnHidden(eColCount, true);

    // --------------------------------------------------------------------

    m_mainLayout->addWidget(m_topControlsContainerWidget);
    m_mainLayout->addWidget(m_tableWidget);

    // --------------------------------------------------------------------
    // Shortcuts: Whole Window

    QShortcut *WindowShortcutN = new QShortcut(QKeySequence("Ctrl+N"), this);
    WindowShortcutN->setContext(Qt::WindowShortcut);
    connect(WindowShortcutN, &QShortcut::activated, this, &MainWindow::action_EditSettingsFile);

    // --------------------------------------------------------------------
    // Context menu Actions (m_tableWidget)

    m_actionListViewOpenFiles = new QAction(tr("Open"), this);
    //m_actionListViewOpenFiles->setShortcut(QKeySequence("Ctrl+O"));
    //m_actionListViewOpenFiles->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewOpenFiles);
    connect(m_actionListViewOpenFiles, &QAction::triggered, this, &MainWindow::action_ListViewOpenFiles);

    m_actionListViewEditFiles = new QAction(tr("Edit"), this);
    m_actionListViewEditFiles->setShortcut(QKeySequence("Ctrl+E"));
    m_actionListViewEditFiles->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewEditFiles);
    connect(m_actionListViewEditFiles, &QAction::triggered, this, &MainWindow::action_ListViewEditFiles);

    m_actionListViewBrowseToFile = new QAction(tr("Show in folder"),this);
    m_actionListViewBrowseToFile->setIcon(QIcon::fromTheme("folder-open"));
    m_actionListViewBrowseToFile->setShortcut(QKeySequence("Ctrl+L"));
    m_actionListViewBrowseToFile->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewBrowseToFile);
    connect(m_actionListViewBrowseToFile, &QAction::triggered, this, &MainWindow::action_ListViewBrowseToFile);

    m_actionListViewCopyPaths = new QAction(tr("Copy Path"), this);
    m_actionListViewCopyPaths->setIcon(QIcon::fromTheme("edit-copy-path"));
    m_actionListViewCopyPaths->setShortcut(QKeySequence("Ctrl+Shift+C"));
    m_actionListViewCopyPaths->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewCopyPaths);
    connect(m_actionListViewCopyPaths, &QAction::triggered, this, &MainWindow::action_ListViewCopyPaths);

    m_actionListViewCutFiles = new QAction(tr("Cut"), this);
    m_actionListViewCutFiles->setIcon(QIcon::fromTheme("edit-cut"));
    m_actionListViewCutFiles->setShortcut(QKeySequence("Ctrl+X"));
    m_actionListViewCutFiles->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewCutFiles);
    connect(m_actionListViewCutFiles, &QAction::triggered, this, &MainWindow::action_ListViewCutFiles);

    m_actionListViewCopyFiles = new QAction(tr("Copy"), this);
    m_actionListViewCopyFiles->setIcon(QIcon::fromTheme("edit-copy"));
    m_actionListViewCopyFiles->setShortcut(QKeySequence("Ctrl+C"));
    m_actionListViewCopyFiles->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewCopyFiles);
    connect(m_actionListViewCopyFiles, &QAction::triggered, this, &MainWindow::action_ListViewCopyFiles);

    m_actionListViewDeleteFiles = new QAction(tr("Delete"), this);
    m_actionListViewDeleteFiles->setIcon(QIcon::fromTheme("edit-delete"));
    m_actionListViewDeleteFiles->setShortcut(QKeySequence::Delete);
    m_actionListViewDeleteFiles->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewDeleteFiles);
    connect(m_actionListViewDeleteFiles, &QAction::triggered, this, [this]() { action_ListViewDeleteFiles(true); });

    m_actionListViewRenameFiles = new QAction(tr("Rename"), this);
    m_actionListViewRenameFiles->setIcon(QIcon::fromTheme("edit-rename"));
    m_actionListViewRenameFiles->setShortcut(QKeySequence(Qt::Key_F2));
    m_actionListViewRenameFiles->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewRenameFiles);
    connect(m_actionListViewRenameFiles, &QAction::triggered, this, &MainWindow::action_ListViewRenameFiles);

    m_actionListViewFileProperties = new QAction(tr("Properties"), this);
    m_actionListViewFileProperties->setIcon(QIcon::fromTheme("document-properties"));
    m_actionListViewFileProperties->setShortcut(QKeySequence("Ctrl+I"));
    m_actionListViewFileProperties->setShortcutContext(Qt::WidgetShortcut);
    m_tableWidget->addAction(m_actionListViewFileProperties);
    connect(m_actionListViewFileProperties, &QAction::triggered, this, &MainWindow::action_ListViewFileProperties);

    if (m_settings.showIconsInMenu == false) {
        m_actionListViewOpenFiles->setIconVisibleInMenu(false);
        m_actionListViewEditFiles->setIconVisibleInMenu(false);
        m_actionListViewBrowseToFile->setIconVisibleInMenu(false);
        m_actionListViewCopyPaths->setIconVisibleInMenu(false);
        m_actionListViewCutFiles->setIconVisibleInMenu(false);
        m_actionListViewCopyFiles->setIconVisibleInMenu(false);
        m_actionListViewDeleteFiles->setIconVisibleInMenu(false);
        m_actionListViewRenameFiles->setIconVisibleInMenu(false);
        m_actionListViewFileProperties->setIconVisibleInMenu(false);
    }

    if (m_settings.showShortcutsInMenu == false) {
        m_actionListViewOpenFiles->setShortcutVisibleInContextMenu(false);
        m_actionListViewEditFiles->setShortcutVisibleInContextMenu(false);
        m_actionListViewBrowseToFile->setShortcutVisibleInContextMenu(false);
        m_actionListViewCopyPaths->setShortcutVisibleInContextMenu(false);
        m_actionListViewCutFiles->setShortcutVisibleInContextMenu(false);
        m_actionListViewCopyFiles->setShortcutVisibleInContextMenu(false);
        m_actionListViewDeleteFiles->setShortcutVisibleInContextMenu(false);
        m_actionListViewRenameFiles->setShortcutVisibleInContextMenu(false);
        m_actionListViewFileProperties->setShortcutVisibleInContextMenu(false);
    }

    // --------------------------------------------------------------------

#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    loadMimeCache();
#endif

    // --------------------------------------------------------------------
    // Shorcuts: m_tableWidget

    QShortcut *ListViewShortcutHome = new QShortcut(QKeySequence(Qt::Key_Home), m_tableWidget);
    connect(ListViewShortcutHome, &QShortcut::activated, this, [this]() { if (m_tableWidget->rowCount() > 0) { m_tableWidget->setCurrentCell(0, 0); m_tableWidget->scrollToTop(); }});

    QShortcut *ListViewShortcutEnd = new QShortcut(QKeySequence(Qt::Key_End), m_tableWidget);
    connect(ListViewShortcutEnd, &QShortcut::activated, this, [this]() { int lastRow = m_tableWidget->rowCount() - 1; if (lastRow >= 0) { m_tableWidget->setCurrentCell(lastRow, 0);m_tableWidget->scrollToBottom(); }});

    QShortcut *ListViewShortcutShiftDel = new QShortcut(QKeySequence("Shift+Delete"), m_tableWidget);
    ListViewShortcutShiftDel->setContext(Qt::WidgetShortcut);
    connect(ListViewShortcutShiftDel, &QShortcut::activated, this,  [this]() { action_ListViewDeleteFiles(false); });

    // --------------------------------------------------------------------
    // Shorcuts: InputBoxes

    QShortcut *InputBox1ShortcutR = new QShortcut(QKeySequence("Ctrl+R"), m_LineEdit1);
    InputBox1ShortcutR->setContext(Qt::WidgetShortcut);
    connect(InputBox1ShortcutR, &QShortcut::activated, this, [this]() { m_CheckboxRegExName->toggle(); });

    QShortcut *InputBox1ShortcutD = new QShortcut(QKeySequence("Ctrl+D"), m_LineEdit1);
    InputBox1ShortcutD->setContext(Qt::WidgetShortcut);
    connect(InputBox1ShortcutD, &QShortcut::activated, this, [this]() { m_CheckboxDirectories->toggle(); });

    QShortcut *InputBox2ShortcutR = new QShortcut(QKeySequence("Ctrl+R"), m_LineEdit2);
    InputBox2ShortcutR->setContext(Qt::WidgetShortcut);
    connect(InputBox2ShortcutR, &QShortcut::activated, this, [this]() { m_CheckboxRegExContent->toggle(); });

    QShortcut *InputBox1ShortcutBackSpace = new QShortcut(QKeySequence("Ctrl+Backspace"), m_LineEdit1);
    InputBox1ShortcutBackSpace->setContext(Qt::WidgetShortcut);
    connect(InputBox1ShortcutBackSpace, &QShortcut::activated, this, [this]() { m_currentSearchGeneration++; m_tableWidget->setRowCount(0); m_LineEdit2->clear(); m_LineEdit1->clear(); setWindowTitle(QDir::toNativeSeparators(m_currentDirectory)); m_LineEdit1->setFocus(); });

    QShortcut *InputBox2ShortcutBackSpace = new QShortcut(QKeySequence("Ctrl+Backspace"), m_LineEdit2);
    InputBox2ShortcutBackSpace->setContext(Qt::WidgetShortcut);
    connect(InputBox2ShortcutBackSpace, &QShortcut::activated, this, [this]() { m_currentSearchGeneration++; m_tableWidget->setRowCount(0); m_LineEdit2->clear(); m_LineEdit1->clear(); setWindowTitle(QDir::toNativeSeparators(m_currentDirectory)); m_LineEdit1->setFocus(); });


    // --------------------------------------------------------------------
    // Shorcuts: CheckBoxes

    QShortcut *CheckboxRegExFileNameEnter = new QShortcut(QKeySequence("Enter"), m_CheckboxRegExName);
    CheckboxRegExFileNameEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExFileNameEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxRegExFileNameReturn = new QShortcut(QKeySequence("Return"), m_CheckboxRegExName);
    CheckboxRegExFileNameReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExFileNameReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxRegExContentEnter = new QShortcut(QKeySequence("Enter"), m_CheckboxRegExContent);
    CheckboxRegExContentEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExContentEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxRegExContentReturn = new QShortcut(QKeySequence("Return"), m_CheckboxRegExContent);
    CheckboxRegExContentReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExContentReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxNameCaseSenseEnter = new QShortcut(QKeySequence("Enter"), m_CheckboxNameCaseSense);
    CheckboxNameCaseSenseEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxNameCaseSenseEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxNameCaseSenseReturn = new QShortcut(QKeySequence("Return"), m_CheckboxNameCaseSense);
    CheckboxNameCaseSenseReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxNameCaseSenseReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxContentCaseSenseEnter = new QShortcut(QKeySequence("Enter"), m_CheckboxContentCaseSense);
    CheckboxContentCaseSenseEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxContentCaseSenseEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxContentCaseSenseReturn = new QShortcut(QKeySequence("Return"), m_CheckboxContentCaseSense);
    CheckboxContentCaseSenseReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxContentCaseSenseReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxDirectoriesEnter = new QShortcut(QKeySequence("Enter"), m_CheckboxDirectories);
    CheckboxDirectoriesEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxDirectoriesEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxDirectoriesReturn = new QShortcut(QKeySequence("Return"), m_CheckboxDirectories);
    CheckboxDirectoriesReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxDirectoriesReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxCRCEnter = new QShortcut(QKeySequence("Enter"), m_CheckboxCRC);
    CheckboxCRCEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxCRCEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxCRCReturn = new QShortcut(QKeySequence("Return"), m_CheckboxCRC);
    CheckboxCRCReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxCRCReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // --------------------------------------------------------------------

    qApp->installEventFilter(this);

    // --------------------------------------------------------------------

    m_timerCalcCrc = new QTimer(this);
    m_timerCalcCrc->setSingleShot(true);
    connect(m_timerCalcCrc, &QTimer::timeout, this, &MainWindow::onTimedCalcCRC);

    m_timerUpdateIcons = new QTimer(this);
    m_timerUpdateIcons->setSingleShot(true);
    connect(m_timerUpdateIcons, &QTimer::timeout, this, &MainWindow::onTimedUpdateIcons);

    connect(m_LineEdit1, &QLineEdit::returnPressed, this, &MainWindow::startSearch);
    connect(m_LineEdit2, &QLineEdit::returnPressed, this, &MainWindow::startSearch);
    connect(m_LineEdit1, &QLineEdit::textChanged, this, &MainWindow::validateInputBoxRegex);
    connect(m_LineEdit2, &QLineEdit::textChanged, this, &MainWindow::validateInputBoxRegex);
    connect(m_CheckboxRegExName, &QCheckBox::checkStateChanged, this, &MainWindow::validateInputBoxRegex);
    connect(m_CheckboxRegExContent, &QCheckBox::checkStateChanged, this, &MainWindow::validateInputBoxRegex);
    connect(m_tableWidget, &Custom_QTableWidget::itemChanged, this, &MainWindow::onItemChanged);
    connect(m_tableWidget, &Custom_QTableWidget::customContextMenuRequested, this, &MainWindow::onShowContextMenu);
    connect(m_tableWidget, &Custom_QTableWidget::itemDoubleClicked, this, &MainWindow::onListViewItemDoubleClicked);
    connect(m_CheckboxCRC, &QCheckBox::checkStateChanged, this, &MainWindow::onCheckboxClickedCRC);
    connect(m_CheckboxRegExName, &QCheckBox::checkStateChanged, this, &MainWindow::onCheckboxClickedRegExName);
    connect(m_CheckboxRegExContent, &QCheckBox::checkStateChanged, this, &MainWindow::onCheckboxClickedRegExContent);
    connect(m_tableWidget->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onVerticalBarScrollChange);
    connect(m_tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &MainWindow::onListViewHeaderClicked);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::onClipboardChanged);
}

MainWindow::~MainWindow() = default;

void MainWindow::onCheckboxClickedRegExName(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        m_LineEdit1->setPlaceholderText(tr("(filename search terms)"));
    } else {
        m_LineEdit1->setPlaceholderText(tr("(filename regex expression)"));
    }
}

void MainWindow::onCheckboxClickedRegExContent(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        m_LineEdit2->setPlaceholderText(tr("(content search terms)"));
    } else {
        m_LineEdit2->setPlaceholderText(tr("(content regex expression)"));
    }
}

void MainWindow::onCheckboxClickedCRC(Qt::CheckState state) {
    m_tableWidget->setColumnHidden(eColCRC, !state);
    m_timerCalcCrc->start(100);
}

void MainWindow::onVerticalBarScrollChange() {
    m_timerCalcCrc->start(100);
    m_timerUpdateIcons->start(100);
}

void MainWindow::onListViewHeaderClicked() {
    m_timerCalcCrc->start(100);
    m_timerUpdateIcons->start(100);
}
void MainWindow::validateInputBoxRegex() {
    QString InputBox1Text = m_LineEdit1->text();
    QString InputBox2Text = m_LineEdit2->text();

    if (InputBox1Text.isEmpty()) {
        m_LineEdit1->setStyleSheet("");
    } else {
        if (m_CheckboxRegExName->isChecked()) {
            QRegularExpression re1(InputBox1Text);
            if (!re1.isValid()) {
                m_LineEdit1->setStyleSheet("background-color: red; color: white;");
            } else {
                m_LineEdit1->setStyleSheet("");
            }
        } else {
            m_LineEdit1->setStyleSheet("");
        }
    }

    if (InputBox2Text.isEmpty()) {
        m_LineEdit2->setStyleSheet("");
    } else {
        if (m_CheckboxRegExContent->isChecked()) {
            QRegularExpression re2(InputBox2Text);
            if (!re2.isValid()) {
                m_LineEdit2->setStyleSheet("background-color: red; color: white;");
            } else {
                m_LineEdit2->setStyleSheet("");
            }
        } else {
            m_LineEdit2->setStyleSheet("");
        }
    }
}

void MainWindow::onTimedCalcCRC() {
    if (m_bSearchActive.load() || (m_tableWidget->rowCount() == 0)) {
        return;
    }

    int firstVisible = m_tableWidget->rowAt(0);
    int lastVisible = m_tableWidget->rowAt(m_tableWidget->viewport()->height() - 1);    // Substract 1 pixel to make sure we're in the viewport
    if (lastVisible == -1) {
        lastVisible = m_tableWidget->rowCount() - 1;
    }
    if ((firstVisible == -1) || (lastVisible == -1)) {
        return;
    }

    if (m_CheckboxCRC->isChecked()) {
        for (int i = firstVisible; i <= lastVisible; ++i) {
            QTableWidgetItem *crcItem = m_tableWidget->item(i, eColCRC);
            QTableWidgetItem *nameItem = m_tableWidget->item(i, eColName);

            if (crcItem && crcItem->text().isEmpty() && nameItem) {
                QString fullPath = nameItem->data(Qt::UserRole).toString();

                if (!fullPath.isEmpty()) {
                    crcItem->setText("TBD");
                    int searchIdForThisThread = m_currentSearchGeneration.load();
                    auto watcher = new QFutureWatcher<QString>(this);

                    connect(watcher, &QFutureWatcher<QString>::finished, [this, watcher, crcItem, fullPath, searchIdForThisThread]() {
                        if (searchIdForThisThread != m_currentSearchGeneration.load()) {
                            watcher->deleteLater();
                            return; // Abort! Do NOT touch crcItem - it's dead!
                        }

                        // Check with crcItem->m_tableWidget() if crcItem still lives within a table
                        if (crcItem->tableWidget() != nullptr) {
                            int currentRow = crcItem->row(); // Get items's current row (works even after changing sort order)
                            QTableWidgetItem *currentNameItem = m_tableWidget->item(currentRow, eColName);

                            if (currentNameItem && currentNameItem->data(Qt::UserRole).toString() == fullPath) {
                                crcItem->setText(watcher->result());
                            }
                        }
                        watcher->deleteLater();
                    });

                    QFuture<QString> future = QtConcurrent::run([fullPath]() {
                        quint32 crc = calculateCRC32(fullPath);
                        return QString("%1").arg(crc, 8, 16, QChar('0')).toUpper();
                    });
                    watcher->setFuture(future);
                }
            }
        }
    } else {
        for (int i = firstVisible; i <= lastVisible; ++i) {
            QTableWidgetItem *crcItem = m_tableWidget->item(i, eColCRC);
            if (crcItem && !crcItem->text().isEmpty()) {
                crcItem->setText("");
            }
        }
    }
}

void MainWindow::startSearch() {
    QString InputBox1Text = m_LineEdit1->text();
    QString InputBox2Text = m_LineEdit2->text();
    bool bRegExFilename = m_CheckboxRegExName->isChecked();
    bool bRegExContent = m_CheckboxRegExContent->isChecked();

    if (m_bSearchActive) {
        return;
    }

    if (InputBox1Text.trimmed().isEmpty() && InputBox2Text.trimmed().isEmpty()) {
        m_currentSearchGeneration++; // Make old crc calc threads invalid to prevent seg faults from Use-After-Free
        m_tableWidget->setRowCount(0);
        m_bSearchActive = false;
        return;
    }

    if (bRegExFilename && !InputBox1Text.isEmpty()) {
        QRegularExpression qreFileName(InputBox1Text);
        if (!qreFileName.isValid()) {
            return;
        }
    }

    if (bRegExContent && !InputBox2Text.isEmpty()) {
        QRegularExpression qreContent(InputBox2Text);
        if (!qreContent.isValid()) {
            return;
        }
    }

    m_bSearchActive = true;
    m_bAbortRequested = false;


    m_BenchmarkTimer.start();

    m_workerHasFinished = false;
    m_isProcessingPending = false;

    m_SearchStats_bSearchInterrupted = false;
    m_SearchStats_iItemsFound = 0;
    m_SearchStats_iNameMatched = 0;
    m_SearchStats_iContentMatched = 0;

    m_lastWidget = QApplication::focusWidget();
    // Reset focus to lineEdit widget of the same row after search
    if (m_lastWidget == m_CheckboxRegExName || m_lastWidget == m_CheckboxNameCaseSense || m_lastWidget == m_CheckboxDirectories) {
        m_lastWidget = m_LineEdit1;
    } else if (m_lastWidget == m_CheckboxRegExContent || m_lastWidget == m_CheckboxContentCaseSense || m_lastWidget == m_CheckboxCRC) {
        m_lastWidget = m_LineEdit2;
    }

    m_topControlsContainerWidget->setEnabled(false);
    m_tableWidget->setEnabled(false);

    m_topControlsContainerWidget->repaint();
    m_tableWidget->repaint();

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    setWindowTitle(QDir::toNativeSeparators(m_currentDirectory) + "   (Searching)");
    m_currentSearchGeneration++; // Make old crc calc threads invalid to prevent seg faults from Use-After-Free

    m_tableWidget->setUpdatesEnabled(false);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // Important!! Calculations of header don't get stopped by "m_tableWidget->setUpdatesEnabled(false)"
    m_tableWidget->setRowCount(0);
    m_tableWidget->setSortingEnabled(false);
    m_tableWidget->blockSignals(true);  // block "itemChanged" signals

    QThread* m_workerThread = new QThread;
    SearchWorker* worker = new SearchWorker(m_currentDirectory, InputBox1Text, InputBox2Text, bRegExFilename, bRegExContent, m_CheckboxNameCaseSense->isChecked(), m_CheckboxContentCaseSense->isChecked(), m_CheckboxDirectories->checkState(), m_settings.textExts);
    worker->moveToThread(m_workerThread);

    // Verbindungen
    connect(m_workerThread, &QThread::started, worker, &SearchWorker::process);
    connect(worker, &SearchWorker::filesFoundBatch, this, &MainWindow::onWorkerSentBatch);
    connect(worker, &SearchWorker::searchStats, this, &MainWindow::onWorkerFinished);   // 1. Store result info
    connect(worker, &SearchWorker::finished, m_workerThread, &QThread::quit);           // 2. Stop m_workerThread
    connect(worker, &SearchWorker::finished, worker, &SearchWorker::deleteLater);       // 3. Clean up object
    connect(m_workerThread, &QThread::finished, m_workerThread, &QThread::deleteLater); // 4. Clean up m_workerThread

    connect(this, &MainWindow::abortSearchWorkerRequested, worker, &SearchWorker::abort, Qt::DirectConnection);   // React to "Escape" key press

    m_workerThread->start();
}

void MainWindow::onWorkerSentBatch(const QList<SearchResult> &batch) {
    m_pendingBatches.enqueue(batch);

    // Wenn wir schon verarbeiten, macht die laufende Schleife weiter.
    if (m_isProcessingPending) return;

    m_isProcessingPending = true;
    processNextBatch(); // Wir starten die Kette
}

void MainWindow::processNextBatch() {
    if (m_pendingBatches.isEmpty()) {
        m_isProcessingPending = false;

        if (m_workerHasFinished) {
            finalizeUI();
        }
        return;
    }

    if (m_bAbortRequested) {
        m_pendingBatches.clear();
        m_isProcessingPending = false;

        if (m_workerHasFinished) {
            finalizeUI();
        }
        return;
    }

    // Einen Batch verarbeiten
    QList<SearchResult> currentBatch = m_pendingBatches.dequeue();
    int currentRows = m_tableWidget->rowCount();
    m_tableWidget->setRowCount(currentRows + currentBatch.size());

    for (int i = 0; i < currentBatch.size(); ++i) {
        addFileToTable(currentBatch.at(i).fileInfo, currentRows + i, currentBatch.at(i).iLenRem, currentBatch.at(i).nameMatchQuality, currentBatch.at(i).contentMatchCount);
    }

    // Der Clou: Wir planen den nächsten Batch für "sofort, wenn Zeit ist"
    // Das verhindert den "Wiedereintritt"-Fehler (Reentrancy)
    QTimer::singleShot(0, this, &MainWindow::processNextBatch);
}


void MainWindow::onWorkerFinished(uint iItemsFound, uint iNameMatched, uint iContentMatched, bool bSearchInterrupted) {
    m_SearchStats_iItemsFound = iItemsFound;
    m_SearchStats_iNameMatched = iNameMatched;
    m_SearchStats_iContentMatched = iContentMatched;
    m_SearchStats_bSearchInterrupted = bSearchInterrupted;
    m_workerHasFinished = true;
    // Nur wenn gerade KEIN Batch mehr verarbeitet wird,
    // müssen wir hier den Abschluss triggern.
    if (!m_isProcessingPending) {
        finalizeUI();
    }
}


void MainWindow::finalizeUI() {
    qDebug() << "finalizeUI() entry point. m_BenchmarkTimer:" << m_BenchmarkTimer.elapsed() << " ms elapsed since start of search.  m_SearchStats_bSearchInterrupted =" << m_SearchStats_bSearchInterrupted << "  m_bAbortRequested = " << m_bAbortRequested;

    if (m_SearchStats_iItemsFound == 0 || m_SearchStats_bSearchInterrupted == true || m_bAbortRequested) {
        m_currentSearchGeneration++; // Make old crc calc threads invalid to prevent seg faults from Use-After-Free
        m_tableWidget->setRowCount(0);
    }

    m_tableWidget->blockSignals(false);

    if (m_SearchStats_iContentMatched > 0) {
        m_tableWidget->setColumnHidden(eColQuality, true);
        m_tableWidget->setColumnHidden(eColCount, false);
        m_tableWidget->sortByColumn(eColCount, Qt::DescendingOrder);
    } else {
        m_tableWidget->setColumnHidden(eColCount, true);
        m_tableWidget->setColumnHidden(eColQuality, false);
        m_tableWidget->sortByColumn(eColQuality, Qt::AscendingOrder);
    }

    m_tableWidget->setSortingEnabled(true);
    updateColumns();
    m_tableWidget->setUpdatesEnabled(true);



    QString titleString;
    QString InputBox2Text = m_LineEdit2->text();

    if (m_SearchStats_bSearchInterrupted == true || m_bAbortRequested) {
        titleString = QString(tr("Search aborted..."));
    } else if (InputBox2Text.trimmed().isEmpty()) {
        titleString = QString(tr("%1 (%2 hits in %3 items)")).arg(QDir::toNativeSeparators(m_currentDirectory), QLocale::system().toString(m_SearchStats_iNameMatched), QLocale::system().toString(m_SearchStats_iItemsFound));
    } else {
        titleString = QString(tr("%1 (%2 matches spread across %3 files of %4 searched)")).arg(QDir::toNativeSeparators(m_currentDirectory), QLocale::system().toString(m_SearchStats_iContentMatched), QLocale::system().toString(m_SearchStats_iNameMatched), QLocale::system().toString(m_SearchStats_iItemsFound));
    }

    setWindowTitle(titleString);

    m_topControlsContainerWidget->setEnabled(true);
    m_tableWidget->setEnabled(true);

    QGuiApplication::restoreOverrideCursor();

    m_bAbortRequested = false;
    m_bSearchActive = false;

    if (m_lastWidget) {
        m_lastWidget->setFocus();
    }

    qDebug() << "m_BenchmarkTimer:" << m_BenchmarkTimer.elapsed() << " ms elapsed since start of search for " << titleString;
    m_timerCalcCrc->start(100);
    m_timerUpdateIcons->start(100);
}

void MainWindow::updateColumns() {
    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColSize,    QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColDate,    QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColType,    QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColQuality, QHeaderView::ResizeToContents);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColCount,   QHeaderView::ResizeToContents);
    //m_tableWidget->horizontalHeader()->setSectionResizeMode(eColCRC,   QHeaderView::ResizeToContents);

    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColName, QHeaderView::Stretch);
    m_tableWidget->horizontalHeader()->setSectionResizeMode(eColPath, QHeaderView::Stretch);

    m_tableWidget->horizontalHeader()->doItemsLayout();

    int eColNameWidth = m_tableWidget->columnWidth(eColName);
    int eColPathWidth = m_tableWidget->columnWidth(eColPath);

    m_tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    m_tableWidget->setColumnWidth(eColName, eColNameWidth);
    m_tableWidget->setColumnWidth(eColPath, eColPathWidth);
}

void MainWindow::onItemChanged(QTableWidgetItem *item) {
    if (item->column() != eColName) return;

    QString oldPath = item->data(Qt::UserRole).toString();
    if (oldPath.isEmpty()) return;

    QString originalInput = item->text();
    QString cleanedName = cleanFileName(originalInput);

    QFileInfo oldInfo(oldPath);
    QString newPath = oldInfo.absolutePath() + "/" + cleanedName;

    if (oldPath != newPath) {
        if (QFile::rename(oldPath, newPath)) {
            QSignalBlocker blocker(item->tableWidget());
            item->setText(cleanedName);
            item->setData(Qt::UserRole, newPath); // Pfad im UserRole Bereich des Items aktualisieren
        } else {
            QMessageBox::critical(this, "Fehler", "Umbenennen fehlgeschlagen.");
            QSignalBlocker blocker(item->tableWidget());
            item->setText(oldInfo.fileName()); // Text zurücksetzen
        }
    } else if (cleanedName != originalInput) {
        QSignalBlocker blocker(item->tableWidget());
        item->setText(cleanedName);
    }
}

void MainWindow::onShowContextMenu(const QPoint &pos) {
    QTableWidgetItem *item = m_tableWidget->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QString filePath = m_tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
    QFileInfo fileInfo(filePath);
    QString fileExt = fileInfo.suffix().toLower();

    QMenu mainMenu(this);

    mainMenu.addAction(m_actionListViewOpenFiles);
    mainMenu.setDefaultAction(m_actionListViewOpenFiles);

    if (m_settings.audioExts.contains(fileExt) || m_settings.imageExts.contains(fileExt) || m_settings.textExts.contains(fileExt) || m_settings.videoExts.contains(fileExt)) {
        mainMenu.addAction(m_actionListViewEditFiles);
    }

#ifdef Q_OS_WIN
    mainMenu.addSeparator(); //-----------------------------------------

    QDir sendToDir(getSendToPath());
    if (sendToDir.exists()) {
        QMenu *sendToMenu = mainMenu.addMenu(tr("Send to"));
        QFileInfoList shortcuts = sendToDir.entryInfoList({"*.lnk"}, QDir::Files);
        for (const QFileInfo &shortcutInfo : std::as_const(shortcuts)) {
            QString displayName = shortcutInfo.completeBaseName();

            QIcon cleanIcon;
            QString targetPath = shortcutInfo.symLinkTarget();
            if (!targetPath.isEmpty() && QFileInfo::exists(targetPath)) {
                cleanIcon = m_iconProvider.icon(QFileInfo(targetPath));
            } else {
                cleanIcon = m_iconProvider.icon(shortcutInfo);
            }

            QPixmap pix = cleanIcon.pixmap(16, 16);

            QAction *sendAction = sendToMenu->addAction(QIcon(pix), displayName);
            sendAction->setData(shortcutInfo.absoluteFilePath());	// store lnk path inside sendAction object

            connect(sendAction, &QAction::triggered, this, [this, shortcutInfo]() {
                QStringList pathList = getTablePathList();
                if (pathList.isEmpty()) return;

                QString targetPath = shortcutInfo.symLinkTarget();

                if (!targetPath.isEmpty() && QFileInfo::exists(targetPath)) {
                    QStringList args;
                    for (const QString &p : std::as_const(pathList)) {
                        args << QDir::toNativeSeparators(p);
                    }

                    QProcess::startDetached(targetPath, args);
                }
                else {
                    QString allParams;
                    for (const QString &p : std::as_const(pathList)) {
                        if (!allParams.isEmpty()) allParams += " ";
                        allParams += "\"" + QDir::toNativeSeparators(p) + "\"";
                    }

                    ShellExecuteW(nullptr, L"open",
                                  reinterpret_cast<const wchar_t*>(shortcutInfo.absoluteFilePath().utf16()),
                                  reinterpret_cast<const wchar_t*>(allParams.utf16()),
                                  nullptr, SW_SHOWNORMAL);
                }
            });
        }
    }
#else
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForFile(filePath);
    QString mimeName = mime.name();

    QStringList appIds = m_mimeCache.value(mime.name());
    if (appIds.isEmpty()) {
        for (const QString &parent : mime.allAncestors()) {
            appIds = m_mimeCache.value(parent);
            if (!appIds.isEmpty()) break;
        }
    }

    if (!appIds.isEmpty()) {
        mainMenu.addSeparator(); //-----------------------------------------

        QMenu *openWithMenu = mainMenu.addMenu(tr("Open with"));
        if (m_settings.showIconsInMenu == true) {
            openWithMenu->setIcon(QIcon::fromTheme("system-run"));
        }
        for (const QString &id : std::as_const(appIds)) {
            DesktopEntry info = getDesktopEntryById(id);
            if (info.isValid) {
                QAction *action = openWithMenu->addAction(QIcon::fromTheme(info.icon), info.name);
                connect(action, &QAction::triggered, [info, filePath, this]() {
                    openFileListWithHandler(info.id, getTablePathList());
                });
            }
        }
    }
#endif

    mainMenu.addSeparator(); //-----------------------------------------
    mainMenu.addAction(m_actionListViewBrowseToFile);
    mainMenu.addAction(m_actionListViewCopyPaths);
    mainMenu.addAction(m_actionListViewCutFiles);
    mainMenu.addAction(m_actionListViewCopyFiles);
    mainMenu.addAction(m_actionListViewDeleteFiles);
    mainMenu.addAction(m_actionListViewRenameFiles);
    mainMenu.addSeparator(); //-----------------------------------------
    mainMenu.addAction(m_actionListViewFileProperties);

    mainMenu.exec(m_tableWidget->viewport()->mapToGlobal(pos));
}

// ---------------------------------------------------------------------------------------------
// Actions

QStringList MainWindow::getTablePathList() {
    QStringList pathList;

    QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
    if (selectedItems.isEmpty()) return pathList;

    // Zeilenindizes sammeln (verhindert Dopplungen bei Mehrfachauswahl in einer Zeile)
    QSet<int> rowSet;
    for (auto *item : std::as_const(selectedItems)) {
        rowSet.insert(item->row());
    }

    for (int row : rowSet) {
        // Wir nehmen an, der Pfad liegt in Spalte 0 in der UserRole
        QString path = m_tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            pathList << path;
        }
    }

    return pathList;
}

void MainWindow::onListViewItemDoubleClicked(QTableWidgetItem *item) {
    action_ListViewOpenFiles();
}

void MainWindow::action_ListViewOpenFiles() {
    //QStringList pathList = getTablePathList();
    QTableWidgetItem *item = m_tableWidget->currentItem();
    if (!item) return;

    QString path = m_tableWidget->item(item->row(), eColName)->data(Qt::UserRole).toString();
    QFileInfo fileInfo(path);

    if (!fileInfo.exists()) {
        return;
    }

    QString fileExt = fileInfo.suffix().toLower();

    if (fileExt == "desktop") {
        launchDesktopFile(getDesktopEntry(fileInfo));
#if defined(Q_OS_LINUX) || defined(Q_OS_UNIX)
    } else if (fileInfo.isExecutable() && !fileInfo.isDir() && !m_settings.audioExts.contains(fileExt) && !m_settings.imageExts.contains(fileExt) && !m_settings.videoExts.contains(fileExt)) {
        // Workaround on linux where executible files are not neccessarily executed when opened via QDesktopServices::openUrl().
        QProcess::startDetached(path, QStringList(), fileInfo.absolutePath());
#endif
    } else {
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void MainWindow::action_ListViewEditFiles() {

    QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    // Zeilenindizes sammeln (verhindert Dopplungen bei Mehrfachauswahl in einer Zeile)
    // (Die Funktion selectedItems() gibt stumpf jedes einzelne Item (jede Zelle) zurück, das gerade farblich hinterlegt ist.)
    QSet<int> rowSet;
    for (auto *item : std::as_const(selectedItems)) {
        rowSet.insert(item->row());
    }

    QStringList pathListAudio;
    QStringList pathListImage;
    QStringList pathListText;
    QStringList pathListVideo;

    for (int row : rowSet) {
        // Der Pfad liegt in Spalte 0 in der UserRole
        QString fullPath = m_tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
        if (fullPath.isEmpty()) {
            continue;
        }

        QFileInfo fileInfo(fullPath);
        QString fileExt = fileInfo.suffix().toLower();
        if (fileExt.isEmpty()) {
            continue;
        }

        if (m_settings.audioExts.contains(fileExt)) {
            pathListAudio << fullPath;
        } else if (m_settings.imageExts.contains(fileExt)) {
            pathListImage << fullPath;
        } else if (m_settings.textExts.contains(fileExt)) {
            pathListText << fullPath;
        } else if (m_settings.videoExts.contains(fileExt)) {
            pathListVideo << fullPath;
        }
    }

    if (!pathListAudio.isEmpty()) {
        openFileListWithHandler(m_settings.audioEditor, pathListAudio);
    }

    if (!pathListImage.isEmpty()) {
        openFileListWithHandler(m_settings.imageEditor, pathListImage);
    }

    if (!pathListText.isEmpty()) {
        openFileListWithHandler(m_settings.textEditor, pathListText);
    }

    if (!pathListVideo.isEmpty()) {
        openFileListWithHandler(m_settings.videoEditor, pathListVideo);
    }
}

void MainWindow::action_ListViewCopyPaths() {
    QStringList pathList = getTablePathList();

    QStringList pathListNative;
    for (const QString &path : std::as_const(pathList)) {
        pathListNative << QDir::toNativeSeparators(path);
    }

#ifdef Q_OS_WIN
    QString sClipboardList = pathListNative.join("\r\n");
#else
    QString sClipboardList = pathListNative.join("\n");
#endif

    QGuiApplication::clipboard()->setText(sClipboardList);
}

void MainWindow::action_ListViewDeleteFiles(bool bRecycleOnly) {
    QList<QTableWidgetItem*> selected = m_tableWidget->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    //----------------------------------------------------------------------------------------------
    // 1. Collect unique rows
    QSet<int> rowSet;
    for (auto *item : std::as_const(selected)) {
        rowSet.insert(item->row());
    }

    QString sSingleFilePath = "";
    if (rowSet.size() == 1) {
        int firstElement = rowSet.values().at(0);
        sSingleFilePath = m_tableWidget->item(firstElement, eColName)->data(Qt::UserRole).toString();
        if (sSingleFilePath.isEmpty()) {
            return;
        }

        if (!QFile::exists(sSingleFilePath)) {
            return;
        }
    }

    //----------------------------------------------------------------------------------------------
    // 2. Prepare dialog

    QString sTitle;
    QString sText;
    QString sWarning;
    QMessageBox::Icon iIcon;

    if (bRecycleOnly) {
        iIcon = QMessageBox::Question;
        sWarning = "";
        if (rowSet.size() == 1) {
            sTitle = tr("Delete File");
            sText = tr("Do you really want to move this file into the recycle bin?");
        } else {
            sTitle = tr("Delete multiple elements");
            sText = QString(tr("Do you really want to move these %1 files into the recylce bin?")).arg(rowSet.size());
        }
    } else {
        iIcon = QMessageBox::Warning;
        sWarning = "<p style='color: red;'><i>" + tr("This process cannot be undone.") + "</i></p>";
        if (rowSet.size() == 1) {
            sTitle = tr("Delete File");
            sText = tr("Are you sure you want to delete this file permanently?");
        } else {
            sTitle = tr("Delete multiple elements");
            sText = QString(tr("Are you sure you want to delete these %1 files permanently?")).arg(rowSet.size());
        }
    }

    //----------------------------------------------------------------------------------------------
    // 3. Show dialog

    QMessageBox msgBox(this);
    msgBox.setWindowTitle(sTitle);
    msgBox.setIcon(iIcon);

    if (rowSet.size() == 1) {
        QFileInfo fileInfo(sSingleFilePath);
        QString fileName = fileInfo.fileName();
        QString size = formatAdaptiveSize(fileInfo.size());
        QString lastModified = fileInfo.lastModified().toString("yyyy-MM-dd  HH:mm:ss");

        QFileIconProvider provider;
        QIcon icon = provider.icon(fileInfo);
        QPixmap pix = icon.pixmap(QSize(48, 48));

/*
//        qDebug() << "Icon loaded via pixmap() has size " << pix.width() << "x" << pix.height() << "available48: " << available48.width() << "x" << available48.height() << "available64: " << available64.width() << "x" << available64.height() << "available256: " << available256.width() << "x" << available256.height();

        QList<QSize> sizes = icon.availableSizes();
        qDebug() << "--- Icon Analyse für:" << fileInfo.fileName() << "---";
        if (sizes.isEmpty()) {
            qDebug() << "Keine festen Größen hinterlegt (dynamisches/skalierbares Icon).";
        } else {
            for (int i = 0; i < sizes.size(); ++i) {
                qDebug() << QString("Auflösung [%1]: %2 x %3")
                                .arg(i)
                                .arg(sizes[i].width())
                                .arg(sizes[i].height());
            }
        }
        qDebug() << "------------------------------------------";

        if (pix.width() != 48 || pix.height() != 48) {
            pix = pix.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
*/
        QByteArray ba;
        QBuffer bu(&ba);
        pix.save(&bu, "PNG");
        QString imgBase64 = ba.toBase64();

        msgBox.setText(QString("<h3>%1</h3>").arg(sText));
        msgBox.setInformativeText(QString(tr("<table width='100%' cellspacing='0' cellpadding='0'><tr><td rowspan=4 width='48' valign='top' style='padding-right: 10px;'><img src='data:image/png;base64,%1'></td><td style='color: #555; padding-top: 2px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;' width='1%'>Name:</td><td style='color: #555; padding-top: 2px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;'>%2</td></tr><tr><td style='color: #555; padding-top: 2px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;'>Size:</td><td style='color: #555; padding-top: 2px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;'>%3</td></tr><tr><td style='color: #555; padding-top: 2px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;'>Date:</td><td style='color: #555; padding-top: 2px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;'>%4</td></tr><tr><td colspan=2 style='padding-top: 8px; padding-bottom: 2px; padding-left: 8px; padding-right: 8px;'>%5</td></tr></table>")).arg(imgBase64, fileName, size, lastModified, sWarning));
    } else {
        //msgBox.setText(sText);
        msgBox.setText(QString("<h3>%1</h3>").arg(sText));
        msgBox.setInformativeText(sWarning);
    }

    QPushButton *deleteButton = msgBox.addButton(tr("Delete"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(deleteButton);
    deleteButton->setStyleSheet("QPushButton { font-weight: bold; min-width: 80px; }");

    msgBox.exec();

    if (msgBox.clickedButton() != deleteButton) {
        return;
    }

    //----------------------------------------------------------------------------------------------
    // 4. Delete in reverse order. (Important! Wenn deleting row 2, row 3 will become row 2!)

    QList<int> sortedRows = rowSet.values();
    std::sort(sortedRows.begin(), sortedRows.end(), std::greater<int>());

    for (int row : std::as_const(sortedRows)) {
        QTableWidgetItem *nameItem = m_tableWidget->item(row, eColName);
        if (!nameItem) continue;

        QString path = nameItem->data(Qt::UserRole).toString();

        if (bRecycleOnly) {
            if (QFile::moveToTrash(path)) {
                m_tableWidget->removeRow(row);
            } else {
                // Fehlerbehandlung
            }
        } else {
            if (QFile::remove(path)) {
                m_tableWidget->removeRow(row);
            } else {
                // Fehlerbehandlung
            }
        }
    }
}

void MainWindow::action_ListViewCutFiles() {
    removeCutMarkers();

    QList<QTableWidgetItem*> selectedItems = m_tableWidget->selectedItems();

    // Zeilenindizes sammeln (verhindert Dopplungen bei Mehrfachauswahl in einer Zeile)
    QSet<int> rowSet;
    for (const auto *item : std::as_const(selectedItems)) {
        rowSet.insert(item->row());
    }

    for (int row : std::as_const(rowSet)) {
        for (int col = 0; col < m_tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = m_tableWidget->item(row, col);
            if (item) {
                item->setData(Qt::UserRole + 5, true);
            }
        }
    }

    m_rowsWithCutMarkers = rowSet;

    //m_tableWidget->viewport()->update();    // possibly not necessary
    setupClipboardForCut(rowSet);   // Todo: Better first try to change clipboard, and only on success ghost out cut items
}

void MainWindow::setupClipboardForCut(const QSet<int> &rowSet) {
    auto *mimeData = new QMimeData();
    QList<QUrl> urls;

    for (int row : rowSet) {
        // Wir nehmen an, der Pfad liegt in Spalte 0 in der UserRole
        QString path = m_tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            urls << QUrl::fromLocalFile(path);
        }
    }

    mimeData->setUrls(urls);

    // "Drop Effect" (Copy oder Move) for Windows
    QByteArray buffer;
    buffer.append(static_cast<char>(Qt::MoveAction));
    buffer.append('\0'); buffer.append('\0'); buffer.append('\0');
    mimeData->setData("Preferred DropEffect", buffer);

    // Particular to Linux (GNOME/Dolphin/etc.)
    // Format: "cut" oder "copy", dann Zeilenumbruch, dann alle URLs (ebenfalls per \n getrennt)
    QByteArray gnomeFormat = "cut";
    for (const QUrl &url : std::as_const(urls)) {
        gnomeFormat.append("\n");
        gnomeFormat.append(url.toEncoded());
    }
    mimeData->setData("x-special/gnome-copied-files", gnomeFormat);

    m_currentClipboardToken = QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    mimeData->setData("application/x-mkfilesearch-token", m_currentClipboardToken);

    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void MainWindow::onClipboardChanged() {
    // We can not just always remove the markers, since we get notified of our own changes to the clipboard as well.
    // We probably should set some flag with a random value when we cut items and check if this flag is still there.
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();

    if (!mimeData->hasFormat("application/x-mkfilesearch-token") || mimeData->data("application/x-mkfilesearch-token") != m_currentClipboardToken) {
        removeCutMarkers();
        m_currentClipboardToken.clear();
    }
}

void MainWindow::removeCutMarkers() {
    if (!m_rowsWithCutMarkers.isEmpty()) {
        m_tableWidget->setUpdatesEnabled(false);

        for (int r : std::as_const(m_rowsWithCutMarkers)) {
            for (int c = 0; c < m_tableWidget->columnCount(); ++c) {
                if (QTableWidgetItem *item = m_tableWidget->item(r, c)) {
                    item->setData(Qt::UserRole + 5, false);
                }
            }
        }

        m_rowsWithCutMarkers.clear();
        m_tableWidget->setUpdatesEnabled(true);
    }
}

void MainWindow::action_ListViewCopyFiles() {
    removeCutMarkers();

    QStringList pathList = getTablePathList();

    auto *mimeData = new QMimeData();
    QList<QUrl> urls;

    for (const QString &path : std::as_const(pathList)) {
        urls << QUrl::fromLocalFile(path);
    }
    mimeData->setUrls(urls);

    // 2. Den "Drop Effect" (Copy oder Move) setzen
    QByteArray buffer;
    buffer.append(static_cast<char>(Qt::CopyAction));
    buffer.append('\0'); buffer.append('\0'); buffer.append('\0');
    mimeData->setData("Preferred DropEffect", buffer);

    // 3. Speziell für Linux (GNOME/Dolphin/etc.)
    // Format: "cut" oder "copy", dann Zeilenumbruch, dann alle URLs (ebenfalls per \n getrennt)
    QByteArray gnomeFormat = "copy";
    for (const QUrl &url : std::as_const(urls)) {
        gnomeFormat.append("\n");
        gnomeFormat.append(url.toEncoded());
    }
    mimeData->setData("x-special/gnome-copied-files", gnomeFormat);

    m_currentClipboardToken = QByteArray::number(QDateTime::currentMSecsSinceEpoch());
    mimeData->setData("application/x-mkfilesearch-token", m_currentClipboardToken);

    QGuiApplication::clipboard()->setMimeData(mimeData);
}

void MainWindow::action_ListViewBrowseToFile() {
    QTableWidgetItem *item = m_tableWidget->currentItem();
    if (!item) return;

    QString path = m_tableWidget->item(item->row(), eColName)->data(Qt::UserRole).toString();
    browseToFile(path);
}

void MainWindow::action_ListViewRenameFiles() {
    QTableWidgetItem *item = m_tableWidget->currentItem();
    if (!item) return;

    int row = item->row();
    m_tableWidget->editItem(m_tableWidget->item(row, eColName));
}

void MainWindow::action_ListViewFileProperties() {
    QStringList pathList = getTablePathList();

    if (pathList.isEmpty()) {
        return;
    }

    auto *dialog = new FilePropertiesDialog(pathList);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::action_EditSettingsFile() {
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_settings.getSettingsPath()));
}

void MainWindow::onTimedUpdateIcons() {
    if (m_bSearchActive.load() || (m_tableWidget->rowCount() == 0)) {
        return;
    }

    int firstVisible = m_tableWidget->rowAt(0);
    int lastVisible = m_tableWidget->rowAt(m_tableWidget->viewport()->height() - 1);    // Substract 1 pixel to make sure we're in the viewport
    if (lastVisible == -1) {
        lastVisible = m_tableWidget->rowCount() - 1;
    }
    if ((firstVisible == -1) || (lastVisible == -1)) {
        return;
    }

    for (int i = firstVisible; i <= lastVisible; ++i) {
        QTableWidgetItem *nameItem = m_tableWidget->item(i, eColName);
        if (nameItem) {
            QString fullPath = nameItem->data(Qt::UserRole).toString();
            if (!fullPath.isEmpty()) {
                if (nameItem->data(Qt::UserRole + 1).toBool() == false) {
                    QFileInfo fileInfo(fullPath);
#ifdef Q_OS_WIN
                    bool needsTrueIcon = fileInfo.isDir() ||
                                         fullPath.endsWith(".exe", Qt::CaseInsensitive) ||
                                         fullPath.endsWith(".ico", Qt::CaseInsensitive) ||
                                         fullPath.endsWith(".lnk", Qt::CaseInsensitive) ||
                                         fullPath.endsWith(".msi", Qt::CaseInsensitive) ||
                                         fullPath.endsWith(".cur", Qt::CaseInsensitive) ||
                                         fullPath.endsWith(".ani", Qt::CaseInsensitive);
#else
                    bool needsTrueIcon = fileInfo.isDir() ||
                                         fileInfo.isExecutable() ||
                                         fullPath.endsWith(".desktop", Qt::CaseInsensitive);
#endif
                    if (needsTrueIcon) {
                        QIcon trueIcon = m_iconProvider.icon(fileInfo);

                        m_pathIconCache.insert(fullPath, trueIcon);

                        nameItem->setIcon(trueIcon);
                    }

                    nameItem->setData(Qt::UserRole + 1, true);  // set true so we don't update this item's icon again.
                }
            }
        }
    }
}

void MainWindow::addFileToTable(const QFileInfo &fileInfo, int iRow, int iLenRem, int nameMatchQuality, int contentMatchCount) {

    // Icon & Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileInfo.fileName());

    QHash<QString, QIcon>::iterator it;
    it = m_pathIconCache.find(fileInfo.absoluteFilePath());
    if (it != m_pathIconCache.end()) {
        nameItem->setData(Qt::UserRole + 1, true); // Mark as done, so the icon update timer ignores this one
    } else {
        if (fileInfo.isDir()) {
            // The '/' is a forbidden character for file names both on linux and windows,
            // so we can use it as an otherwise impossible suffix marker

            it = m_iconCache.find("//dir//");
            if (it == m_iconCache.end()) {
                it = m_iconCache.insert("//dir//", m_iconProvider.icon(QFileIconProvider::Folder));
            }
        } else {
            QString suffix = fileInfo.suffix().toLower();

            it = m_iconCache.find(suffix);
            if (it == m_iconCache.end()) {
                QFileInfo dummyInfo("any_filename." + suffix);
                it = m_iconCache.insert(suffix, m_iconProvider.icon(dummyInfo));
            }
        }
    }

    nameItem->setIcon(it.value());
    nameItem->setData(Qt::UserRole, fileInfo.absoluteFilePath());
    nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    m_tableWidget->setItem(iRow, eColName, nameItem);

    // Subfolder
    QTableWidgetItem *pathItem = new QTableWidgetItem();
    QString pathOnly = fileInfo.absolutePath();
    if (iLenRem <= pathOnly.length()) {
        pathItem->setData(Qt::DisplayRole, QDir::toNativeSeparators(pathOnly.sliced(iLenRem)));
    }
    pathItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColPath, pathItem);

    // Size (right aligned)
    quint64 sizeInBytes = fileInfo.size();
    SizeTableItem *sizeItem = new SizeTableItem(formatAdaptiveSize(sizeInBytes));     // Using sublass "sizeItem" in place of "QTableWidgetItem"
    sizeItem->setData(Qt::UserRole, sizeInBytes);
    sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sizeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColSize, sizeItem);

    // Date
    QTableWidgetItem *dateItem = new QTableWidgetItem(fileInfo.lastModified().toString("yyyy-MM-dd  HH:mm:ss"));
    dateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColDate, dateItem);

    // Type or File Extension
    QTableWidgetItem *typeItem = new QTableWidgetItem(fileInfo.suffix());
    //typeItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColType, typeItem);

    // Match quality
    QTableWidgetItem *qualityItem = new QTableWidgetItem();
    if (nameMatchQuality != -1)
        qualityItem->setData(Qt::DisplayRole, nameMatchQuality);
    qualityItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    qualityItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColQuality, qualityItem);

    // Content match count
    QTableWidgetItem *countItem = new QTableWidgetItem();
    if (contentMatchCount != -1)
        countItem->setData(Qt::DisplayRole, contentMatchCount);
    countItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    countItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColCount, countItem);

    // CRC
    QTableWidgetItem *crcItem = new QTableWidgetItem();
    crcItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    crcItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    m_tableWidget->setItem(iRow, eColCRC, crcItem);
}

#ifdef Q_OS_WIN
QString MainWindow::getSendToPath() {
    PWSTR path = nullptr;
    // FOLDERID_SendTo ist die offizielle GUID für diesen Ordner
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_SendTo, 0, nullptr, &path);
    if (SUCCEEDED(hr)) {
        QString result = QString::fromWCharArray(path);
        CoTaskMemFree(path); // Wichtig: Speicher freigeben
        return result;
    }
    return QString();
}
#endif

void MainWindow::loadMimeCache() {
    m_mimeCache.clear();
    m_mimeCache.reserve(500);

    QStringList appDirs = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);  // Order: User before System
    std::reverse(appDirs.begin(), appDirs.end());   // Reverse to System before User, so we can overwrite System with User keys while parsing
    for (const QString &dirPath : std::as_const(appDirs)) {
        QString cachePath = QDir(dirPath).filePath("mimeinfo.cache");
        parseMimeInfoCache(cachePath);
    }

    parseMimeAppsList(QDir::homePath() + "/.config/mimeapps.list");
}

void MainWindow::parseMimeInfoCache(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        auto equalsPos = line.indexOf('=');
        if (equalsPos < 1) continue;  // -1 (kein '=') und 0 (leerer mime) überspringen

        QString mime = line.first(equalsPos).trimmed();
        QStringList newApps = line.sliced(equalsPos + 1).split(';', Qt::SkipEmptyParts);

        QStringList &currentApps = m_mimeCache[mime];
        for (const QString &app : std::as_const(newApps)) {
            QString trimmed = app.trimmed();
            if (!trimmed.isEmpty() && !currentApps.contains(trimmed)) {
                currentApps.append(trimmed);
            }
        }
    }
}

void MainWindow::parseMimeAppsList(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&file);
    QString currentGroup;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        if (line.startsWith('[') && line.endsWith(']')) {
            currentGroup = line.mid(1, line.length() - 2);
            continue;
        }

        auto equalsPos = line.indexOf('=');
        if (equalsPos < 1) continue; // -1 (kein '=') und 0 (leerer mime) überspringen

        QString mime = line.first(equalsPos).trimmed();
        QStringList apps = line.sliced(equalsPos + 1).trimmed().split(';', Qt::SkipEmptyParts);

        if (currentGroup == "Added Associations" || currentGroup == "Default Applications") {
            QStringList &currentApps = m_mimeCache[mime];

            for (int i = apps.size() - 1; i >= 0; --i) {
                QString app = apps.at(i).trimmed();
                if (app.isEmpty()) continue;

                currentApps.removeAll(app);
                currentApps.prepend(app);
            }
        }
        else if (currentGroup == "Removed Associations") {
            QStringList &currentApps = m_mimeCache[mime];
            for (const QString &app : std::as_const(apps)) {
                currentApps.removeAll(app.trimmed());
            }
        }
    }
}

//######################################################################################
// Protected Overrides

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateColumns();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_tableWidget->viewport() && event->type() == QEvent::Resize) {
        m_timerCalcCrc->start(100);
        m_timerUpdateIcons->start(100);
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        //if (keyEvent->key()) qDebug() << "eventFilter: key pressed:" << keyEvent->key();

        if (keyEvent->key() == Qt::Key_Escape) {
            //qDebug() << "eventFilter: Qt::Key_Escape pressed!   m_bSearchActive=" << m_bSearchActive;
            if (m_bSearchActive) {
                m_bAbortRequested = true;
                emit abortSearchWorkerRequested();
                return true;
            }

            if (m_tableWidget->hasFocus()) {
                // Empty Clipboard. But only if it's our own!
                const QMimeData* mimeData = QApplication::clipboard()->mimeData();
                if (mimeData->hasFormat("application/x-mkfilesearch-token") && mimeData->data("application/x-mkfilesearch-token") == m_currentClipboardToken) {
                    QApplication::clipboard()->clear();
                    // removeCutMarkers();  // Will be called automatically since we changed the clipboard
                    return true; // Escape "verschlucken"
                }
            }
        }
    }

    return QObject::eventFilter(obj, event);
}
