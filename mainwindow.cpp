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
#include <QVBoxLayout>
#include <utility> // Für std::as_const

#include "mainwindow.h"
#include "helpers.h"
#include "searchworker.h"
#include "filepropertiesdialog.h"

MainWindow::MainWindow(const QString &targetDirectory, QWidget *parent)
    : QMainWindow(parent), currentDirectory(targetDirectory)
{
    setWindowTitle(QDir::toNativeSeparators(currentDirectory));
    setWindowIcon(QIcon(":/icons/res/app.ico"));
    resize(728, 545);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout1 = new QVBoxLayout();
    //topControlsVBoxLayout1->setSpacing(15);
    topControlsVBoxLayout1->setContentsMargins(5, 5, 5, 5);

    InputBox1 = new QLineEdit();
    topControlsVBoxLayout1->addWidget(InputBox1);

    InputBox2 = new QLineEdit();
    topControlsVBoxLayout1->addWidget(InputBox2);

    if (m_settings.showPlaceholderText == true) {
        InputBox1->setPlaceholderText(tr("(filename search terms)"));
        InputBox2->setPlaceholderText(tr("(content search terms)"));
    }

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout2 = new QVBoxLayout();
    //topControlsVBoxLayout2->setSpacing(15);
    topControlsVBoxLayout2->setContentsMargins(5, 5, 5, 5);

    CheckboxRegExName = new QCheckBox("RegEx");
    CheckboxRegExName->setChecked(false);
    topControlsVBoxLayout2->addWidget(CheckboxRegExName);

    CheckboxRegExContent = new QCheckBox("RegEx");
    CheckboxRegExContent->setChecked(false);
    topControlsVBoxLayout2->addWidget(CheckboxRegExContent);

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout3 = new QVBoxLayout();
    //topControlsVBoxLayout3->setSpacing(15);
    topControlsVBoxLayout3->setContentsMargins(5, 5, 5, 5);

    CheckboxNameCaseSense = new QCheckBox(tr("CaseSense"));
    CheckboxNameCaseSense->setChecked(false);
    topControlsVBoxLayout3->addWidget(CheckboxNameCaseSense);

    CheckboxContentCaseSense = new QCheckBox(tr("CaseSense"));
    CheckboxContentCaseSense->setChecked(false);
    topControlsVBoxLayout3->addWidget(CheckboxContentCaseSense);

    // --------------------------------------------------------------------

    QVBoxLayout *topControlsVBoxLayout4 = new QVBoxLayout();
    //topControlsVBoxLayout4->setSpacing(15);
    topControlsVBoxLayout4->setContentsMargins(5, 5, 25, 5);

    CheckboxDirectories = new QCheckBox(tr("Directories"));
    CheckboxDirectories->setChecked(false);
    CheckboxDirectories->setTristate(true);
    topControlsVBoxLayout4->addWidget(CheckboxDirectories);

    CheckboxCRC = new QCheckBox(tr("CRC"));
    CheckboxCRC->setChecked(false);
    topControlsVBoxLayout4->addWidget(CheckboxCRC);

    // --------------------------------------------------------------------

    // Container for editbox and checkbox controls

    topControlsContainerWidget = new QWidget();
    QHBoxLayout *topControlsHBoxLayout = new QHBoxLayout(topControlsContainerWidget);
    topControlsHBoxLayout->setContentsMargins(0, 0, 0, 0);
    topControlsHBoxLayout->setSpacing(5);

    topControlsHBoxLayout->addLayout(topControlsVBoxLayout1);
    topControlsHBoxLayout->addLayout(topControlsVBoxLayout2);
    topControlsHBoxLayout->addLayout(topControlsVBoxLayout3);
    topControlsHBoxLayout->addLayout(topControlsVBoxLayout4);

    // --------------------------------------------------------------------
    // ListView

    tableWidget = new Custom_QTableWidget();
    tableWidget->setItemDelegate(new CutDelegate(this));
    tableWidget->setEditTriggers(QAbstractItemView::EditKeyPressed);    // QAbstractItemView::NoEditTriggers
    tableWidget->setStyleSheet("QTableWidget { border: none; }");
    tableWidget->verticalHeader()->setVisible(false);
    tableWidget->verticalHeader()->setMinimumSectionSize(0);
    tableWidget->verticalHeader()->setDefaultSectionSize(18);
    tableWidget->setColumnCount(8);
    tableWidget->setHorizontalHeaderLabels({tr("Name"), tr("Subfolder"), tr("Size"), tr("Changed"), tr("Type"), tr("Rating"), tr("Count"), tr("CRC")});

    tableWidget->setColumnWidth(eColName, 160);
    tableWidget->setColumnWidth(eColSubpath, 160);
    tableWidget->setColumnWidth(eColSize, 60);
    tableWidget->setColumnWidth(eColDate, 118);
    tableWidget->setColumnWidth(eColType, 46);
    tableWidget->setColumnWidth(eColQuality, 45);
    tableWidget->setColumnWidth(eColCount, 42);
    tableWidget->setColumnWidth(eColCRC, 80);

    tableWidget->horizontalHeaderItem(eColName)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColSubpath)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColSize)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColDate)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColType)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColQuality)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColCount)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    tableWidget->horizontalHeaderItem(eColCRC)->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);

    tableWidget->horizontalHeader()->setSectionsMovable(true);
    tableWidget->horizontalHeader()->setHighlightSections(false);

    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    tableWidget->setAlternatingRowColors(m_settings.alternatingRowColors);
    tableWidget->setShowGrid(m_settings.showGrid);
    tableWidget->setColumnHidden(eColCRC, true);

    tableWidget->resizeColumnToContents(eColSize);
    tableWidget->resizeColumnToContents(eColDate);
    tableWidget->resizeColumnToContents(eColType);
    tableWidget->resizeColumnToContents(eColQuality);
    tableWidget->resizeColumnToContents(eColCount);

    // --------------------------------------------------------------------

    mainLayout->addWidget(topControlsContainerWidget);
    mainLayout->addWidget(tableWidget);

    // --------------------------------------------------------------------
    // Shortcuts: Whole Window

    QShortcut *WindowShortcutN = new QShortcut(QKeySequence("Ctrl+N"), this);
    WindowShortcutN->setContext(Qt::WindowShortcut);
    connect(WindowShortcutN, &QShortcut::activated, this, &MainWindow::action_EditSettingsFile);

    // --------------------------------------------------------------------
    // Context menu Actions (tableWidget)

    m_actionListViewOpenFiles = new QAction(tr("Open"), this);
    //m_actionListViewOpenFiles->setShortcut(QKeySequence("Ctrl+O"));
    //m_actionListViewOpenFiles->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewOpenFiles);
    connect(m_actionListViewOpenFiles, &QAction::triggered, this, &MainWindow::action_ListViewOpenFiles);

    m_actionListViewEditFiles = new QAction(tr("Edit"), this);
    m_actionListViewEditFiles->setShortcut(QKeySequence("Ctrl+E"));
    m_actionListViewEditFiles->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewEditFiles);
    connect(m_actionListViewEditFiles, &QAction::triggered, this, &MainWindow::action_ListViewEditFiles);

    m_actionListViewBrowseToFile = new QAction(tr("Show in folder"),this);
    m_actionListViewBrowseToFile->setIcon(QIcon::fromTheme("folder-open"));
    m_actionListViewBrowseToFile->setShortcut(QKeySequence("Ctrl+L"));
    m_actionListViewBrowseToFile->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewBrowseToFile);
    connect(m_actionListViewBrowseToFile, &QAction::triggered, this, &MainWindow::action_ListViewBrowseToFile);

    m_actionListViewCopyPaths = new QAction(tr("Copy Path"), this);
    m_actionListViewCopyPaths->setIcon(QIcon::fromTheme("edit-copy-path"));
    m_actionListViewCopyPaths->setShortcut(QKeySequence("Ctrl+Shift+C"));
    m_actionListViewCopyPaths->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewCopyPaths);
    connect(m_actionListViewCopyPaths, &QAction::triggered, this, &MainWindow::action_ListViewCopyPaths);

    m_actionListViewCutFiles = new QAction(tr("Cut"), this);
    m_actionListViewCutFiles->setIcon(QIcon::fromTheme("edit-cut"));
    m_actionListViewCutFiles->setShortcut(QKeySequence("Ctrl+X"));
    m_actionListViewCutFiles->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewCutFiles);
    connect(m_actionListViewCutFiles, &QAction::triggered, this, &MainWindow::action_ListViewCutFiles);

    m_actionListViewCopyFiles = new QAction(tr("Copy"), this);
    m_actionListViewCopyFiles->setIcon(QIcon::fromTheme("edit-copy"));
    m_actionListViewCopyFiles->setShortcut(QKeySequence("Ctrl+C"));
    m_actionListViewCopyFiles->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewCopyFiles);
    connect(m_actionListViewCopyFiles, &QAction::triggered, this, &MainWindow::action_ListViewCopyFiles);

    m_actionListViewDeleteFiles = new QAction(tr("Delete"), this);
    m_actionListViewDeleteFiles->setIcon(QIcon::fromTheme("edit-delete"));
    m_actionListViewDeleteFiles->setShortcut(QKeySequence::Delete);
    m_actionListViewDeleteFiles->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewDeleteFiles);
    connect(m_actionListViewDeleteFiles, &QAction::triggered, this, [this]() { action_ListViewDeleteFiles(true); });

    m_actionListViewRenameFiles = new QAction(tr("Rename"), this);
    m_actionListViewRenameFiles->setIcon(QIcon::fromTheme("edit-rename"));
    m_actionListViewRenameFiles->setShortcut(QKeySequence(Qt::Key_F2));
    m_actionListViewRenameFiles->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewRenameFiles);
    connect(m_actionListViewRenameFiles, &QAction::triggered, this, &MainWindow::action_ListViewRenameFiles);

    m_actionListViewFileProperties = new QAction(tr("Properties"), this);
    m_actionListViewFileProperties->setIcon(QIcon::fromTheme("document-properties"));
    m_actionListViewFileProperties->setShortcut(QKeySequence("Ctrl+I"));
    m_actionListViewFileProperties->setShortcutContext(Qt::WidgetShortcut);
    tableWidget->addAction(m_actionListViewFileProperties);
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
    // Shorcuts: tableWidget

    QShortcut *ListViewShortcutHome = new QShortcut(QKeySequence(Qt::Key_Home), tableWidget);
    connect(ListViewShortcutHome, &QShortcut::activated, this, [this]() { if (tableWidget->rowCount() > 0) { tableWidget->setCurrentCell(0, 0); tableWidget->scrollToTop(); }});

    QShortcut *ListViewShortcutEnd = new QShortcut(QKeySequence(Qt::Key_End), tableWidget);
    connect(ListViewShortcutEnd, &QShortcut::activated, this, [this]() { int lastRow = tableWidget->rowCount() - 1; if (lastRow >= 0) { tableWidget->setCurrentCell(lastRow, 0);tableWidget->scrollToBottom(); }});

    QShortcut *ListViewShortcutShiftDel = new QShortcut(QKeySequence("Shift+Delete"), tableWidget);
    ListViewShortcutShiftDel->setContext(Qt::WidgetShortcut);
    connect(ListViewShortcutShiftDel, &QShortcut::activated, this,  [this]() { action_ListViewDeleteFiles(false); });

    // --------------------------------------------------------------------
    // Shorcuts: InputBoxes

    QShortcut *InputBox1ShortcutR = new QShortcut(QKeySequence("Ctrl+R"), InputBox1);
    InputBox1ShortcutR->setContext(Qt::WidgetShortcut);
    connect(InputBox1ShortcutR, &QShortcut::activated, this, [this]() { CheckboxRegExName->toggle(); });

    QShortcut *InputBox1ShortcutD = new QShortcut(QKeySequence("Ctrl+D"), InputBox1);
    InputBox1ShortcutD->setContext(Qt::WidgetShortcut);
    connect(InputBox1ShortcutD, &QShortcut::activated, this, [this]() { CheckboxDirectories->toggle(); });

    QShortcut *InputBox2ShortcutR = new QShortcut(QKeySequence("Ctrl+R"), InputBox2);
    InputBox2ShortcutR->setContext(Qt::WidgetShortcut);
    connect(InputBox2ShortcutR, &QShortcut::activated, this, [this]() { CheckboxRegExContent->toggle(); });

    QShortcut *InputBox1ShortcutBackSpace = new QShortcut(QKeySequence("Ctrl+Backspace"), InputBox1);
    InputBox1ShortcutBackSpace->setContext(Qt::WidgetShortcut);
    connect(InputBox1ShortcutBackSpace, &QShortcut::activated, this, [this]() { m_currentSearchGeneration++; tableWidget->setRowCount(0); InputBox2->clear(); InputBox1->clear(); setWindowTitle(QDir::toNativeSeparators(currentDirectory)); InputBox1->setFocus(); });

    QShortcut *InputBox2ShortcutBackSpace = new QShortcut(QKeySequence("Ctrl+Backspace"), InputBox2);
    InputBox2ShortcutBackSpace->setContext(Qt::WidgetShortcut);
    connect(InputBox2ShortcutBackSpace, &QShortcut::activated, this, [this]() { m_currentSearchGeneration++; tableWidget->setRowCount(0); InputBox2->clear(); InputBox1->clear(); setWindowTitle(QDir::toNativeSeparators(currentDirectory)); InputBox1->setFocus(); });


    // --------------------------------------------------------------------
    // Shorcuts: CheckBoxes

    QShortcut *CheckboxRegExFileNameEnter = new QShortcut(QKeySequence("Enter"), CheckboxRegExName);
    CheckboxRegExFileNameEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExFileNameEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxRegExFileNameReturn = new QShortcut(QKeySequence("Return"), CheckboxRegExName);
    CheckboxRegExFileNameReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExFileNameReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxRegExContentEnter = new QShortcut(QKeySequence("Enter"), CheckboxRegExContent);
    CheckboxRegExContentEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExContentEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxRegExContentReturn = new QShortcut(QKeySequence("Return"), CheckboxRegExContent);
    CheckboxRegExContentReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxRegExContentReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxNameCaseSenseEnter = new QShortcut(QKeySequence("Enter"), CheckboxNameCaseSense);
    CheckboxNameCaseSenseEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxNameCaseSenseEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxNameCaseSenseReturn = new QShortcut(QKeySequence("Return"), CheckboxNameCaseSense);
    CheckboxNameCaseSenseReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxNameCaseSenseReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxContentCaseSenseEnter = new QShortcut(QKeySequence("Enter"), CheckboxContentCaseSense);
    CheckboxContentCaseSenseEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxContentCaseSenseEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxContentCaseSenseReturn = new QShortcut(QKeySequence("Return"), CheckboxContentCaseSense);
    CheckboxContentCaseSenseReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxContentCaseSenseReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxDirectoriesEnter = new QShortcut(QKeySequence("Enter"), CheckboxDirectories);
    CheckboxDirectoriesEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxDirectoriesEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxDirectoriesReturn = new QShortcut(QKeySequence("Return"), CheckboxDirectories);
    CheckboxDirectoriesReturn->setContext(Qt::WidgetShortcut);
    connect(CheckboxDirectoriesReturn, &QShortcut::activated, this, &MainWindow::startSearch);

    // -----------------------

    QShortcut *CheckboxCRCEnter = new QShortcut(QKeySequence("Enter"), CheckboxCRC);
    CheckboxCRCEnter->setContext(Qt::WidgetShortcut);
    connect(CheckboxCRCEnter, &QShortcut::activated, this, &MainWindow::startSearch);

    QShortcut *CheckboxCRCReturn = new QShortcut(QKeySequence("Return"), CheckboxCRC);
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

    connect(InputBox1, &QLineEdit::returnPressed, this, &MainWindow::startSearch);
    connect(InputBox2, &QLineEdit::returnPressed, this, &MainWindow::startSearch);
    connect(InputBox1, &QLineEdit::textChanged, this, &MainWindow::validateInputBoxRegex);
    connect(InputBox2, &QLineEdit::textChanged, this, &MainWindow::validateInputBoxRegex);
    connect(CheckboxRegExName, &QCheckBox::checkStateChanged, this, &MainWindow::validateInputBoxRegex);
    connect(CheckboxRegExContent, &QCheckBox::checkStateChanged, this, &MainWindow::validateInputBoxRegex);
    connect(tableWidget, &Custom_QTableWidget::itemChanged, this, &MainWindow::onItemChanged);
    connect(tableWidget, &Custom_QTableWidget::customContextMenuRequested, this, &MainWindow::onShowContextMenu);
    connect(tableWidget, &Custom_QTableWidget::itemDoubleClicked, this, &MainWindow::onListViewItemDoubleClicked);
    connect(CheckboxCRC, &QCheckBox::checkStateChanged, this, &MainWindow::onCheckboxClickedCRC);
    connect(CheckboxRegExName, &QCheckBox::checkStateChanged, this, &MainWindow::onCheckboxClickedRegExName);
    connect(CheckboxRegExContent, &QCheckBox::checkStateChanged, this, &MainWindow::onCheckboxClickedRegExContent);
    connect(tableWidget->verticalScrollBar(), &QScrollBar::valueChanged, this, &MainWindow::onVerticalBarScrollChange);
    connect(tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &MainWindow::onListViewHeaderClicked);
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::onClipboardChanged);
}

