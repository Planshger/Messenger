#include <QWidget>
#include <QTcpSocket>
#include <QJsonObject>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>

class TestableClient;

class Client : public QWidget {
    friend class TestableClient;
    Q_OBJECT

public:
    explicit Client(QWidget* parent = nullptr);

public slots:
    void connectToServer();
    void disconnectFromServer();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void sendMessage();
    void changeInterlocutor();

private:
    void setupUI();
    void sendAuthRequest();
    void sendMessageWithSize(const QJsonObject& jsonObj);
    void processServerMessage(const QByteArray& data);
    void updateConnectionStatus(bool connected);
    void cleanup();

    QTcpSocket* m_socket;
    QString m_clientName;
    QString m_interlocutorName;
    bool m_isAuthenticated;
    bool m_interlocutorConnected;
    quint32 m_messageSize;

    QGroupBox* m_authGroup;
    QGroupBox* m_chatGroup;
    QLineEdit* m_serverAddressEdit;
    QLineEdit* m_clientNameEdit;
    QLineEdit* m_interlocutorNameEdit;
    QLineEdit* m_changeInterlocutorEdit;
    QTextEdit* m_chatDisplay;
    QLineEdit* m_messageInput;
    QPushButton* m_connectButton;
    QPushButton* m_sendButton;
    QPushButton* m_changeInterlocutorButton;
    QLabel* m_statusLabel;
};
