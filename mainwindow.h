#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidget>
#include <QLineEdit>
#include <QKeyEvent>
#include <QFileIconProvider>
#include <QCheckBox>
#include <QElapsedTimer>
#include <QPointer>
#include <QQueue>
#include <QTimer>
#include <QSet>
#include <QPainter>             // Added for cut item opacity drawing
#include <QStyledItemDelegate>  // Added for cut item opacity drawing

#include "searchworker.h"
#include "settingsmanager.h"

/*
 * [QTableWidget Qt::UserRole usage]
 *
 *  eColName	Qt::UserRole		QString absoluteFilePath
 *  eColSize	Qt::UserRole + 1	bool trueIcon
 *  eColSize	Qt::UserRole		qint64 sizeInBytes
 *  any item	Qt::UserRole + 5	bool isCut
*/

// Subclass QTableWidget as Custom_QTableWidget where F2 key presses are diverted to the first row's item
class Custom_QTableWidget : public QTableWidget {
protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_F2) {
            int row = currentRow();
            if (row != -1) {
                setCurrentCell(row, 0);     // Den Fokus explizit auf die erste Spalte setzen
                editItem(item(row, 0));     // Den Editor manuell starten
                return;
            }
        }

        QTableWidget::keyPressEvent(event); // Alle anderen Tasten normal verarbeiten
    }
};

// Subclass QTableWidgetItem for sorting the filesize column by the data hidden with UserRole
class SizeTableItem : public QTableWidgetItem {
public:
    SizeTableItem(const QString &text) : QTableWidgetItem(text) {}

    bool operator<(const QTableWidgetItem &other) const override {
        QVariant v1 = this->data(Qt::UserRole);
        QVariant v2 = other.data(Qt::UserRole);

        if (v1.isValid() && v2.isValid()) {
            return v1.toLongLong() < v2.toLongLong();
        }

        return QTableWidgetItem::operator<(other);  // Fallback auf Standard-Textvergleich
    }
};


class CutDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        // Wir nutzen eine benutzerdefinierte Daten-Rolle (z.B. Qt::UserRole + 5),
        // um zu prüfen, ob dieses Item "ausgeschnitten" ist.
        bool isCut = index.data(Qt::UserRole + 5).toBool();

        if (isCut) {
            painter->save();
            painter->setOpacity(0.50);
            QStyledItemDelegate::paint(painter, option, index);
            painter->restore();
        } else {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void abortSearchWorkerRequested();

public:
    explicit MainWindow(const QString &targetDirectory, QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onItemChanged(QTableWidgetItem *item);
    void onCheckboxClickedCRC(Qt::CheckState state);
    void onCheckboxClickedRegExName(Qt::CheckState state);
    void onCheckboxClickedRegExContent(Qt::CheckState state);
    void onClipboardChanged();
    void onListViewHeaderClicked();
    void onListViewItemDoubleClicked(QTableWidgetItem *item);
    void onShowContextMenu(const QPoint &pos);
    void onTimedCalcCRC();
    void onTimedUpdateIcons();
    void onVerticalBarScrollChange();

private:
    enum Column {
        eColName = 0,
        eColSubpath = 1,
        eColSize = 2,
        eColDate = 3,
        eColType = 4,
        eColQuality = 5,
        eColCount = 6,
        eColCRC = 7,
    };

    QCheckBox *CheckboxRegExContent;
    QCheckBox *CheckboxRegExName;
    QCheckBox *CheckboxNameCaseSense;
    QCheckBox *CheckboxContentCaseSense;
    QCheckBox *CheckboxDirectories;
    QCheckBox *CheckboxCRC;
    QElapsedTimer m_BenchmarkTimer;
    QFileIconProvider m_iconProvider;
    QLineEdit *InputBox1;
    QLineEdit *InputBox2;
    QString currentDirectory;
    QTableWidget *tableWidget;
    QTimer *m_timerCalcCrc;
    QTimer *m_timerUpdateIcons;
    QWidget *topControlsContainerWidget;

    QAction *m_actionListViewOpenFiles;
    QAction *m_actionListViewEditFiles;
    QAction *m_actionListViewBrowseToFile;
    QAction *m_actionListViewCopyPaths;
    QAction *m_actionListViewCutFiles;
    QAction *m_actionListViewCopyFiles;
    QAction *m_actionListViewDeleteFiles;
    QAction *m_actionListViewRenameFiles;
    QAction *m_actionListViewFileProperties;

    QStringList getTablePathList();
    void action_EditSettingsFile();
    void action_ListViewBrowseToFile();
    void action_ListViewCopyFiles();
    void action_ListViewCopyPaths();
    void action_ListViewCutFiles();
    void action_ListViewDeleteFiles(bool bRecycleOnly);
    void action_ListViewEditFiles();
    void action_ListViewFileProperties();
    void action_ListViewOpenFiles();
    void action_ListViewRenameFiles();
    void addFileToTable(const QFileInfo fileInfo, int iRow, int iLenRem, int nameMatchQuality, int contentMatchCount);
    void finalizeUI();
    void onWorkerFinished(uint iItemsFound, uint iNameMatched, uint iContentMatched, bool bSearchInterrupted);
    void onWorkerSentBatch(const QList<SearchResult> &results);
    void openFileListWithHandler(const QString &handler, const QStringList &fileList);
    void removeCutMarkers();
    void setupClipboardForCut(QSet<int> rowSet);
    void startSearch();
    void validateInputBoxRegex();


    QPointer<QWidget> m_lastWidget;

    uint m_SearchStats_iItemsFound;
    uint m_SearchStats_iNameMatched;
    uint m_SearchStats_iContentMatched;
    bool m_SearchStats_bSearchInterrupted;

    std::atomic<bool> m_bSearchActive{false};
    std::atomic<bool> m_bAbortRequested{false};
    std::atomic<bool> m_workerHasFinished{false};
    std::atomic<int> m_currentSearchGeneration{0};
    QHash<QString, QIcon> m_iconCache;

    QQueue<QList<SearchResult>> m_pendingBatches;
    bool m_isProcessingPending = false;
    QByteArray m_currentClipboardToken;
    QSet<int> m_rowsWithCutMarkers;

    SettingsManager m_settings;


protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};
#endif // MAINWINDOW_H

