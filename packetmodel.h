// packetmodel.h
#ifndef PACKETMODEL_H
#define PACKETMODEL_H

#include <QAbstractTableModel>
#include <QVector>
#include <QColor>
#include "pcap_file.h"
#include "packet_structs.h"

struct PacketDisplayInfo {
    int index;
    double timestamp;
    QString src;
    QString dst;
    QString protocol;
    int length;
    QString info;
    QColor color;
    QByteArray rawData;
};

class PacketModel : public QAbstractTableModel {
    Q_OBJECT
public:
    explicit PacketModel(QObject *parent = nullptr);
    void addPacket(const QByteArray &data, double ts);
    void setPackets(const QVector<ParsedPacket> &packets);
    void clear();
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    PacketDisplayInfo getPacket(int row) const;
    QVector<ParsedPacket> getAllParsedPackets() const;

private:
    void parseAndAppend(const QByteArray &data, double ts);
    QVector<PacketDisplayInfo> m_packets;
    double m_startTime;
};

#endif // PACKETMODEL_H