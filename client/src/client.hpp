#include <QObject>
#include <QTcpSocket>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QVBoxLayout;

class Client : public QWidget {

    Q_OBJECT

public:
    Client(QWidget* parent = nullptr);

public slots:
    void socketConnect();
    void readyRead();
    void sendMessage();

private:
    QTcpSocket* m_sock;
    QLineEdit* messageInput;
    QPushButton* sendButton;
};