MainWindow::~MainWindow() = default;

void MainWindow::onCheckboxClickedRegExName(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        InputBox1->setPlaceholderText(tr("(filename search terms)"));
    } else {
        InputBox1->setPlaceholderText(tr("(filename regex expression)"));
    }
}

void MainWindow::onCheckboxClickedRegExContent(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        InputBox2->setPlaceholderText(tr("(content search terms)"));
    } else {
        InputBox2->setPlaceholderText(tr("(content regex expression)"));
    }
}

void MainWindow::onCheckboxClickedCRC(Qt::CheckState state) {
    tableWidget->setColumnHidden(eColCRC, !state);
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
    QString InputBox1Text = InputBox1->text();
    QString InputBox2Text = InputBox2->text();

    if (InputBox1Text.isEmpty()) {
        InputBox1->setStyleSheet("");
    } else {
        if (CheckboxRegExName->isChecked()) {
            QRegularExpression re1(InputBox1Text);
            if (!re1.isValid()) {
                InputBox1->setStyleSheet("background-color: red; color: white;");
            } else {
                InputBox1->setStyleSheet("");
            }
        } else {
            InputBox1->setStyleSheet("");
        }
    }

    if (InputBox2Text.isEmpty()) {
        InputBox2->setStyleSheet("");
    } else {
        if (CheckboxRegExContent->isChecked()) {
            QRegularExpression re2(InputBox2Text);
            if (!re2.isValid()) {
                InputBox2->setStyleSheet("background-color: red; color: white;");
            } else {
                InputBox2->setStyleSheet("");
            }
        } else {
            InputBox2->setStyleSheet("");
        }
    }
}

