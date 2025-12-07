#pragma once
#include <QTcpSocket>
#include <QObject>
#include <QJsonObject>

class NetworkClient : public QObject {
    Q_OBJECT

public:
    explicit NetworkClient(QObject* parent = nullptr);
    ~NetworkClient();

    bool connectToServer(const QString& address, quint16 port);
    void disconnectFromServer();
    bool isConnected() const;

    void sendAuthRequest(const QString& clientName, const QString& interlocutorName);
    void sendMessage(const QString& text);
    void changeInterlocutor(const QString& newInterlocutor);
    void sendRawJson(const QJsonObject& json);

signals:
    void connected();
    void disconnected();
    void authenticationSuccess(const QString& clientName, const QString& interlocutorName, bool interlocutorConnected);
    void authenticationError(const QString& error);
    void messageReceived(const QString& sender, const QString& text, const QString& timestamp);
    void interlocutorConnected(const QString& name);
    void interlocutorDisconnected();
    void interlocutorOffline();
    void interlocutorChanged(const QString& newInterlocutor, bool isConnected);
    void interlocutorChangeError(const QString& error);
    void connectionError(const QString& error);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    void processServerMessage(const QByteArray& data);
    void sendMessageWithSize(const QJsonObject& jsonObj);

    QTcpSocket* m_socket;
    quint32 m_messageSize;
    bool m_isAuthenticated;
};
