#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QHostAddress>
#include <QStatusBar>
#include <QLabel>
#include <QFontDatabase>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#ifdef HAVE_NPCAP
#include <pcap.h>
#endif

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_worker(nullptr) {
    setupUI();
    setupMenuBar();
    applyTheme();
    
    m_model = new PacketModel(this);
    m_packetTable->setModel(m_model);
    m_packetTable->horizontalHeader()->setStretchLastSection(true);
    m_packetTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_packetTable->setSelectionMode(QAbstractItemView::SingleSelection);
    
    connect(m_packetTable->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex &current, const QModelIndex &) {
                onPacketSelected(current);
            });

    connect(m_detailTree, &QTreeWidget::itemClicked, this, &MainWindow::onTreeItemClicked);

    m_currentDevice = selectNetworkInterface();
    if (!m_currentDevice.isEmpty()) {
        statusBar()->showMessage(QString("Capturing on: %1").arg(m_currentDevice));
        m_worker = new CaptureWorker(m_currentDevice, this);
        connect(m_worker, &CaptureWorker::packetCaptured, this, &MainWindow::onPacketCaptured);
        connect(m_worker, &CaptureWorker::errorOccurred, this, [](QString err){
            QMessageBox::critical(nullptr, "Capture Error", err);
        });
        m_worker->start();
    } else {
        statusBar()->showMessage("No network interface selected.");
    }
}

MainWindow::~MainWindow() {
    if (m_worker) {
        m_worker->stop();
        m_worker->wait();
    }
}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(central);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    
    m_mainSplitter = new QSplitter(Qt::Vertical);
    m_bottomSplitter = new QSplitter(Qt::Horizontal);
    
    m_packetTable = new QTableView();
    m_detailTree = new QTreeWidget();
    m_hexView = new HexView();
    
    // Configure Table
    m_packetTable->setShowGrid(false);
    m_packetTable->setAlternatingRowColors(true);
    m_packetTable->verticalHeader()->setVisible(false);
    m_packetTable->verticalHeader()->setDefaultSectionSize(48);
    m_packetTable->horizontalHeader()->setHighlightSections(false);
    
    // Configure Tree
    m_detailTree->setHeaderLabels({"Field", "Value"});
    m_detailTree->setColumnWidth(0, 480);
    m_detailTree->setAlternatingRowColors(true);
    m_detailTree->setAnimated(true);
    
    // Splitter Assembly
    m_bottomSplitter->addWidget(m_detailTree);
    m_bottomSplitter->addWidget(m_hexView);
    m_bottomSplitter->setStretchFactor(0, 1);
    m_bottomSplitter->setStretchFactor(1, 1);
    
    m_mainSplitter->addWidget(m_packetTable);
    m_mainSplitter->addWidget(m_bottomSplitter);
    m_mainSplitter->setStretchFactor(0, 5);
    m_mainSplitter->setStretchFactor(1, 4);
    
    layout->addWidget(m_mainSplitter);
    setCentralWidget(central);
    
    // Status Bar configuration
    statusBar()->setSizeGripEnabled(false);
    statusBar()->showMessage("Ready");
    
    resize(1600, 1000);
    setWindowTitle("Packet Inspector — Dark Edition (Large UI)");
}

void MainWindow::setupMenuBar() {
    // --- FILE MENU ---
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    QAction *openAct = fileMenu->addAction("Open Capture...");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::onOpen);
    
    QAction *saveAct = fileMenu->addAction("Save Capture As...");
    saveAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAct, &QAction::triggered, this, &MainWindow::onSaveAs);
    
    fileMenu->addSeparator();
    
    QAction *ifaceAct = fileMenu->addAction("Select Network Interface...");
    ifaceAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(ifaceAct, &QAction::triggered, this, &MainWindow::onSelectInterface);
    
    fileMenu->addSeparator();
    
    QAction *exitAct = fileMenu->addAction("Exit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    // --- VIEW / WINDOW MENU ---
    QMenu *viewMenu = menuBar()->addMenu("&View");

    // 1. Minimize Action
    QAction *minimizeAct = viewMenu->addAction("Minimize");
    minimizeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(minimizeAct, &QAction::triggered, this, &QWidget::showMinimized);

    viewMenu->addSeparator();

    // 2. Always On Top Toggle Action
    QAction *stayOnTopAct = viewMenu->addAction("Always on Top");
    stayOnTopAct->setCheckable(true);
    stayOnTopAct->setChecked(false);
    
    connect(stayOnTopAct, &QAction::toggled, this, [this](bool checked) {
        Qt::WindowFlags flags = this->windowFlags();
        if (checked) {
            flags |= Qt::WindowStaysOnTopHint;
        } else {
            flags &= ~Qt::WindowStaysOnTopHint;
        }
        this->setWindowFlags(flags);
        this->show(); // Mandatory: Changing window flags hides the window in Qt
    });
}

