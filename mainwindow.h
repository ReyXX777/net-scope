#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QTreeWidget>
#include <QSplitter>
#include <QMenuBar>
#include <QAction>
#include "packetmodel.h"
#include "captureworker.h"
#include "hexview.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPacketCaptured(QByteArray data, double ts);
    void onPacketSelected(const QModelIndex &index);
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onSaveAs();
    void onOpen();
    void onSelectInterface();

private:
    void setupUI();
    void setupMenuBar();
    void applyTheme();
    void populateTree(const PacketDisplayInfo &pkt);
    QString selectNetworkInterface();

    QSplitter *m_mainSplitter;
    QSplitter *m_bottomSplitter;
    QTableView *m_packetTable;
    QTreeWidget *m_detailTree;
    HexView *m_hexView;
    
    PacketModel *m_model;
    CaptureWorker *m_worker;
    QString m_currentDevice;
};

#endif // MAINWINDOW_H