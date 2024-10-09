#include "logger/logger.h"

#include "./private/qmodbusrtuovertcpclient_p.h"
#include "qmodbusrtuovertcpclient.h"

#include <QtCore/qurl.h>

QModbusRtuOverTcpClient::QModbusRtuOverTcpClient(QObject *parent)
    : QModbusClient(*new QModbusRtuOverTcpClientPrivate, parent)
{
    Q_D(QModbusRtuOverTcpClient);
    d->setupTcpSocket();
}

QModbusRtuOverTcpClient::~QModbusRtuOverTcpClient()
{
    close();
}

QModbusRtuOverTcpClient::QModbusRtuOverTcpClient(QModbusRtuOverTcpClientPrivate &dd, QObject *parent)
    : QModbusClient(dd, parent)
{
    Q_D(QModbusRtuOverTcpClient);
    d->setupTcpSocket();
}

bool QModbusRtuOverTcpClient::open()
{
    if (state() == QModbusDevice::ConnectedState)
        return true;

    Q_D(QModbusRtuOverTcpClient);
    if (d->m_socket->state() != QAbstractSocket::UnconnectedState)
        return false;

    const QUrl url = QUrl::fromUserInput(d->m_networkAddress + QStringLiteral(":")
        + QString::number(d->m_networkPort));

    if (!url.isValid()) {
        setError(tr("Invalid connection settings for TCP communication specified."),
            QModbusDevice::ConnectionError);
        DIY_LOG(LOG_ERROR, QString("(TCP client) Invalid host: %1 or port: %2.")
                .arg(url.host().arg(url.port())));
        return false;
    }

    d->m_socket->connectToHost(url.host(), url.port());

    return true;
}

void QModbusRtuOverTcpClient::close()
{
    if (state() == QModbusDevice::UnconnectedState)
        return;

    Q_D(QModbusRtuOverTcpClient);
    d->m_socket->disconnectFromHost();
}

/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
/*------------------------------------------------------------*/
void QModbusRtuOverTcpClientPrivate::processQueueElement(const QModbusResponse &pdu,
                                               const QueueElement &element)
{
    if (element.reply.isNull())
        return;

    element.reply->setRawResult(pdu);
    if (pdu.isException()) {
        element.reply->setError(QModbusDevice::ProtocolError,
            QModbusClient::tr("Modbus Exception Response."));
        return;
    }

    if (element.reply->type() != QModbusReply::Common) {
        element.reply->setFinished(true);
        return;
    }

    QModbusDataUnit unit = element.unit;
    if (!processResponse(pdu, &unit)) {
        element.reply->setError(QModbusDevice::UnknownError,
            QModbusClient::tr("An invalid response has been received."));
        return;
    }

    element.reply->setResult(unit);
    element.reply->setFinished(true);
}

bool QModbusRtuOverTcpClientPrivate::processResponse(const QModbusResponse &response, QModbusDataUnit *data)
{
    switch (response.functionCode()) {
    case QModbusRequest::ReadCoils:
        return processReadCoilsResponse(response, data);
    case QModbusRequest::ReadDiscreteInputs:
        return processReadDiscreteInputsResponse(response, data);
    case QModbusRequest::ReadHoldingRegisters:
        return processReadHoldingRegistersResponse(response, data);
    case QModbusRequest::ReadInputRegisters:
        return processReadInputRegistersResponse(response, data);
    case QModbusRequest::WriteSingleCoil:
        return processWriteSingleCoilResponse(response, data);
    case QModbusRequest::WriteSingleRegister:
        return processWriteSingleRegisterResponse(response, data);
    case QModbusRequest::ReadExceptionStatus:
    case QModbusRequest::Diagnostics:
    case QModbusRequest::GetCommEventCounter:
    case QModbusRequest::GetCommEventLog:
        return false;   // Return early, it's not a private response.
    case QModbusRequest::WriteMultipleCoils:
        return processWriteMultipleCoilsResponse(response, data);
    case QModbusRequest::WriteMultipleRegisters:
        return processWriteMultipleRegistersResponse(response, data);
    case QModbusRequest::ReportServerId:
    case QModbusRequest::ReadFileRecord:
    case QModbusRequest::WriteFileRecord:
    case QModbusRequest::MaskWriteRegister:
        return false;   // Return early, it's not a private response.
    case QModbusRequest::ReadWriteMultipleRegisters:
        return processReadWriteMultipleRegistersResponse(response, data);
    case QModbusRequest::ReadFifoQueue:
    case QModbusRequest::EncapsulatedInterfaceTransport:
        return false;   // Return early, it's not a private response.
    default:
        break;
    }
    return q_func()->processPrivateResponse(response, data);
}

static bool isValid(const QModbusResponse &response, QModbusResponse::FunctionCode fc)
{
    if (!response.isValid())
        return false;
    if (response.isException())
        return false;
    if (response.functionCode() != fc)
        return false;
    return true;
}

bool QModbusRtuOverTcpClientPrivate::processReadCoilsResponse(const QModbusResponse &response,
                                                    QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::ReadCoils))
        return false;
    return collateBits(response, QModbusDataUnit::Coils, data);
}