void MainWindow::applyTheme() {
    QString qss = R"(
        QMainWindow, QWidget {
            background-color: #11111b;
            color: #cdd6f4;
            font-family: "JetBrains Mono", "Cascadia Code", "Consolas", "Segoe UI", sans-serif;
            font-size: 26px;
        }

        QMenuBar {
            background-color: #181825;
            color: #cdd6f4;
            padding: 8px 12px;
            border-bottom: 2px solid #313244;
        }
        QMenuBar::item {
            background: transparent;
            padding: 8px 16px;
            border-radius: 6px;
        }
        QMenuBar::item:selected {
            background-color: #313244;
            color: #89b4fa;
        }
        QMenu {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 2px solid #45475a;
            border-radius: 8px;
            padding: 8px;
        }
        QMenu::item {
            padding: 10px 32px 10px 16px;
            border-radius: 6px;
        }
        QMenu::item:selected {
            background-color: #313244;
            color: #89b4fa;
        }
        QMenu::separator {
            height: 2px;
            background-color: #313244;
            margin: 6px 0px;
        }

        QTableView {
            background-color: #1e1e2e;
            alternate-background-color: #181825;
            color: #cdd6f4;
            gridline-color: #313244;
            border: 2px solid #313244;
            border-radius: 8px;
            selection-background-color: #45475a;
            selection-color: #89b4fa;
            outline: none;
        }
        QTableView::item {
            padding: 8px 12px;
            border: none;
        }
        QTableView::item:selected {
            background-color: #313244;
            color: #89b4fa;
            font-weight: bold;
        }

        QHeaderView::section {
            background-color: #181825;
            color: #a6adc8;
            padding: 10px 16px;
            border: none;
            border-bottom: 3px solid #89b4fa;
            border-right: 2px solid #313244;
            font-weight: bold;
            font-size: 24px;
            text-transform: uppercase;
        }
        QHeaderView::section:hover {
            background-color: #313244;
            color: #89b4fa;
        }

        QTreeWidget {
            background-color: #1e1e2e;
            alternate-background-color: #181825;
            color: #cdd6f4;
            border: 2px solid #313244;
            border-radius: 8px;
            padding: 8px;
        }
        QTreeWidget::item {
            padding: 8px;
            border-radius: 4px;
        }
        QTreeWidget::item:hover {
            background-color: #313244;
        }
        QTreeWidget::item:selected {
            background-color: #45475a;
            color: #a6e3a1;
        }

        QSplitter::handle {
            background-color: #11111b;
            margin: 4px;
        }
        QSplitter::handle:horizontal {
            width: 8px;
            background-color: #313244;
            border-radius: 4px;
        }
        QSplitter::handle:vertical {
            height: 8px;
            background-color: #313244;
            border-radius: 4px;
        }
        QSplitter::handle:hover {
            background-color: #89b4fa;
        }

        QScrollBar:vertical {
            background-color: #181825;
            width: 18px;
            margin: 0px;
            border-radius: 9px;
        }
        QScrollBar::handle:vertical {
            background-color: #45475a;
            min-height: 30px;
            border-radius: 9px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #89b4fa;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: #181825;
            height: 18px;
            margin: 0px;
            border-radius: 9px;
        }
        QScrollBar::handle:horizontal {
            background-color: #45475a;
            min-width: 30px;
            border-radius: 9px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #89b4fa;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        QStatusBar {
            background-color: #181825;
            color: #a6adc8;
            border-top: 2px solid #313244;
            padding: 8px;
            font-size: 22px;
        }
    )";
    this->setStyleSheet(qss);
}

QString MainWindow::selectNetworkInterface() {
#ifdef HAVE_NPCAP
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *alldevs;
    
    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        QMessageBox::warning(this, "Error", "Could not find network interfaces.");
        return QString();
    }
    
    QStringList devices;
    QMap<QString, QString> deviceMap;
    
    for (pcap_if_t *d = alldevs; d; d = d->next) {
        QString desc = d->description ? QString(d->description) : QString(d->name);
        devices << QString("%1 (%2)").arg(desc).arg(d->name);
        deviceMap[devices.last()] = QString(d->name);
    }
    
    pcap_freealldevs(alldevs);
    
    if (devices.isEmpty()) {
        QMessageBox::warning(this, "Error", "No network interfaces found.");
        return QString();
    }
    
    bool ok;
    QString selected = QInputDialog::getItem(this, "Select Interface", "Choose network interface:", devices, 0, false, &ok);
    
    if (ok && !selected.isEmpty()) {
        return deviceMap[selected];
    }
