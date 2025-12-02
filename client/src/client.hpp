#include <QWidget>
#include <QTcpSocket>

class QLineEdit;
class QTextEdit;
class QPushButton;
class QLabel;
class QGroupBox;

class Client : public QWidget {
    Q_OBJECT

public:
    Client(QWidget* parent = nullptr);

private slots:
    void connectToServer();
    void disconnectFromServer();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void sendMessage();
    void updateConnectionStatus(bool connected);

private:
    void setupUI();
    void sendAuthRequest();
    void processServerMessage(const QJsonObject& message);

    QTcpSocket* m_socket;
    bool m_isAuthenticated;
    QString m_clientName;
    QString m_partnerName;

    QGroupBox* m_authGroup;
    QLineEdit* m_serverAddressEdit;
    QLineEdit* m_clientNameEdit;
    QLineEdit* m_partnerNameEdit;
    QPushButton* m_connectButton;

    QGroupBox* m_chatGroup;
    QTextEdit* m_chatDisplay;
    QLineEdit* m_messageInput;
    QPushButton* m_sendButton;

    QLabel* m_statusLabel;
};

