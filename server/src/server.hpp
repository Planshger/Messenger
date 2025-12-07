#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QString>
#include <QJsonObject>

class Server : public QTcpServer {
    Q_OBJECT

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

public:

    struct ClientInfo {
        QTcpSocket* socket;
        QString interlocutor;
        bool isAuthenticated;
    };

    struct ClientBuffer {
        QTcpSocket* socket;
        quint32 expectedSize;
    };

    explicit Server(QObject* parent = nullptr);
    bool open(const QString& port);

    void processClientMessage(QTcpSocket* clientSocket, const QByteArray& data);
    void processAuth(QTcpSocket* clientSocket, const QJsonObject& obj);
    void processMessage(QTcpSocket* clientSocket, const QJsonObject& obj);
    void processChangeInterlocutor(QTcpSocket* clientSocket, const QJsonObject& obj);
    void sendMessageWithSize(QTcpSocket* socket, const QJsonObject& jsonObj);
    bool validateConnection(const QString& clientName, const QString& interlocutorName, QString& error);
    bool validateInterlocutorChange(const QString& clientName, const QString& newInterlocutor, QString& error);
    void sendToClient(const QString& receiverName, const QString& message);
    void removeClient(const QString& clientName);
    void notifyInterlocutorDisconnected(const QString& clientName);

    QMap<QString, ClientInfo> m_clients;
    QMap<QTcpSocket*, QString> m_socketToName;
    QMap<QTcpSocket*, ClientBuffer> m_buffers;
};