#endif
    return QString();
}

void MainWindow::onSelectInterface() {
    if (m_worker) {
        m_worker->stop();
        m_worker->wait();
        delete m_worker;
        m_worker = nullptr;
    }
    
    m_model->clear();
    m_currentDevice = selectNetworkInterface();
    
    if (!m_currentDevice.isEmpty()) {
        statusBar()->showMessage(QString("Capturing on: %1").arg(m_currentDevice));
        m_worker = new CaptureWorker(m_currentDevice, this);
        connect(m_worker, &CaptureWorker::packetCaptured, this, &MainWindow::onPacketCaptured);
        connect(m_worker, &CaptureWorker::errorOccurred, this, [](QString err){
            QMessageBox::critical(nullptr, "Capture Error", err);
        });
        m_worker->start();
    } else {
        statusBar()->showMessage("Capture stopped. No interface selected.");
    }
}

void MainWindow::onPacketCaptured(QByteArray data, double ts) {
    m_model->addPacket(data, ts);
    m_packetTable->scrollToBottom();
}

void MainWindow::onPacketSelected(const QModelIndex &index) {
    if (!index.isValid()) return;
    PacketDisplayInfo pkt = m_model->getPacket(index.row());
    populateTree(pkt);
    m_hexView->setData(pkt.rawData);
}