void MainWindow::onTimedCalcCRC() {
    if (m_bSearchActive.load() || (tableWidget->rowCount() == 0)) {
        return;
    }

    int firstVisible = tableWidget->rowAt(0);
    int lastVisible = tableWidget->rowAt(tableWidget->viewport()->height() - 1);    // Substract 1 pixel to make sure we're in the viewport
    if (lastVisible == -1) {
        lastVisible = tableWidget->rowCount() - 1;
    }
    if ((firstVisible == -1) || (lastVisible == -1)) {
        return;
    }

    if (CheckboxCRC->isChecked()) {
        for (int i = firstVisible; i <= lastVisible; ++i) {
            QTableWidgetItem *crcItem = tableWidget->item(i, eColCRC);
            QTableWidgetItem *nameItem = tableWidget->item(i, eColName);

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

                        // Check with crcItem->tableWidget() if crcItem still lives within a table
                        if (crcItem->tableWidget() != nullptr) {
                            int currentRow = crcItem->row(); // Get items's current row (works even after changing sort order)
                            QTableWidgetItem *currentNameItem = tableWidget->item(currentRow, eColName);

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
            QTableWidgetItem *crcItem = tableWidget->item(i, eColCRC);
            if (crcItem && !crcItem->text().isEmpty()) {
                crcItem->setText("");
            }
        }
    }
}

void MainWindow::startSearch() {
    QString InputBox1Text = InputBox1->text();
    QString InputBox2Text = InputBox2->text();
    bool bRegExFilename = CheckboxRegExName->isChecked();
    bool bRegExContent = CheckboxRegExContent->isChecked();

    if (m_bSearchActive) {
        return;
    }

    if (InputBox1Text.trimmed().isEmpty() && InputBox2Text.trimmed().isEmpty()) {
        m_currentSearchGeneration++; // Make old crc calc threads invalid to prevent seg faults from Use-After-Free
        tableWidget->setRowCount(0);
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
    if (m_lastWidget == CheckboxRegExName || m_lastWidget == CheckboxNameCaseSense || m_lastWidget == CheckboxDirectories) {
        m_lastWidget = InputBox1;
    } else if (m_lastWidget == CheckboxRegExContent || m_lastWidget == CheckboxContentCaseSense || m_lastWidget == CheckboxCRC) {
        m_lastWidget = InputBox2;
    }

    topControlsContainerWidget->setEnabled(false);
    tableWidget->setEnabled(false);

    topControlsContainerWidget->repaint();
    tableWidget->repaint();

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    setWindowTitle(QDir::toNativeSeparators(currentDirectory) + "   (Searching)");
    m_currentSearchGeneration++; // Make old crc calc threads invalid to prevent seg faults from Use-After-Free

    tableWidget->setUpdatesEnabled(false);
    tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);  // Important!! Calculations of header don't get stopped by "tableWidget->setUpdatesEnabled(false)"
    tableWidget->setRowCount(0);
    tableWidget->setSortingEnabled(false);
    tableWidget->blockSignals(true);  // block "itemChanged" signals

    QThread* thread = new QThread;
    SearchWorker* worker = new SearchWorker(currentDirectory, InputBox1Text, InputBox2Text, bRegExFilename, bRegExContent, CheckboxNameCaseSense->isChecked(), CheckboxContentCaseSense->isChecked(), CheckboxDirectories->checkState(), m_settings.textExts);
    worker->moveToThread(thread);

    // Verbindungen
    connect(thread, &QThread::started, worker, &SearchWorker::process);
    connect(worker, &SearchWorker::filesFoundBatch, this, &MainWindow::onWorkerSentBatch);
    connect(worker, &SearchWorker::searchStats, this, &MainWindow::onWorkerFinished);   // 1. Store result info
    connect(worker, &SearchWorker::finished, thread, &QThread::quit);                   // 2. Stop thread
    connect(worker, &SearchWorker::finished, worker, &SearchWorker::deleteLater);       // 3. Clean up object
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);                 // 4. Clean up thread

    connect(this, &MainWindow::abortSearchWorkerRequested, worker, &SearchWorker::abort, Qt::DirectConnection);   // React to "Escape" key press

    thread->start();
}

