#include <QTcpServer>
#include <QMap>
#include <QTcpSocket>

class Server : public QTcpServer {

    Q_OBJECT

public:
    Server();
    bool open(const QString&);

public slots:
    void newClientConnected();
    void clientStateChanged();
    void receiveMessage();

private:
    QMap<int, QTcpSocket*> mActiveClients;
};
