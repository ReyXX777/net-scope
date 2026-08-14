// packetmodel.cpp
#include "packetmodel.h"
#include <QHostAddress>
#include <QDateTime>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

PacketModel::PacketModel(QObject *parent) : QAbstractTableModel(parent) {
    m_startTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
}

void PacketModel::addPacket(const QByteArray &data, double ts) {
    beginInsertRows(QModelIndex(), m_packets.size(), m_packets.size());
    parseAndAppend(data, ts);
    endInsertRows();
}

void PacketModel::setPackets(const QVector<ParsedPacket> &packets) {
    beginResetModel();
    m_packets.clear();
    if (!packets.isEmpty()) {
        m_startTime = packets.first().timestamp;
        for (const auto &p : packets) {
            parseAndAppend(p.data, p.timestamp);
        }
    }
    endResetModel();
}

void PacketModel::clear() {
    beginResetModel();
    m_packets.clear();
    endResetModel();
}

void PacketModel::parseAndAppend(const QByteArray &data, double ts) {
    PacketDisplayInfo pkt;
    pkt.index = m_packets.size() + 1;
    pkt.timestamp = ts - m_startTime;
    pkt.length = data.size();
    pkt.rawData = data;
    pkt.protocol = "Unknown";
    pkt.info = "";
    pkt.color = Qt::white;

    if (data.size() >= static_cast<int>(sizeof(EthernetHeader))) {
        const EthernetHeader *eth = reinterpret_cast<const EthernetHeader*>(data.constData());
        uint16_t type = ntohs(eth->ethertype);
        
        if (type == 0x0800 && data.size() >= static_cast<int>(sizeof(EthernetHeader) + sizeof(IPv4Header))) {
            const IPv4Header *ip = reinterpret_cast<const IPv4Header*>(data.constData() + sizeof(EthernetHeader));
            pkt.src = QHostAddress(ntohl(ip->src_ip)).toString();
            pkt.dst = QHostAddress(ntohl(ip->dst_ip)).toString();
            
            int ipHeaderLen = (ip->ver_ihl & 0x0F) * 4;
            int transportOffset = sizeof(EthernetHeader) + ipHeaderLen;
            
            if (ip->protocol == 6) {
                pkt.protocol = "TCP";
                pkt.color = QColor(0xE0, 0xF0, 0xFF);
                if (data.size() >= transportOffset + static_cast<int>(sizeof(TCPHeader))) {
                    const TCPHeader *tcp = reinterpret_cast<const TCPHeader*>(data.constData() + transportOffset);
                    QString flags;
                    if (tcp->flags & 0x02) flags += "SYN ";
                    if (tcp->flags & 0x10) flags += "ACK ";
                    if (tcp->flags & 0x01) flags += "FIN ";
                    if (tcp->flags & 0x04) flags += "RST ";
                    if (tcp->flags & 0x08) flags += "PSH ";
                    
                    pkt.info = QString("%1 → %2 [%3]")
                        .arg(ntohs(tcp->src_port))
                        .arg(ntohs(tcp->dst_port))
                        .arg(flags.trimmed());
                }
            } else if (ip->protocol == 17) {
                pkt.protocol = "UDP";
                pkt.color = QColor(0xE0, 0xFF, 0xE0);
                if (data.size() >= transportOffset + static_cast<int>(sizeof(UDPHeader))) {
                    const UDPHeader *udp = reinterpret_cast<const UDPHeader*>(data.constData() + transportOffset);
                    pkt.info = QString("%1 → %2 Len=%3")
                        .arg(ntohs(udp->src_port))
                        .arg(ntohs(udp->dst_port))
                        .arg(ntohs(udp->length) - 8);
                }
            } else if (ip->protocol == 1) {
                pkt.protocol = "ICMP";
                pkt.color = QColor(0xF0, 0xF0, 0xF0);
                pkt.info = "Echo Request/Reply";
            }
        }
    }

    m_packets.append(pkt);
}

int PacketModel::rowCount(const QModelIndex &) const { return m_packets.size(); }
int PacketModel::columnCount(const QModelIndex &) const { return 6; }

QVariant PacketModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    
    if (role == Qt::BackgroundRole) {
        return m_packets[index.row()].color;
    }
    
    if (role != Qt::DisplayRole) return QVariant();
    
    const PacketDisplayInfo &pkt = m_packets[index.row()];
    switch (index.column()) {
        case 0: return pkt.index;
        case 1: return QString::number(pkt.timestamp, 'f', 6);
        case 2: return pkt.src;
        case 3: return pkt.dst;
        case 4: return pkt.protocol;
        case 5: return pkt.info;
    }
    return QVariant();
}

QVariant PacketModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return "No.";
            case 1: return "Time";
            case 2: return "Source";
            case 3: return "Destination";
            case 4: return "Protocol";
            case 5: return "Info";
        }
    }
    return QVariant();
}

PacketDisplayInfo PacketModel::getPacket(int row) const {
    if (row >= 0 && row < m_packets.size()) return m_packets[row];
    return PacketDisplayInfo();
}

QVector<ParsedPacket> PacketModel::getAllParsedPackets() const {
    QVector<ParsedPacket> result;
    for (const auto &p : m_packets) {
        result.append({p.timestamp + m_startTime, p.rawData});
    }
    return result;
}