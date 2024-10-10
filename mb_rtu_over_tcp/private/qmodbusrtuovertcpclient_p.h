#ifndef QMODBUSRTUOVERTCPCLIENT_P_H
#define QMODBUSRTUOVERTCPCLIENT_P_H

#include <QtNetwork/qhostaddress.h>
#include <QtNetwork/qtcpsocket.h>
#include "private/qmodbusclient_p.h"

#include <private/qmodbusadu_p.h>
#include <private/qmodbus_symbols_p.h>

#include <QBasicTimer>
#include <QQueue>

#include "logger/logger.h"
#include "../qmodbusrtuovertcpclient.h"

QT_BEGIN_NAMESPACE

class Timer : public QObject
{
    Q_OBJECT

public:
    Timer() = default;
    int start(int msec)
    {
        m_timer = QBasicTimer();
        m_timer.start(msec, Qt::PreciseTimer, this);
        return m_timer.timerId();
    }
    void stop() { m_timer.stop(); }
    bool isActive() const { return m_timer.isActive(); }

signals:
    void timeout(int timerId);

private:
    void timerEvent(QTimerEvent *event) override
    {
        const auto id = m_timer.timerId();
        if (event->timerId() == id)
            emit timeout(id);
    }

private:
    QBasicTimer m_timer;
};

class ProcessNextReq: public QObject
{
    Q_OBJECT
signals:
    void processNextReq_sig();
public:
    void send_signal()
    {
        emit processNextReq_sig();
    }
};

class QModbusRtuOverTcpClientPrivate:public QModbusClientPrivate
{
    Q_DECLARE_PUBLIC(QModbusRtuOverTcpClient)

    enum State
    {
        Idle,
        WaitingForReplay,
        ProcessReply
    } m_state = Idle;

public:
    /*from qmodbusclient_p.h*/
    bool processResponse(const QModbusResponse &response, QModbusDataUnit *data);

    bool processReadCoilsResponse(const QModbusResponse &response, QModbusDataUnit *data);
    bool processReadDiscreteInputsResponse(const QModbusResponse &response, QModbusDataUnit *data);
    bool collateBits(const QModbusPdu &pdu, QModbusDataUnit::RegisterType type, QModbusDataUnit *data);

    bool processReadHoldingRegistersResponse(const QModbusResponse &response, QModbusDataUnit *data);
    bool processReadInputRegistersResponse(const QModbusResponse &response, QModbusDataUnit *data);
    bool collateBytes(const QModbusPdu &pdu, QModbusDataUnit::RegisterType type, QModbusDataUnit *data);

    bool processWriteSingleCoilResponse(const QModbusResponse &response, QModbusDataUnit *data);
    bool processWriteSingleRegisterResponse(const QModbusResponse &response,
                                            QModbusDataUnit *data);
    bool collateSingleValue(const QModbusPdu &pdu, QModbusDataUnit::RegisterType type,
                         QModbusDataUnit *data);

    bool processWriteMultipleCoilsResponse(const QModbusResponse &response, QModbusDataUnit *data);
    bool processWriteMultipleRegistersResponse(const QModbusResponse &response,
                                               QModbusDataUnit *data);
    bool collateMultipleValues(const QModbusPdu &pdu, QModbusDataUnit::RegisterType type,
                          QModbusDataUnit *data);

    bool processReadWriteMultipleRegistersResponse(const QModbusResponse &response,
                                                  QModbusDataUnit *data);

    void processQueueElement(const QModbusResponse &pdu, const QueueElement &element);

