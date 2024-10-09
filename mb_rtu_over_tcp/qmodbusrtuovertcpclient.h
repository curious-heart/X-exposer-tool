#ifndef QMODBUSRTUOVERTCPCLIENT_H
#define QMODBUSRTUOVERTCPCLIENT_H

#include <QModbusClient>

class QModbusRtuOverTcpClientPrivate;

class QModbusRtuOverTcpClient : public QModbusClient
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QModbusRtuOverTcpClient)

public:
    explicit QModbusRtuOverTcpClient(QObject *parent = nullptr);
    ~QModbusRtuOverTcpClient();

protected:
    QModbusRtuOverTcpClient(QModbusRtuOverTcpClientPrivate &dd, QObject *parent = nullptr);

    bool open() override;
    void close() override;
};

#endif // QMODBUSRTUOVERTCPCLIENT_H