void MainWindow::onWorkerSentBatch(const QList<SearchResult> &batch) {
    m_pendingBatches.enqueue(batch);

    // Wenn bereits eine Verarbeitungsschleife läuft, beenden wir diesen Aufruf.
    // Die laufende Schleife wird unseren Batch später automatisch aus der Queue holen.
    if (m_isProcessingPending) return;

    // Wir sind der "Master"-Aufruf. Wir starten die Verarbeitung.
    m_isProcessingPending = true;

    while (!m_pendingBatches.isEmpty()) {
        // We've set the batch size to 1000 in the SearchWorker, which on my system takes around 10 ms.
        // It should be fine (converning UI-responsiveness) to processEvents() every 10 ms.
        QCoreApplication::processEvents();
        if (m_bAbortRequested) break;

        // Fetch next batch from queue
        QList<SearchResult> currentBatch = m_pendingBatches.dequeue();
        if (currentBatch.isEmpty()) {
            continue;
        }

        int currentRows = tableWidget->rowCount();
        int newItemsCount = currentBatch.size();

        tableWidget->setRowCount(currentRows + newItemsCount);

        for (int i = 0; i < newItemsCount; ++i) {
            int targetRow = currentRows + i;
            const auto &res = currentBatch.at(i);

            QFileInfo fileInfo = res.fileInfo;
            int iLenRem = res.iLenRem;
            int nameMatchQuality = res.nameMatchQuality;
            int contentMatchCount = res.contentMatchCount;

            addFileToTable(fileInfo, targetRow, iLenRem, nameMatchQuality, contentMatchCount);
        }
    }

    if (m_bAbortRequested) {
        m_pendingBatches.clear();
    }

    m_isProcessingPending = false;

    if (m_workerHasFinished) {
        finalizeUI();
    }
}

