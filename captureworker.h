// captureworker.h
#ifndef CAPTUREWORKER_H
#define CAPTUREWORKER_H

#include <QThread>
#include <QByteArray>
#include <QString>

#ifdef HAVE_NPCAP
#include <pcap.h>
#endif

class CaptureWorker : public QThread {
    Q_OBJECT
public:
    explicit CaptureWorker(const QString &deviceName, QObject *parent = nullptr);
    void stop();

signals:
    void packetCaptured(QByteArray rawBytes, double timestamp);
    void errorOccurred(QString err);

protected:
    void run() override;

private:
    QString m_deviceName;
    bool m_running;
#ifdef HAVE_NPCAP
    static void packetHandler(u_char *user, const struct pcap_pkthdr *h, const u_char *bytes);
    pcap_t *m_pcapHandle;
#endif
};

#endif // CAPTUREWORKER_H