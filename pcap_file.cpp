// pcap_file.cpp
#include "pcap_file.h"
#include <QFile>
#include <QDataStream>
#include <QtEndian>

#pragma pack(push, 1)
struct PcapGlobalHeader {
    uint32_t magic_number;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t  thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t network;
};

struct PcapPacketHeader {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t incl_len;
    uint32_t orig_len;
};
#pragma pack(pop)

bool PcapFile::save(const QString &filename, const QVector<ParsedPacket> &packets) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return false;

    PcapGlobalHeader gh;
    gh.magic_number = 0xa1b2c3d4;
    gh.version_major = 2;
    gh.version_minor = 4;
    gh.thiszone = 0;
    gh.sigfigs = 0;
    gh.snaplen = 65535;
    gh.network = 1; // DLT_EN10MB

    file.write(reinterpret_cast<const char*>(&gh), sizeof(gh));

    for (const auto &pkt : packets) {
        PcapPacketHeader ph;
        ph.ts_sec = static_cast<uint32_t>(pkt.timestamp);
        ph.ts_usec = static_cast<uint32_t>((pkt.timestamp - ph.ts_sec) * 1000000);
        ph.incl_len = pkt.data.size();
        ph.orig_len = pkt.data.size();

        file.write(reinterpret_cast<const char*>(&ph), sizeof(ph));
        file.write(pkt.data);
    }

    file.close();
    return true;
}

bool PcapFile::load(const QString &filename, QVector<ParsedPacket> &packets) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return false;

    PcapGlobalHeader gh;
    if (file.read(reinterpret_cast<char*>(&gh), sizeof(gh)) != sizeof(gh)) return false;

    bool swapEndian = (gh.magic_number == 0xd4c3b2a1);
    if (gh.magic_number != 0xa1b2c3d4 && !swapEndian) return false;

    packets.clear();

    while (!file.atEnd()) {
        PcapPacketHeader ph;
        if (file.read(reinterpret_cast<char*>(&ph), sizeof(ph)) != sizeof(ph)) break;

        if (swapEndian) {
            ph.ts_sec = qbswap(ph.ts_sec);
            ph.ts_usec = qbswap(ph.ts_usec);
            ph.incl_len = qbswap(ph.incl_len);
            ph.orig_len = qbswap(ph.orig_len);
        }

        QByteArray data = file.read(ph.incl_len);
        if (data.size() != static_cast<int>(ph.incl_len)) break;

        double ts = ph.ts_sec + (ph.ts_usec / 1000000.0);
        packets.append({ts, data});
    }

    file.close();
    return true;
}