void MainWindow::addFileToTable(QFileInfo fileInfo, int iRow, int iLenRem, int nameMatchQuality, int contentMatchCount) {
    //-----------------------------------
    // Icon Cache

    QHash<QString, QIcon>::iterator it;

    if (fileInfo.isDir()) {
        // The '/' is a forbidden character for file names both on linux and windows,
        // so we can use it as an otherwise impossible suffix marker

        it = m_iconCache.find("//dir//");
        if (it == m_iconCache.end()) {
            it = m_iconCache.insert("//dir//", m_iconProvider.icon(QFileIconProvider::Folder));
        }

        /*
        if (!m_iconCache.contains("//dir//")) {
            m_iconCache.insert("//dir//", m_iconProvider.icon(QFileIconProvider::Folder));
        }
        */
    } else {
        QString suffix = fileInfo.suffix().toLower();

        it = m_iconCache.find(suffix);
        if (it == m_iconCache.end()) {
            QFileInfo dummyInfo("any_filename." + suffix);
            it = m_iconCache.insert(suffix, m_iconProvider.icon(dummyInfo));
        }

        /*
        if (!m_iconCache.contains(suffix)) {
            //m_iconCache.insert(suffix, m_iconProvider.icon(fileInfo)); // This would give us the unique file's actual icon, including overlays etc.
            QFileInfo dummyInfo("any_filename." + suffix);
            m_iconCache.insert(suffix, m_iconProvider.icon(dummyInfo));
        }
        */
    }

    // Icon & Name
    QTableWidgetItem *nameItem = new QTableWidgetItem(fileInfo.fileName());
    nameItem->setIcon(it.value());
    nameItem->setData(Qt::UserRole, fileInfo.absoluteFilePath());
    nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);
    tableWidget->setItem(iRow, eColName, nameItem);

    // Subfolder
    QTableWidgetItem *pathItem = new QTableWidgetItem();
    QString pathOnly = fileInfo.absolutePath();
    if (iLenRem <= pathOnly.length()) {
        pathItem->setData(Qt::DisplayRole, QDir::toNativeSeparators(pathOnly.sliced(iLenRem)));
    }
    pathItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColSubpath, pathItem);

    // Size (right aligned)
    qint64 sizeInBytes = fileInfo.size();
    SizeTableItem *sizeItem = new SizeTableItem(formatAdaptiveSize(sizeInBytes));     // Using sublass "sizeItem" in place of "QTableWidgetItem"
    sizeItem->setData(Qt::UserRole, sizeInBytes);
    sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sizeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColSize, sizeItem);

    // Date
    QTableWidgetItem *dateItem = new QTableWidgetItem(fileInfo.lastModified().toString("yyyy-MM-dd  HH:mm:ss"));
    dateItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColDate, dateItem);

    // Type or File Extension
    QTableWidgetItem *typeItem = new QTableWidgetItem(fileInfo.suffix());
    //typeItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    typeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColType, typeItem);

    // Match quality
    QTableWidgetItem *qualityItem = new QTableWidgetItem();
    if (nameMatchQuality != -1)
        qualityItem->setData(Qt::DisplayRole, nameMatchQuality);
    qualityItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    qualityItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColQuality, qualityItem);

    // Content match count
    QTableWidgetItem *countItem = new QTableWidgetItem();
    if (contentMatchCount != -1)
        countItem->setData(Qt::DisplayRole, contentMatchCount);
    countItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    countItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColCount, countItem);

    // CRC
    QTableWidgetItem *crcItem = new QTableWidgetItem();
    crcItem->setTextAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    crcItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    tableWidget->setItem(iRow, eColCRC, crcItem);
}


