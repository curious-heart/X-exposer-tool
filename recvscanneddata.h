
#ifndef RECVSCANNEDDATA_H
#define RECVSCANNEDDATA_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QQueue>
#include <QByteArray>
#include <QHostAddress>
#include <QMutex>

class RecvScannedData : public QObject
{
    Q_OBJECT

public:
    enum CollectState {
        ST_IDLE,
        ST_WAIT_CONN_ACK,
        ST_COLLECTING,
        ST_WAIT_DISCONN_ACK
    };

    explicit RecvScannedData(QQueue<QByteArray> *queue, QMutex *mutex,
                             QObject *parent = nullptr, quint16 localPort = 0);

signals:
    void conn_timeout();
    void discconn_timeout();
    void new_data_ready();

public slots:
    void start_collect_sc_data(QString ip, quint16 port, int connTimeout, int packetCount);
    void stop_collect_sc_data();

private:
    QUdpSocket udpSocket;
    QQueue<QByteArray> *dataQueue = nullptr;
    QMutex *queueMutex = nullptr;
    QHostAddress remoteAddress;
    quint16 remotePort, m_localPort;
    int m_connTimeout;

    QTimer connTimer;
    QTimer discTimer;

    int expectedPacketCount = 0;
    int receivedPacketCount = 0;
    CollectState collectingState = ST_IDLE;

    QByteArray m_start_req, m_start_ack, m_stop_req, m_stop_ack;

    void stopCollection();

    bool is_from_proper_peer(QHostAddress &rmt_addr, quint16 rmt_port);
};

#endif // RECVSCANNEDDATA_H