void MainWindow::populateTree(const PacketDisplayInfo &pkt) {
    m_detailTree->clear();
    m_hexView->clearHighlight();
    
    const QByteArray &data = pkt.rawData;
    
    if (data.size() >= static_cast<int>(sizeof(EthernetHeader))) {
        QTreeWidgetItem *ethItem = new QTreeWidgetItem(m_detailTree);
        ethItem->setText(0, "Ethernet II");
        ethItem->setData(0, Qt::UserRole, QPoint(0, sizeof(EthernetHeader)));
        ethItem->setForeground(0, QBrush(QColor("#89b4fa")));
        
        const EthernetHeader *eth = reinterpret_cast<const EthernetHeader*>(data.constData());
        
        auto addMac = [&](const char *label, const uint8_t *mac, int off) {
            QTreeWidgetItem *item = new QTreeWidgetItem(ethItem, {
                label,
                QString("%1:%2:%3:%4:%5:%6")
                    .arg(mac[0], 2, 16, QChar('0')).arg(mac[1], 2, 16, QChar('0'))
                    .arg(mac[2], 2, 16, QChar('0')).arg(mac[3], 2, 16, QChar('0'))
                    .arg(mac[4], 2, 16, QChar('0')).arg(mac[5], 2, 16, QChar('0'))
            });
            item->setData(0, Qt::UserRole, QPoint(off, 6));
        };
        
        addMac("Dst MAC", eth->dest_mac, 0);
        addMac("Src MAC", eth->src_mac, 6);
        
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(ethItem, {"EtherType", QString("0x%1").arg(ntohs(eth->ethertype), 4, 16, QChar('0'))});
        typeItem->setData(0, Qt::UserRole, QPoint(12, 2));
        
        int offset = sizeof(EthernetHeader);
        
        if (ntohs(eth->ethertype) == 0x0800 && data.size() >= offset + static_cast<int>(sizeof(IPv4Header))) {
            QTreeWidgetItem *ipItem = new QTreeWidgetItem(m_detailTree);
            ipItem->setText(0, "Internet Protocol Version 4");
            ipItem->setData(0, Qt::UserRole, QPoint(offset, sizeof(IPv4Header)));
            ipItem->setForeground(0, QBrush(QColor("#a6e3a1")));
            
            const IPv4Header *ip = reinterpret_cast<const IPv4Header*>(data.constData() + offset);
            int ipLen = (ip->ver_ihl & 0x0F) * 4;
            
            auto addIpField = [&](const char *label, const QString &val, int off, int len) {
                QTreeWidgetItem *item = new QTreeWidgetItem(ipItem, {label, val});
                item->setData(0, Qt::UserRole, QPoint(off, len));
            };
            
            addIpField("Version/Header Length", QString("%1 / %2 bytes").arg(ip->ver_ihl >> 4).arg(ipLen), offset, 1);
            addIpField("Total Length", QString::number(ntohs(ip->total_length)), offset + 2, 2);
            addIpField("TTL", QString::number(ip->ttl), offset + 8, 1);
            addIpField("Protocol", QString::number(ip->protocol), offset + 9, 1);
            addIpField("Src IP", QHostAddress(ntohl(ip->src_ip)).toString(), offset + 12, 4);
            addIpField("Dst IP", QHostAddress(ntohl(ip->dst_ip)).toString(), offset + 16, 4);
            
            int transportOffset = offset + ipLen;
            
            if (ip->protocol == 6 && data.size() >= transportOffset + static_cast<int>(sizeof(TCPHeader))) {
                QTreeWidgetItem *tcpItem = new QTreeWidgetItem(m_detailTree);
                tcpItem->setText(0, "Transmission Control Protocol");
                tcpItem->setData(0, Qt::UserRole, QPoint(transportOffset, sizeof(TCPHeader)));
                tcpItem->setForeground(0, QBrush(QColor("#f9e2af")));
                
                const TCPHeader *tcp = reinterpret_cast<const TCPHeader*>(data.constData() + transportOffset);
                
                auto addTcpField = [&](const char *label, const QString &val, int off, int len) {
                    QTreeWidgetItem *item = new QTreeWidgetItem(tcpItem, {label, val});
                    item->setData(0, Qt::UserRole, QPoint(off, len));
                };
                
                addTcpField("Src Port", QString::number(ntohs(tcp->src_port)), transportOffset, 2);
                addTcpField("Dst Port", QString::number(ntohs(tcp->dst_port)), transportOffset + 2, 2);
                addTcpField("Seq Number", QString::number(ntohl(tcp->seq_num)), transportOffset + 4, 4);
                addTcpField("Ack Number", QString::number(ntohl(tcp->ack_num)), transportOffset + 8, 4);
                
                QString flags;
                if (tcp->flags & 0x02) flags += "SYN ";
                if (tcp->flags & 0x10) flags += "ACK ";
                if (tcp->flags & 0x01) flags += "FIN ";
                if (tcp->flags & 0x04) flags += "RST ";
                if (tcp->flags & 0x08) flags += "PSH ";
                if (tcp->flags & 0x20) flags += "URG ";
                
                addTcpField("Flags", flags.trimmed(), transportOffset + 13, 1);
                addTcpField("Window Size", QString::number(ntohs(tcp->window_size)), transportOffset + 14, 2);
                
            } else if (ip->protocol == 17 && data.size() >= transportOffset + static_cast<int>(sizeof(UDPHeader))) {
                QTreeWidgetItem *udpItem = new QTreeWidgetItem(m_detailTree);
                udpItem->setText(0, "User Datagram Protocol");
                udpItem->setData(0, Qt::UserRole, QPoint(transportOffset, sizeof(UDPHeader)));
                udpItem->setForeground(0, QBrush(QColor("#f38ba8")));
                
                const UDPHeader *udp = reinterpret_cast<const UDPHeader*>(data.constData() + transportOffset);
                
                auto addUdpField = [&](const char *label, const QString &val, int off, int len) {
                    QTreeWidgetItem *item = new QTreeWidgetItem(udpItem, {label, val});
                    item->setData(0, Qt::UserRole, QPoint(off, len));
                };
                
                addUdpField("Src Port", QString::number(ntohs(udp->src_port)), transportOffset, 2);
                addUdpField("Dst Port", QString::number(ntohs(udp->dst_port)), transportOffset + 2, 2);
                addUdpField("Length", QString::number(ntohs(udp->length)), transportOffset + 4, 2);
            }
        }
    }
    m_detailTree->expandAll();
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem *item, int) {
    QVariant data = item->data(0, Qt::UserRole);
    if (data.isValid()) {
        QPoint range = data.toPoint();
        m_hexView->setHighlight(range.x(), range.y());
    }
}

void MainWindow::onSaveAs() {
    QString filename = QFileDialog::getSaveFileName(this, "Save Capture", "", "PCAP Files (*.pcap)");
    if (!filename.isEmpty()) {
        QVector<ParsedPacket> packets = m_model->getAllParsedPackets();
        if (PcapFile::save(filename, packets)) {
            statusBar()->showMessage(QString("Saved %1 packets to %2").arg(packets.size()).arg(filename), 5000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to save file.");
        }
    }
}

void MainWindow::onOpen() {
    QString filename = QFileDialog::getOpenFileName(this, "Open Capture", "", "PCAP Files (*.pcap)");
    if (!filename.isEmpty()) {
        QVector<ParsedPacket> packets;
        if (PcapFile::load(filename, packets)) {
            m_model->setPackets(packets);
            statusBar()->showMessage(QString("Loaded %1 packets from %2").arg(packets.size()).arg(filename), 5000);
        } else {
            QMessageBox::warning(this, "Error", "Failed to load file or invalid format.");
        }
    }
}