void MainWindow::onWorkerFinished(uint iItemsFound, uint iNameMatched, uint iContentMatched, bool bSearchInterrupted) {
    m_SearchStats_iItemsFound = iItemsFound;
    m_SearchStats_iNameMatched = iNameMatched;
    m_SearchStats_iContentMatched = iContentMatched;
    m_SearchStats_bSearchInterrupted = bSearchInterrupted;

    m_workerHasFinished = true;

    // Falls die Queue schon leer war, als das Signal kam:
    if (!m_isProcessingPending && m_pendingBatches.isEmpty()) {
        finalizeUI();
    }
}


void MainWindow::finalizeUI() {
    qDebug() << "finalizeUI() entry point. m_BenchmarkTimer:" << m_BenchmarkTimer.elapsed() << " ms elapsed since start of search.  m_SearchStats_bSearchInterrupted =" << m_SearchStats_bSearchInterrupted << "  m_bAbortRequested = " << m_bAbortRequested;

    if (m_SearchStats_iItemsFound == 0 || m_SearchStats_bSearchInterrupted == true || m_bAbortRequested) {
        m_currentSearchGeneration++; // Make old crc calc threads invalid to prevent seg faults from Use-After-Free
        tableWidget->setRowCount(0);
    }

    tableWidget->blockSignals(false);

    if (m_SearchStats_iContentMatched > 0)
        tableWidget->sortByColumn(eColCount, Qt::DescendingOrder);
    else
        tableWidget->sortByColumn(eColQuality, Qt::AscendingOrder);

    // Check again to see, if Escape key was pressed. We might react to user Input 100 ms sooner if we don't need to wait for the painting of a large number of list items.
    QCoreApplication::processEvents();
    if (m_bAbortRequested) {
        tableWidget->setRowCount(0);
    }

    tableWidget->setSortingEnabled(true);
    tableWidget->setUpdatesEnabled(true);

    tableWidget->resizeColumnToContents(eColName);
    //tableWidget->resizeColumnToContents(eColSubpath);
    tableWidget->resizeColumnToContents(eColSize);
    tableWidget->resizeColumnToContents(eColDate);
    tableWidget->resizeColumnToContents(eColType);
    tableWidget->resizeColumnToContents(eColQuality);
    tableWidget->resizeColumnToContents(eColCount);
    //tableWidget->resizeColumnToContents(eColCRC);

    QString titleString;
    QString InputBox2Text = InputBox2->text();

    if (m_SearchStats_bSearchInterrupted == true || m_bAbortRequested) {
        titleString = QString(tr("Search aborted..."));
    } else if (InputBox2Text.trimmed().isEmpty()) {
        titleString = QString(tr("%1 (%2 hits in %3 items)")).arg(QDir::toNativeSeparators(currentDirectory), QLocale::system().toString(m_SearchStats_iNameMatched), QLocale::system().toString(m_SearchStats_iItemsFound));
    } else {
        titleString = QString(tr("%1 (%2 matches spread across %3 files of %4 searched)")).arg(QDir::toNativeSeparators(currentDirectory), QLocale::system().toString(m_SearchStats_iContentMatched), QLocale::system().toString(m_SearchStats_iNameMatched), QLocale::system().toString(m_SearchStats_iItemsFound));
    }

    setWindowTitle(titleString);

    topControlsContainerWidget->setEnabled(true);
    tableWidget->setEnabled(true);

    QGuiApplication::restoreOverrideCursor();

    m_bSearchActive = false;

    if (m_lastWidget) {
        m_lastWidget->setFocus();
    }

    qDebug() << "m_BenchmarkTimer:" << m_BenchmarkTimer.elapsed() << " ms elapsed since start of search for " << titleString;
    m_timerCalcCrc->start(100);
    m_timerUpdateIcons->start(100);
}

void MainWindow::onItemChanged(QTableWidgetItem *item) {
    if (item->column() != 0) return;

    QString oldPath = item->data(Qt::UserRole).toString();
    if (oldPath.isEmpty()) return;

    QString originalInput = item->text();
    QString cleanedName = cleanFileName(originalInput);

    QFileInfo oldInfo(oldPath);
    QString newPath = oldInfo.absolutePath() + "/" + cleanedName;

    if (oldPath != newPath) {
        if (QFile::rename(oldPath, newPath)) {
            item->setText(cleanedName);
            item->setData(Qt::UserRole, newPath); // Pfad im UserRole Bereich des Items aktualisieren
        } else {
            QMessageBox::critical(this, "Fehler", "Umbenennen fehlgeschlagen.");
            item->setText(oldInfo.fileName()); // Text zurücksetzen
        }
    } else if (cleanedName != originalInput) {
        item->setText(cleanedName);
    }
}

void MainWindow::onShowContextMenu(const QPoint &pos) {
    QTableWidgetItem *item = tableWidget->itemAt(pos);
    if (!item) return;

    int row = item->row();
    QString filePath = tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
    QFileInfo fileInfo(filePath);
    QString fileExt = fileInfo.suffix().toLower();

    QMenu menu(this);

    menu.addAction(m_actionListViewOpenFiles);
    menu.setDefaultAction(m_actionListViewOpenFiles);

    if (m_settings.audioExts.contains(fileExt) || m_settings.imageExts.contains(fileExt) || m_settings.textExts.contains(fileExt) || m_settings.videoExts.contains(fileExt)) {
        menu.addAction(m_actionListViewEditFiles);
    }

    menu.addSeparator(); //-----------------------------------------
    menu.addAction(m_actionListViewBrowseToFile);
    menu.addAction(m_actionListViewCopyPaths);
    menu.addAction(m_actionListViewCutFiles);
    menu.addAction(m_actionListViewCopyFiles);
    menu.addAction(m_actionListViewDeleteFiles);
    menu.addAction(m_actionListViewRenameFiles);
    menu.addSeparator(); //-----------------------------------------
    menu.addAction(m_actionListViewFileProperties);

    menu.exec(tableWidget->viewport()->mapToGlobal(pos));
}