    /*from qmodbustcpclient_p.h, modified.*/
    void setupTcpSocket()
    {
        Q_Q(QModbusRtuOverTcpClient);

        m_socket = new QTcpSocket(q);

        QObject::connect(&m_proc_req_sig_helper, &ProcessNextReq::processNextReq_sig,
                         q, [this]() {processQueue();});

        QObject::connect(&m_responseTimer, &Timer::timeout, q, [this](int timerId) {
            onResponseTimeout(timerId);
        });

        QObject::connect(m_socket, &QAbstractSocket::connected, q, [this]() {
            DIY_LOG(LOG_DEBUG, QString("(Rtu over TCP client) Connected to %1 on port %2.")
                   .arg(m_socket->peerAddress().toString()).arg(m_socket->peerPort()));
            Q_Q(QModbusRtuOverTcpClient);
            responseBuffer.clear();
            q->setState(QModbusDevice::ConnectedState);
        });

        QObject::connect(m_socket, &QAbstractSocket::disconnected, q, [this]() {
           DIY_LOG(LOG_DEBUG, "(Rtu over TCP client) Connection closed.");
           Q_Q(QModbusRtuOverTcpClient);
           q->setState(QModbusDevice::UnconnectedState);
        });

        QObject::connect(m_socket,
                         &QAbstractSocket::errorOccurred, q,
                         [this](QAbstractSocket::SocketError /*error*/)
        {
            Q_Q(QModbusRtuOverTcpClient);

            if (m_socket->state() == QAbstractSocket::UnconnectedState) {
                q->setState(QModbusDevice::UnconnectedState);
            }
            q->setError(QModbusClient::tr("TCP socket error (%1).").arg(m_socket->errorString()),
                        QModbusDevice::ConnectionError);
        });

        QObject::connect(m_socket, &QIODevice::readyRead, q, [this](){
            responseBuffer += m_socket->read(m_socket->bytesAvailable());
            DIY_LOG(LOG_DEBUG,QString("(Rtu over TCP client) Response buffer: %1")
                    .arg(QString(responseBuffer.toHex()), 0, QLatin1Char('0')));

            // can we read enough for Modbus ADU header?
            if (responseBuffer.size() < m_min_adu_byte_num) {
                DIY_LOG(LOG_DEBUG, "(Rtu over TCP client) Modbus ADU not complete");
                return;
            }

            const QModbusSerialAdu tmpAdu(QModbusSerialAdu::Rtu, responseBuffer);
            int pduSizeWithoutFcode = QModbusResponse::calculateDataSize(tmpAdu.pdu());
            if (pduSizeWithoutFcode < 0) {
                // wait for more data
                DIY_LOG(LOG_DEBUG,
                        QString("(Rtu over TCP client) Cannot calculate PDU size for"
                                "function code: %1, delaying pending frame")
                                .arg(tmpAdu.pdu().functionCode()));
                return;
            }

            // server address byte + function code byte + PDU size + 2 bytes CRC
            int aduSize = m_srv_addr_plus_fc_byte_num + pduSizeWithoutFcode + m_rtu_crc_byte_num;
            if (tmpAdu.rawSize() < aduSize) {
                DIY_LOG(LOG_DEBUG, "(Rtu over TCP client) Incomplete ADU received, ignoring");
                return;
            }

            if (m_queue.isEmpty())
                return;

            auto &current = m_queue.first();

            // Special case for Diagnostics:ReturnQueryData. The response has no
            // length indicator and is just a simple echo of what we have send.
            if (tmpAdu.pdu().functionCode() == QModbusPdu::Diagnostics) {
                const QModbusResponse response = tmpAdu.pdu();
                if (canMatchRequestAndResponse(response, tmpAdu.serverAddress())) {
                    quint16 subCode = 0xffff;
                    response.decodeData(&subCode);
                    if (subCode == Diagnostics::ReturnQueryData) {
                        if (response.data() != current.requestPdu.data())
                            return; // echo does not match request yet
                        aduSize =  m_srv_addr_plus_fc_byte_num + response.dataSize() + m_rtu_crc_byte_num;
                        if (tmpAdu.rawSize() < aduSize)
                            return; // echo matches, probably checksum missing
                    }
                }
            }

            const QModbusSerialAdu adu(QModbusSerialAdu::Rtu, responseBuffer.left(aduSize));
            responseBuffer.remove(0, aduSize);

            DIY_LOG(LOG_DEBUG, QString("(Rtu over TCP client) Received ADU: %1")
                        .arg(QString(adu.rawData().toHex()), 0, QLatin1Char('0')));

            if (!responseBuffer.isEmpty())
                DIY_LOG(LOG_DEBUG, QString("(Rtu over TCP client) Pending buffer:")
                        .arg(QString(responseBuffer.toHex()), 0, QLatin1Char('0')));

            // check CRC
            if (!adu.matchingChecksum()) {
                DIY_LOG(LOG_WARN,
                        QString("(Rtu over TCP client) Discarding response with wrong CRC,"
                                " received: %1, calculated CRC: %2")
                        .arg(adu.checksum<quint16>())
                        .arg(QModbusSerialAdu::calculateCRC(adu.data(), adu.size())));
                return;
            }

            const QModbusResponse response = adu.pdu();
            if (!canMatchRequestAndResponse(response, adu.serverAddress())) {
                DIY_LOG(LOG_WARN,  "(Rtu over TCP client) Cannot match response with open request, ignoring");
                return;
            }

            m_state = ProcessReply;
            m_responseTimer.stop();
            current.m_timerId = INT_MIN;

            processQueueElement(response, m_queue.dequeue());

            m_state = Idle;

            m_proc_req_sig_helper.send_signal();
        });
    }

