#include <QTcpServer>
#include <QMap>
#include <QTcpSocket>

class Server : public QTcpServer {
    Q_OBJECT

public:
    Server(QObject* parent = nullptr);
    bool open(const QString& port);

public slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();

private:
    struct ClientInfo {
        QTcpSocket* socket;
        QString partner;
        bool isAuthenticated;
    };

    QMap<QString, ClientInfo> m_clients;
    QMap<QTcpSocket*, QString> m_socketToName;

    void sendToClient(const QString& receiverName, const QString& message);
    void removeClient(const QString& clientName);
    bool validateConnection(const QString& clientName, const QString& partnerName, QString& error);
    void notifyPartnerDisconnected(const QString& clientName);
    void processAuth(QTcpSocket* clientSocket, const QJsonObject& obj);
    void processMessage(QTcpSocket* clientSocket, const QJsonObject& obj);
};