// ---------------------------------------------------------------------------------------------
// Actions

QStringList MainWindow::getTablePathList() {
    QStringList pathList;

    QList<QTableWidgetItem*> selectedItems = tableWidget->selectedItems();
    if (selectedItems.isEmpty()) return pathList;

    // Zeilenindizes sammeln (verhindert Dopplungen bei Mehrfachauswahl in einer Zeile)
    QSet<int> rowSet;
    for (auto *item : std::as_const(selectedItems)) {
        rowSet.insert(item->row());
    }

    for (int row : rowSet) {
        // Wir nehmen an, der Pfad liegt in Spalte 0 in der UserRole
        QString path = tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
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
    QTableWidgetItem *item = tableWidget->currentItem();
    if (!item) return;

    QString path = tableWidget->item(item->row(), eColName)->data(Qt::UserRole).toString();
    QFileInfo fileInfo(path);

    if (!fileInfo.exists()) {
        return;
    }

    QString fileExt = fileInfo.suffix().toLower();

    if (fileExt == "desktop") {
        launchDesktopFile(fileInfo);
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

    QList<QTableWidgetItem*> selectedItems = tableWidget->selectedItems();
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
        QString fullPath = tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
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

void MainWindow::openFileListWithHandler(const QString &handlerApp, const QStringList &fileList) {
    QString appPath = handlerApp;

    if (!QFile::exists(appPath)) {
        // Doesn't seem to be an absolute path...
        qDebug() << "Error: " << handlerApp << " not found with QFile::exists('appName').";

        appPath = QStandardPaths::findExecutable(handlerApp);
        if (appPath.isEmpty()) {
            // Can't find any executables of that name in the standard paths either...
            qDebug() << "Error: " << handlerApp << " not found with QStandardPaths::findExecutable('appName').";

            QString desktopFilePath = QStandardPaths::locate(QStandardPaths::ApplicationsLocation, handlerApp);
            if (desktopFilePath.isEmpty()) {
                // Not a desktop file either...
                qDebug() << "Error: " << handlerApp << " not found with QStandardPaths::locate(QStandardPaths::ApplicationsLocation, 'appName').";
                return;
            } else {
                QSettings desktopFile(desktopFilePath, QSettings::IniFormat);
                desktopFile.beginGroup("Desktop Entry");
                QString execCommand = desktopFile.value("Exec").toString();

                // Remove placeholders like %u, %f, etc.
                execCommand.replace(QRegularExpression("%[a-zA-Z]"), "");
                QString program = execCommand.trimmed();
                QStringList arguments = fileList;

                QStringList tokens = QProcess::splitCommand(program);
                if (!tokens.isEmpty()) {
                    program = tokens.takeFirst();
                    arguments = tokens << fileList;
                }

                qDebug() << "Opening files with  QProcess::startDetached('" + program + "' , fileList)";
                QProcess::startDetached(program, arguments);
                return;
            }
        }
    }

    QString nativePath = QDir::toNativeSeparators(appPath);

    qDebug() << "Opening files with  QProcess::startDetached('" + nativePath + "' , fileList)";

    // bool QProcess::startDetached(const QString &program, const QStringList &arguments = {}, const QString &workingDirectory = QString(), qint64 *pid = nullptr)
    QProcess::startDetached(nativePath, fileList);
}

void MainWindow::action_ListViewCopyPaths() {
    QStringList pathList = getTablePathList();

    QStringList pathListNative;
    for (const QString &path : std::as_const(pathList)) {
        pathListNative << QDir::toNativeSeparators(path);
    }

    QString sClipboardList = pathListNative.join("\r\n");
    QGuiApplication::clipboard()->setText(sClipboardList);
}

void MainWindow::action_ListViewDeleteFiles(bool bRecycleOnly) {
    QList<QTableWidgetItem*> selected = tableWidget->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    //----------------------------------------------------------------------------------------------
    // 1. Collect unique rows
    QSet<int> rowSet;
    for (auto *item : std::as_const(selected)) {
        rowSet.insert(item->row());
    }
    if (rowSet.size() == 0) {
        return;
    }

    QString sSingleFilePath = "";
    if (rowSet.size() == 1) {
        int firstElement = rowSet.values().at(0);
        sSingleFilePath = tableWidget->item(firstElement, eColName)->data(Qt::UserRole).toString();
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
        QTableWidgetItem *nameItem = tableWidget->item(row, eColName);
        if (!nameItem) continue;

        QString path = nameItem->data(Qt::UserRole).toString();

        if (bRecycleOnly) {
            if (QFile::moveToTrash(path)) {
                tableWidget->removeRow(row);
            } else {
                // Fehlerbehandlung
            }
        } else {
            if (QFile::remove(path)) {
                tableWidget->removeRow(row);
            } else {
                // Fehlerbehandlung
            }
        }
    }
}

void MainWindow::action_ListViewCutFiles() {
    removeCutMarkers();

    QList<QTableWidgetItem*> selectedItems = tableWidget->selectedItems();

    // Zeilenindizes sammeln (verhindert Dopplungen bei Mehrfachauswahl in einer Zeile)
    QSet<int> rowSet;
    for (auto item : std::as_const(selectedItems)) {
        rowSet.insert(item->row());
    }

    for (int row : std::as_const(rowSet)) {
        for (int col = 0; col < tableWidget->columnCount(); ++col) {
            QTableWidgetItem* item = tableWidget->item(row, col);
            if (item) {
                item->setData(Qt::UserRole + 5, true);
            }
        }
    }

    m_rowsWithCutMarkers = rowSet;

    //tableWidget->viewport()->update();    // possibly not necessary
    setupClipboardForCut(rowSet);   // Todo: Better first try to change clipboard, and only on success ghost out cut items
}

void MainWindow::setupClipboardForCut(QSet<int> rowSet) {
    auto *mimeData = new QMimeData();
    QList<QUrl> urls;

    for (int row : rowSet) {
        // Wir nehmen an, der Pfad liegt in Spalte 0 in der UserRole
        QString path = tableWidget->item(row, eColName)->data(Qt::UserRole).toString();
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
        tableWidget->setUpdatesEnabled(false);

        for (int r : std::as_const(m_rowsWithCutMarkers)) {
            for (int c = 0; c < tableWidget->columnCount(); ++c) {
                if (QTableWidgetItem *item = tableWidget->item(r, c)) {
                    item->setData(Qt::UserRole + 5, false);
                }
            }
        }

        m_rowsWithCutMarkers.clear();
        tableWidget->setUpdatesEnabled(true);
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
    QTableWidgetItem *item = tableWidget->currentItem();
    if (!item) return;

    QString path = tableWidget->item(item->row(), eColName)->data(Qt::UserRole).toString();
#if defined(Q_OS_WIN)
    QStringList args;
    if (!m_settings.fileManager.isEmpty()) {
        QFileInfo fileInfo(path);
        QString sDir = fileInfo.dir().path();
        qDebug() << m_settings.fileManager;
        args << "-p" << QDir::toNativeSeparators(sDir) << "-f" << fileInfo.fileName();
        QProcess::startDetached(QDir::toNativeSeparators(m_settings.fileManager), args);
    } else {
        QStringList args;
        args << "/select," + QDir::toNativeSeparators(path);
        QProcess::startDetached("explorer.exe", args);
    }
#elif defined(Q_OS_LINUX)
    // Versuche es über D-Bus (funktioniert in KDE und GNOME)
    QProcess::startDetached("dbus-send", {
                                             "--session", "--print-reply", "--dest=org.freedesktop.FileManager1",
                                             "/org/freedesktop/FileManager1", "org.freedesktop.FileManager1.ShowItems",
                                             "array:string:file://" + path, "string:\"\""
                                         });
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
}

void MainWindow::action_ListViewRenameFiles() {
    QTableWidgetItem *item = tableWidget->currentItem();
    if (!item) return;

    int row = item->row();
    tableWidget->editItem(tableWidget->item(row, eColName));
}

void MainWindow::action_ListViewFileProperties() {
    QStringList pathList = getTablePathList();

    if (pathList.size() == 0) {
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
    if (m_bSearchActive.load() || (tableWidget->rowCount() == 0)) {
        return;
    }

    int firstVisible = tableWidget->rowAt(0);
    int lastVisible = tableWidget->rowAt(tableWidget->viewport()->height() - 1);    // Substract 1 pixel to make sure we're in the viewport
    if (lastVisible == -1) {
        lastVisible = tableWidget->rowCount() - 1;
    }
    if ((firstVisible == -1) || (lastVisible == -1)) {
        return;
    }

    for (int i = firstVisible; i <= lastVisible; ++i) {
        QTableWidgetItem *nameItem = tableWidget->item(i, eColName);
        if (nameItem) {
            QString fullPath = nameItem->data(Qt::UserRole).toString();
            if (!fullPath.isEmpty()) {
                if (nameItem->data(Qt::UserRole + 1).toBool() == false) {
                    nameItem->setData(Qt::UserRole + 1, true);  // set true so we don't update this item's icon.

                    QFileInfo fileInfo(fullPath);
                    QIcon trueIcon = m_iconProvider.icon(fileInfo);
                    nameItem->setIcon(trueIcon);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Misc

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == tableWidget->viewport()) {
        if (event->type() == QEvent::Resize) {
            m_timerCalcCrc->start(100);
            m_timerUpdateIcons->start(100);
        }
    }

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        //if (keyEvent->key()) qDebug() << "eventFilter: key pressed:" << keyEvent->key();

        if (keyEvent->key() == Qt::Key_Escape) {
            //qDebug() << "eventFilter: Qt::Key_Escape pressed!   m_bSearchActive=" << m_bSearchActive;
            if (m_bSearchActive) {
                m_bAbortRequested = true;
                emit abortSearchWorkerRequested();  // Gently hint to the SearchWorker that it should abort...
                //QCoreApplication::removePostedEvents(this, QEvent::MetaCall);
                qDebug() << "eventFilter: Qt::Key_Escape pressed, abort requested!  m_BenchmarkTimer:" << m_BenchmarkTimer.elapsed() << " ms elapsed since start of last search.";
                return true; // Escape "verschlucken", damit nichts anderes passiert
            }

            if (tableWidget->hasFocus()) {
                // Empty Clipboard, but only if it's our own
                const QMimeData* mimeData = QApplication::clipboard()->mimeData();
                if (mimeData->hasFormat("application/x-mkfilesearch-token") && mimeData->data("application/x-mkfilesearch-token") == m_currentClipboardToken) {
                    QApplication::clipboard()->clear();
                    // removeCutMarkers();  // Will be called automatically since we changed the clipboard
                    return true; // Escape "verschlucken"
                }
            }
        }
    }
    // WICHTIG: Alles andere an die Basisklasse weiterreichen!
    return QObject::eventFilter(obj, event);
}
