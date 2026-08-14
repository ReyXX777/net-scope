// pcap_file.h
#ifndef PCAP_FILE_H
#define PCAP_FILE_H

#include <QString>
#include <QByteArray>
#include <QVector>
#include <cstdint>

struct ParsedPacket {
    double timestamp;
    QByteArray data;
};

class PcapFile {
public:
    static bool save(const QString &filename, const QVector<ParsedPacket> &packets);
    static bool load(const QString &filename, QVector<ParsedPacket> &packets);
};

#endif // PCAP_FILE_H