    bool writeToSocket(const QByteArray &adu_buffer)
    {
        int writtenBytes = m_socket->write(adu_buffer);
        if (writtenBytes == -1 || writtenBytes < adu_buffer.size()) {
            Q_Q(QModbusRtuOverTcpClient);
            DIY_LOG(LOG_DEBUG, "(Rtu over TCP client) Cannot write request to socket.");
            q->setError(QModbusRtuOverTcpClient::tr("Could not write request to socket."),
                        QModbusDevice::WriteError);
            return false;
        }
        DIY_LOG(LOG_DEBUG, QString("(Rtu over TCP client) Sent RTU ADU (over TCP): %1")
                .arg(QString(adu_buffer.toHex()), 0, QLatin1Char('0')));
        return true;
    }

    void onResponseTimeout(int timerId)
    {
        m_responseTimer.stop();
        if (m_state != State::WaitingForReplay || m_queue.isEmpty())
            return;
        const auto current = m_queue.first();

        if (current.m_timerId != timerId)
            return;

        DIY_LOG(LOG_DEBUG, QString("(RTU client) Receive timeout: %1")
                    .arg(QString(current.requestPdu.data().toHex()), 0, QLatin1Char('0')));

        if (current.numberOfRetries <= 0) {
            auto item = m_queue.dequeue();
            if (item.reply) {
                item.reply->setError(QModbusDevice::TimeoutError,
                    QModbusClient::tr("Request timeout."));
            }
        }

        m_state = Idle;

        m_proc_req_sig_helper.send_signal();
    }

    QModbusReply *enqueueRequest(const QModbusRequest &request, int serverAddress,
                                 const QModbusDataUnit &unit,
                                 QModbusReply::ReplyType type) override
    {
        Q_Q(QModbusRtuOverTcpClient);
        auto reply = new QModbusReply(serverAddress == 0 ? QModbusReply::Broadcast : type,
                                      serverAddress, q);
        QueueElement element = QueueElement{ reply, request, unit, m_numberOfRetries};
        element.adu = QModbusSerialAdu::create(QModbusSerialAdu::Rtu, serverAddress, request);
        m_queue.enqueue(element);

        processQueue();

        return reply;
    }

    bool canMatchRequestAndResponse(const QModbusResponse &response, int sendingServer) const
    {
        if (m_queue.isEmpty())
            return false;
        const auto &current = m_queue.first();

        if (current.reply.isNull())
            return false;   // reply deleted
        if (current.reply->serverAddress() != sendingServer)
            return false;   // server mismatch
        if (current.requestPdu.functionCode() != response.functionCode())
            return false;   // request for different function code
        return true;
    }

    bool isOpen() const override
    {
        if (m_socket)
            return m_socket->isOpen();
        return false;
    }
    QIODevice *device() const override { return m_socket; }

    QTcpSocket *m_socket = nullptr;
    Timer m_responseTimer;
    QByteArray responseBuffer;
    QQueue<QueueElement> m_queue;

public:
    void processQueue()
    {
        if (Idle != m_state)
            return;

        while(!m_queue.isEmpty())
        {
            auto &current = m_queue.first();
            if(!current.reply.isNull()) break;
            m_queue.dequeue();
        }
        if(m_queue.isEmpty()) return;

        m_state = WaitingForReplay;

        auto &current = m_queue.first();
        current.numberOfRetries--;
        bool ret = writeToSocket(current.adu);
        current.m_timerId = m_responseTimer.start(m_responseTimeoutDuration);

        QString pdu_str = QString("%1")
                .arg(QString(current.requestPdu.data().toHex()), 0, QLatin1Char('0'));

        if(ret)
        {
            DIY_LOG(LOG_DEBUG,
                    QString("(Rtu over TCP client) Sent RTU PDU (over TCP): %1").arg(pdu_str));
        }
        else
        {
            DIY_LOG(LOG_ERROR,
                    QString("(Rtu over TCP client) Sent RTU PDU (over TCP) error : %1").arg(pdu_str));
        }
    }

private:
    static const int m_srv_addr_plus_fc_byte_num = 2; //server address byte + function code byte
    static const int m_min_adu_byte_num = m_srv_addr_plus_fc_byte_num;
    static const int m_rtu_crc_byte_num = 2;
    ProcessNextReq m_proc_req_sig_helper;
};

QT_END_NAMESPACE

#endif // QMODBUSRTUOVERTCPCLIENT_P_H
