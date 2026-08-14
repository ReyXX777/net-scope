// captureworker.cpp
#include "captureworker.h"
#include <QDebug>
#include <QDateTime>

CaptureWorker::CaptureWorker(const QString &deviceName, QObject *parent)
    : QThread(parent), m_deviceName(deviceName), m_running(true)
#ifdef HAVE_NPCAP
    , m_pcapHandle(nullptr)
#endif
{}

void CaptureWorker::stop() {
    m_running = false;
#ifdef HAVE_NPCAP
    if (m_pcapHandle) pcap_breakloop(m_pcapHandle);
#endif
}

void CaptureWorker::run() {
#ifndef HAVE_NPCAP
    emit errorOccurred("Npcap not available. Build with Npcap SDK.");
    return;
#else
    char errbuf[PCAP_ERRBUF_SIZE];
    m_pcapHandle = pcap_open_live(m_deviceName.toUtf8().constData(), 65535, 1, 1000, errbuf);
    
    if (!m_pcapHandle) {
        emit errorOccurred(QString("Couldn't open device %1: %2").arg(m_deviceName).arg(errbuf));
        return;
    }

    if (pcap_datalink(m_pcapHandle) != DLT_EN10MB) {
        emit errorOccurred("This program only supports Ethernet interfaces.");
        pcap_close(m_pcapHandle);
        return;
    }

    pcap_loop(m_pcapHandle, -1, &CaptureWorker::packetHandler, reinterpret_cast<u_char*>(this));
    pcap_close(m_pcapHandle);
#endif
}

#ifdef HAVE_NPCAP
void CaptureWorker::packetHandler(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes) {
    auto *worker = reinterpret_cast<CaptureWorker*>(user);
    if (!worker->m_running) return;

    QByteArray data(reinterpret_cast<const char*>(bytes), h->caplen);
    double ts = h->ts.tv_sec + (h->ts.tv_usec / 1000000.0);
    
    emit worker->packetCaptured(data, ts);
}
#endif