bool QModbusRtuOverTcpClientPrivate::processReadDiscreteInputsResponse(const QModbusResponse &response,
                                                             QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::ReadDiscreteInputs))
        return false;
    return collateBits(response, QModbusDataUnit::DiscreteInputs, data);
}

bool QModbusRtuOverTcpClientPrivate::collateBits(const QModbusPdu &response,
                                     QModbusDataUnit::RegisterType type, QModbusDataUnit *data)
{
    if (response.dataSize() < QModbusResponse::minimumDataSize(response))
        return false;

    const QByteArray payload = response.data();
    // byte count needs to match available bytes
    if ((payload.size() - 1) != payload[0])
        return false;

    if (data) {
        uint value = 0;
        for (qint32 i = 1; i < payload.size(); ++i) {
            const quint8 byte = quint8(payload[i]);
            for (qint32 currentBit = 0; currentBit < 8 && value < data->valueCount(); ++currentBit)
                data->setValue(value++, byte & (1U << currentBit) ? 1 : 0);
        }
        data->setRegisterType(type);
    }
    return true;
}

bool QModbusRtuOverTcpClientPrivate::processReadHoldingRegistersResponse(const QModbusResponse &response,
                                                               QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::ReadHoldingRegisters))
        return false;
    return collateBytes(response, QModbusDataUnit::HoldingRegisters, data);
}

bool QModbusRtuOverTcpClientPrivate::processReadInputRegistersResponse(const QModbusResponse &response,
                                                             QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::ReadInputRegisters))
        return false;
    return collateBytes(response, QModbusDataUnit::InputRegisters, data);
}

bool QModbusRtuOverTcpClientPrivate::collateBytes(const QModbusPdu &response,
                                      QModbusDataUnit::RegisterType type, QModbusDataUnit *data)
{
    if (response.dataSize() < QModbusResponse::minimumDataSize(response))
        return false;

    // byte count needs to match available bytes
    const quint8 byteCount = quint8(response.data().at(0));
    if ((response.dataSize() - 1) != byteCount)
        return false;

    // byte count needs to be odd to match full registers
    if (byteCount % 2 != 0)
        return false;

    if (data) {
        QDataStream stream(response.data().remove(0, 1));

        QVector<quint16> values;
        const quint8 itemCount = byteCount / 2;
        for (int i = 0; i < itemCount; i++) {
            quint16 tmp;
            stream >> tmp;
            values.append(tmp);
        }
        data->setValues(values);
        data->setRegisterType(type);
    }
    return true;
}

bool QModbusRtuOverTcpClientPrivate::processWriteSingleCoilResponse(const QModbusResponse &response,
    QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::WriteSingleCoil))
        return false;
    return collateSingleValue(response, QModbusDataUnit::Coils, data);
}

bool QModbusRtuOverTcpClientPrivate::processWriteSingleRegisterResponse(const QModbusResponse &response,
    QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::WriteSingleRegister))
        return false;
    return collateSingleValue(response, QModbusDataUnit::HoldingRegisters, data);
}

bool QModbusRtuOverTcpClientPrivate::collateSingleValue(const QModbusPdu &response,
                                       QModbusDataUnit::RegisterType type, QModbusDataUnit *data)
{
    if (response.dataSize() != QModbusResponse::minimumDataSize(response))
        return false;

    quint16 address, value;
    response.decodeData(&address, &value);
    if ((type == QModbusDataUnit::Coils) && (value != Coil::Off) && (value != Coil::On))
        return false;

    if (data) {
        data->setRegisterType(type);
        data->setStartAddress(address);
        data->setValues(QVector<quint16>{ value });
    }
    return true;
}

bool QModbusRtuOverTcpClientPrivate::processWriteMultipleCoilsResponse(const QModbusResponse &response,
                                                             QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::WriteMultipleCoils))
        return false;
    return collateMultipleValues(response, QModbusDataUnit::Coils, data);
}

bool QModbusRtuOverTcpClientPrivate::processWriteMultipleRegistersResponse(const QModbusResponse &response,
                                                                 QModbusDataUnit *data)
{
    if (!isValid(response, QModbusResponse::WriteMultipleRegisters))
        return false;
    return collateMultipleValues(response, QModbusDataUnit::HoldingRegisters, data);
}

bool QModbusRtuOverTcpClientPrivate::collateMultipleValues(const QModbusPdu &response,
                                      QModbusDataUnit::RegisterType type, QModbusDataUnit *data)
{
    if (response.dataSize() != QModbusResponse::minimumDataSize(response))
        return false;

    quint16 address, count;
    response.decodeData(&address, &count);

    // number of registers to write is 1-123 per request
    if ((type == QModbusDataUnit::HoldingRegisters) && (count < 1 || count > 123))
        return false;

    if (data) {
        data->setValueCount(count);
        data->setRegisterType(type);
        data->setStartAddress(address);
    }
    return true;
}

bool QModbusRtuOverTcpClientPrivate::processReadWriteMultipleRegistersResponse(const QModbusResponse &resp,
                                                                     QModbusDataUnit *data)
{
    if (!isValid(resp, QModbusResponse::ReadWriteMultipleRegisters))
        return false;
    return collateBytes(resp, QModbusDataUnit::HoldingRegisters, data);
}
