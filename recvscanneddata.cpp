#include "recvscanneddata.h"
#include <QNetworkDatagram>
#include <QDebug>
#include <QMutexLocker>

#include "logger/logger.h"

static const char gs_start_req_cmd[] = {'\x00', '\x00', '\x00', '\x02'};
static const char gs_start_ack_cmd[] = {'\x00', '\x00', '\x00', '\x02'};
static const char gs_stop_req_cmd[] = {'\x00', '\x00', '\x00', '\x03'};
static const char gs_stop_ack_cmd[] = {'\x00', '\x00', '\x00', '\x03'};

static const char* gs_def_remote_ip_addr = "192.168.1.123";
static const quint16 gs_def_remote_udp_port = 8020;

RecvScannedData::RecvScannedData(QQueue<QByteArray> *queue, QMutex *mutex,
                                  QObject *parent, quint16 localPort)
    : QObject(parent), dataQueue(queue), queueMutex(mutex),
      remoteAddress(gs_def_remote_ip_addr), remotePort(gs_def_remote_udp_port),
      m_localPort(localPort)
{
    m_start_req = QByteArray::fromRawData(gs_start_req_cmd, sizeof(gs_start_req_cmd));
    m_start_ack = QByteArray::fromRawData(gs_start_ack_cmd, sizeof(gs_start_ack_cmd));
    m_stop_req = QByteArray::fromRawData(gs_stop_req_cmd, sizeof(gs_stop_req_cmd));
    m_stop_ack = QByteArray::fromRawData(gs_stop_ack_cmd, sizeof(gs_stop_ack_cmd));

    if (!udpSocket.bind(QHostAddress::AnyIPv4, m_localPort))
    {
        DIY_LOG(LOG_WARN, QString("Failed to bind UDP socket on port %1").arg(m_localPort));
    }

    connect(&udpSocket, &QUdpSocket::readyRead, this, [this]()
    {
        while (udpSocket.hasPendingDatagrams())
        {
            QNetworkDatagram datagram = udpSocket.receiveDatagram();
            QHostAddress rmt_addr = datagram.senderAddress();
            QString rmt_addr_str = rmt_addr.toString();
            quint16 rmt_port = datagram.senderPort();

            if(!is_from_proper_peer(rmt_addr, rmt_port))
            {
                DIY_LOG(LOG_WARN, QString("receive data from %1:%2, discard it.")
                                        .arg(rmt_addr.toString()));
                continue;
            }

            QByteArray data = datagram.data();

            switch(collectingState)
            {
            case ST_IDLE:
                DIY_LOG(LOG_WARN, "receive data in idle state. discard it.");
                break;

            case ST_WAIT_CONN_ACK:
                if (data == m_start_ack)
                {
                    collectingState = ST_COLLECTING;
                    connTimer.stop();
                }
                else
                {
                    DIY_LOG(LOG_WARN, "receive data in wait-conn-ack state. discard it.");
                }
                break;

            case ST_COLLECTING:
                {
                    QMutexLocker locker(queueMutex);
                    dataQueue->enqueue(data);
                }
                receivedPacketCount++;
                emit new_data_ready();
                if (receivedPacketCount >= expectedPacketCount)
                {
                    stopCollection();
                }
                break;

            default: //ST_WAIT_DISCONN_ACK
                if (data == m_stop_ack)
                {
                    collectingState = ST_IDLE;
                    discTimer.stop();
                }
                else
                {
                    DIY_LOG(LOG_WARN, "receive data in ST_WAIT_DISCONN_ACK but it is not "
                                      "expected ack. discard it");
                }
                break;
            }
        }
    });

    connect(&connTimer, &QTimer::timeout, this, [this]()
    {
        if (collectingState == ST_WAIT_CONN_ACK)
        {
            collectingState = ST_IDLE;
            emit conn_timeout();
            DIY_LOG(LOG_WARN, "start collect: receive ack timeout.");
        }
    });

    connect(&discTimer, &QTimer::timeout, this, [this]()
    {
        if (collectingState == ST_WAIT_DISCONN_ACK)
        {
            collectingState = ST_IDLE;
            emit discconn_timeout();
            DIY_LOG(LOG_WARN, "stop collect: receive ack timeout.");
        }
    });
}

void RecvScannedData::start_collect_sc_data(QString ip, quint16 port,
                                            int connTimeout, int packetCount)
{
    if (collectingState != ST_IDLE)
    {
        DIY_LOG(LOG_WARN, "Current state is not idle.");
        return;
    }
    m_connTimeout = connTimeout;

    remoteAddress = QHostAddress(ip);
    remotePort = port;
    expectedPacketCount = packetCount;
    receivedPacketCount = 0;

    udpSocket.writeDatagram(m_start_req, remoteAddress, remotePort);

    collectingState = ST_WAIT_CONN_ACK;
    connTimer.start(connTimeout * 1000);
}

void RecvScannedData::stop_collect_sc_data()
{
    if (collectingState != ST_COLLECTING)
        return;

    stopCollection();
}

void RecvScannedData::stopCollection()
{
    udpSocket.writeDatagram(m_stop_req, remoteAddress, remotePort);

    collectingState = ST_WAIT_DISCONN_ACK;
    discTimer.start(m_connTimeout * 1000);

    connTimer.stop();
}

bool RecvScannedData::is_from_proper_peer(QHostAddress &rmt_addr, quint16 rmt_port)
{
    return (rmt_addr == remoteAddress && rmt_port == remotePort);

}
