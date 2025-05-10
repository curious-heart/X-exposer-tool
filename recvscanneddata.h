
#ifndef RECVSCANNEDDATA_H
#define RECVSCANNEDDATA_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QQueue>
#include <QByteArray>
#include <QHostAddress>
#include <QMutex>

#include "logger/logger.h"
#include "sc_data_proc.h"

#define COLLECT_ST_E(e) e
#define COLLECT_ST_LIST \
    COLLECT_ST_E(ST_IDLE), \
    COLLECT_ST_E(ST_WAIT_CONN_ACK), \
    COLLECT_ST_E(ST_COLLECTING), \
    COLLECT_ST_E(ST_WAIT_DISCONN_ACK)

typedef enum
{
    COLLECT_ST_LIST
}CollectState_e_t;

typedef enum
{
    COLLECT_RPT_EVT_IGNORE,
    COLLECT_RPT_EVT_CONNECTED,
    COLLECT_RPT_EVT_DISCONNECTED,
    COLLECT_RPT_EVT_CONN_TIMEOUT,
    COLLECT_RPT_EVT_DISCONN_TIMEOUT,
}collect_rpt_evt_e_t;
Q_DECLARE_METATYPE(collect_rpt_evt_e_t)

class RecvScannedData : public QObject
{
    Q_OBJECT

public:
    explicit RecvScannedData(QQueue<recv_data_with_notes_s_t> *queue, QMutex *mutex,
                             QObject *parent = nullptr, quint16 localPort = 0);
    ~RecvScannedData();

signals:
    void new_data_ready();
    void recv_worker_report_sig(LOG_LEVEL lvl, QString report_str,
                                collect_rpt_evt_e_t evt = COLLECT_RPT_EVT_IGNORE);

public slots:
    void start_collect_sc_data(QString ip, quint16 port, int connTimeout, int packetCount);
    void stop_collect_sc_data();
    void data_ready_hdlr();
    void conn_timeout_hdlr();
    void disconn_timeout_hdlr();

private:
    QUdpSocket *udpSocket;
    QQueue<recv_data_with_notes_s_t> *dataQueue = nullptr;
    QMutex *queueMutex = nullptr;
    QHostAddress remoteAddress;
    quint16 remotePort, m_localPort;
    int m_connTimeout;

    QTimer *connTimer;
    QTimer *discTimer;

    int expectedPacketCount = 0;
    int receivedPacketCount = 0;
    CollectState_e_t collectingState = ST_IDLE;

    QByteArray m_start_req, m_start_ack, m_stop_req, m_stop_ack;

    void stopCollection();

    bool is_from_proper_peer(QHostAddress &rmt_addr, quint16 rmt_port);
};

#endif // RECVSCANNEDDATA_H
