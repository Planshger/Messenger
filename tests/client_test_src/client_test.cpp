#include "client_test.hpp"
#include "../../client/src/client.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QMessageBox>
#include <QSignalSpy>
#include <QTest>
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QLabel>
#include <QJsonArray>
#include <memory>
#include <QJsonDocument>
#include <QDataStream>
#include <QTcpSocket>
#include <QGroupBox>

class TestableClient : public Client {
public:
    using Client::Client;
    using Client::sendMessageWithSize;
    using Client::processServerMessage;
    using Client::sendAuthRequest;
    using Client::updateConnectionStatus;
    using Client::cleanup;

    QTcpSocket* socket() const { return m_socket; }
    QString clientName() const { return m_clientName; }
    QString interlocutorName() const { return m_interlocutorName; }
    bool isAuthenticated() const { return m_isAuthenticated; }
    bool interlocutorConnected() const { return m_interlocutorConnected; }
    quint32 messageSize() const { return m_messageSize; }
    QGroupBox* chatGroup() const { return m_chatGroup; }

    void setSocket(QTcpSocket* socket) { m_socket = socket; }
    void setClientName(const QString& name) { m_clientName = name; }
    void setInterlocutorName(const QString& name) { m_interlocutorName = name; }
    void setIsAuthenticated(bool auth) {
        m_isAuthenticated = auth;
        if (auth && m_chatGroup) {
            m_chatGroup->setEnabled(true);
            m_changeInterlocutorEdit->setEnabled(true);
            m_changeInterlocutorButton->setEnabled(true);
        }
    }
    void setInterlocutorConnected(bool connected) {
        m_interlocutorConnected = connected;
        if (m_messageInput) {
            m_messageInput->setEnabled(connected);
        }
        if (m_sendButton) {
            m_sendButton->setEnabled(connected);
        }
    }
    void setMessageSize(quint32 size) { m_messageSize = size; }

    QLineEdit* serverAddressEdit() const { return m_serverAddressEdit; }
    QLineEdit* clientNameEdit() const { return m_clientNameEdit; }
    QLineEdit* interlocutorNameEdit() const { return m_interlocutorNameEdit; }
    QLineEdit* messageInput() const { return m_messageInput; }
    QLineEdit* changeInterlocutorEdit() const { return m_changeInterlocutorEdit; }
    QTextEdit* chatDisplay() const { return m_chatDisplay; }
    QPushButton* connectButton() const { return m_connectButton; }
    QPushButton* sendButton() const { return m_sendButton; }
    QPushButton* changeInterlocutorButton() const { return m_changeInterlocutorButton; }
    QLabel* statusLabel() const { return m_statusLabel; }
};

void ClientTest::init() {
    client = new TestableClient();
}

void ClientTest::cleanup() {
    delete client;
    client = nullptr;
}

std::unique_ptr<QTcpSocket> createMockSocket() {
    return std::make_unique<QTcpSocket>();
}

QByteArray createMessageData(const QJsonObject& obj) {
    QByteArray jsonData = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    quint32 messageSize = static_cast<quint32>(jsonData.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << messageSize;
    out.writeRawData(jsonData.constData(), jsonData.size());

    return block;
}

void simulateServerMessage(TestableClient* testClient, const QJsonObject& message) {
    QByteArray jsonData = QJsonDocument(message).toJson(QJsonDocument::Compact);
    testClient->processServerMessage(jsonData);
}

void ClientTest::testInitialState() {
    QVERIFY(client->socket() == nullptr);
    QVERIFY(!client->isAuthenticated());
    QVERIFY(!client->interlocutorConnected());
    QCOMPARE(client->messageSize(), quint32(0));

    QVERIFY(client->serverAddressEdit() != nullptr);
    QVERIFY(client->clientNameEdit() != nullptr);
    QVERIFY(client->interlocutorNameEdit() != nullptr);
    QVERIFY(client->messageInput() != nullptr);
    QVERIFY(client->chatDisplay() != nullptr);
    QVERIFY(client->connectButton() != nullptr);
    QVERIFY(client->sendButton() != nullptr);

    QVERIFY(client->sendButton()->isEnabled() == false);
    QVERIFY(client->messageInput()->isEnabled() == false);
    QVERIFY(client->chatGroup()->isEnabled() == false);
}

void ClientTest::testConnectToServerValidation() {
    client->clientNameEdit()->setText("");
    client->interlocutorNameEdit()->setText("");
    client->serverAddressEdit()->setText("");

    client->connectToServer();

    QVERIFY(client->socket() == nullptr);
}

void ClientTest::testConnectToServerSameNames() {
    client->clientNameEdit()->setText("same");
    client->interlocutorNameEdit()->setText("same");
    client->serverAddressEdit()->setText("localhost");
    client->connectToServer();
    QVERIFY(client->socket() == nullptr);
}

void ClientTest::testProcessServerMessageAuthSuccess() {
    client->setIsAuthenticated(true);
    client->setInterlocutorName("testInterlocutor");

    QJsonObject authSuccess;
    authSuccess["type"] = "auth_success";
    authSuccess["message"] = "Authentication successful";
    authSuccess["interlocutorName"] = "testInterlocutor";
    authSuccess["interlocutorConnected"] = true;

    simulateServerMessage(client, authSuccess);

    QVERIFY(client->isAuthenticated());
    QVERIFY(client->interlocutorConnected());
    QCOMPARE(client->interlocutorName(), QString("testInterlocutor"));

    QVERIFY(client->chatGroup()->isEnabled());
    QVERIFY(client->sendButton()->isEnabled());
    QVERIFY(client->messageInput()->isEnabled());
}

void ClientTest::testProcessServerMessageAuthError() {
    QJsonObject authError;
    authError["type"] = "auth_error";
    authError["message"] = "Authentication failed";

    simulateServerMessage(client, authError);

    QVERIFY(!client->isAuthenticated());
}

void ClientTest::testProcessServerMessageRegular() {
    client->setIsAuthenticated(true);
    client->setInterlocutorConnected(true);

    QJsonObject message;
    message["type"] = "message";
    message["sender"] = "testSender";
    message["text"] = "Hello, World!";
    message["timestamp"] = "12:00:00";

    simulateServerMessage(client, message);

    QString chatText = client->chatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Hello, World!"));
    QVERIFY(chatText.contains("testSender"));
}

void ClientTest::testProcessServerMessageInterlocutorConnected() {
    client->setIsAuthenticated(true);
    client->setInterlocutorName("newInterlocutor");
    client->chatGroup()->setEnabled(true);

    QJsonObject connectedMsg;
    connectedMsg["type"] = "interlocutor_connected";
    connectedMsg["interlocutorName"] = "newInterlocutor";

    simulateServerMessage(client, connectedMsg);

    QVERIFY(client->interlocutorConnected());
    QCOMPARE(client->interlocutorName(), QString("newInterlocutor"));
    QVERIFY(client->sendButton()->isEnabled());
    QVERIFY(client->messageInput()->isEnabled());
}

void ClientTest::testProcessServerMessageInterlocutorDisconnected() {
    client->setIsAuthenticated(true);
    client->setInterlocutorConnected(true);
    client->chatGroup()->setEnabled(true);
    client->sendButton()->setEnabled(true);
    client->messageInput()->setEnabled(true);

    QJsonObject disconnectedMsg;
    disconnectedMsg["type"] = "interlocutor_disconnected";
    disconnectedMsg["message"] = "Interlocutor disconnected";

    simulateServerMessage(client, disconnectedMsg);

    QVERIFY(!client->interlocutorConnected());
    QVERIFY(!client->sendButton()->isEnabled());
    QVERIFY(!client->messageInput()->isEnabled());
}

void ClientTest::testProcessServerMessageInterlocutorOffline() {
    client->setIsAuthenticated(true);
    client->setInterlocutorConnected(true);
    client->chatGroup()->setEnabled(true);

    QJsonObject offlineMsg;
    offlineMsg["type"] = "interlocutor_offline";
    offlineMsg["message"] = "Interlocutor is offline";

    simulateServerMessage(client, offlineMsg);

    QString chatText = client->chatDisplay()->toPlainText();
    QVERIFY(chatText.contains("offline"));
}

void ClientTest::testProcessServerMessageInterlocutorChanged() {
    client->setIsAuthenticated(true);
    client->chatGroup()->setEnabled(true);
    client->setInterlocutorName("oldInterlocutor");

    QJsonObject changeMsg;
    changeMsg["type"] = "interlocutor_changed";
    changeMsg["newInterlocutor"] = "newInterlocutor";
    changeMsg["interlocutorConnected"] = true;

    simulateServerMessage(client, changeMsg);

    QCOMPARE(client->interlocutorName(), QString("newInterlocutor"));
    QVERIFY(client->interlocutorConnected());
    QVERIFY(client->sendButton()->isEnabled());
    QVERIFY(client->messageInput()->isEnabled());
}

void ClientTest::testProcessServerMessageInterlocutorChangeError() {
    client->setIsAuthenticated(true);
    client->chatGroup()->setEnabled(true);

    QJsonObject errorMsg;
    errorMsg["type"] = "interlocutor_change_error";
    errorMsg["message"] = "Change failed";

    simulateServerMessage(client, errorMsg);

    QString chatText = client->chatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Error"));
    QVERIFY(chatText.contains("Change failed"));
}

void ClientTest::testProcessServerMessageInvalidJson() {
    QByteArray invalidData = "Invalid JSON data";

    client->processServerMessage(invalidData);

    QVERIFY(true);
}

void ClientTest::testProcessServerMessageNotObject() {
    QJsonArray array;
    array.append("item1");
    array.append("item2");

    QByteArray data = QJsonDocument(array).toJson(QJsonDocument::Compact);

    client->processServerMessage(data);

    QVERIFY(true);
}

void ClientTest::testSendMessage() {
    client->setIsAuthenticated(true);
    client->setInterlocutorConnected(true);
    client->setInterlocutorName("testInterlocutor");
    client->chatGroup()->setEnabled(true);
    client->sendButton()->setEnabled(true);
    client->messageInput()->setEnabled(true);

    client->messageInput()->setText("Test message");

    QTcpSocket* socket = new QTcpSocket();
    client->setSocket(socket);

    client->sendMessage();

    QString chatText = client->chatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Test message"));
    QVERIFY(chatText.contains("You:"));

    client->setSocket(nullptr);
    delete socket;
}

void ClientTest::testSendMessageNotAuthenticated() {
    client->setIsAuthenticated(false);
    client->setInterlocutorConnected(false);
    client->messageInput()->setText("Test message");

    client->sendMessage();

    QVERIFY(true);
}

void ClientTest::testSendMessageInterlocutorNotConnected() {
    client->setIsAuthenticated(true);
    client->setInterlocutorConnected(false);
    client->chatGroup()->setEnabled(true);
    client->messageInput()->setEnabled(false);
    client->sendButton()->setEnabled(false);

    QTcpSocket* socket = new QTcpSocket();
    client->setSocket(socket);

    client->messageInput()->setText("Test message");

    client->sendMessage();

    QString chatText = client->chatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Cannot send message"));
    QVERIFY(chatText.contains("interlocutor is not connected"));

    client->setSocket(nullptr);
    delete socket;
}

void ClientTest::testSendMessageEmpty() {
    client->setIsAuthenticated(true);
    client->setInterlocutorConnected(true);
    client->chatGroup()->setEnabled(true);
    client->sendButton()->setEnabled(true);
    client->messageInput()->setEnabled(true);

    client->messageInput()->setText("");

    QTcpSocket* socket = new QTcpSocket();
    client->setSocket(socket);

    client->sendMessage();

    QVERIFY(true);

    client->setSocket(nullptr);
    delete socket;
}

void ClientTest::testSendAuthRequest() {
    client->setClientName("testClient");
    client->setInterlocutorName("testInterlocutor");

    QTcpSocket* socket = new QTcpSocket();
    client->setSocket(socket);

    client->sendAuthRequest();

    client->setSocket(nullptr);
    delete socket;
}

void ClientTest::testChangeInterlocutor() {
    client->setIsAuthenticated(true);
    client->setClientName("client1");
    client->chatGroup()->setEnabled(true);
    client->changeInterlocutorEdit()->setEnabled(true);
    client->changeInterlocutorButton()->setEnabled(true);

    client->changeInterlocutorEdit()->setText("newInterlocutor");

    QTcpSocket* socket = new QTcpSocket();
    client->setSocket(socket);

    client->changeInterlocutor();

    client->setSocket(nullptr);
    delete socket;
}

void ClientTest::testChangeInterlocutorValidation() {
    client->setIsAuthenticated(true);
    client->setClientName("client1");
    client->chatGroup()->setEnabled(true);
    client->changeInterlocutorEdit()->setEnabled(true);
    client->changeInterlocutorButton()->setEnabled(true);

    client->changeInterlocutorEdit()->setText("");
    client->changeInterlocutor();

    client->changeInterlocutorEdit()->setText("client1");
    client->changeInterlocutor();

    QVERIFY(true);
}

void ClientTest::testUpdateConnectionStatus() {
    client->serverAddressEdit()->setText("localhost");
    client->updateConnectionStatus(true);

    QString statusText = client->statusLabel()->text();
    QVERIFY(statusText.contains("localhost"));
    QVERIFY(statusText.contains("Connected"));

    client->updateConnectionStatus(false);
    statusText = client->statusLabel()->text();
    QVERIFY(statusText.contains("Not connected"));
}

void ClientTest::testSocketErrorHandling() {
    QTcpSocket* socket = new QTcpSocket();
    client->setSocket(socket);

    client->setSocket(nullptr);
    delete socket;

    QVERIFY(true);
}

void ClientTest::testClientLifecycle() {
    QVERIFY(!client->isAuthenticated());
    QVERIFY(!client->interlocutorConnected());

    QJsonObject authSuccess;
    authSuccess["type"] = "auth_success";
    authSuccess["message"] = "Authentication successful";
    authSuccess["interlocutorName"] = "testInterlocutor";
    authSuccess["interlocutorConnected"] = true;

    client->setIsAuthenticated(true);
    client->setInterlocutorName("testInterlocutor");

    simulateServerMessage(client, authSuccess);

    QVERIFY(client->isAuthenticated());
    QVERIFY(client->interlocutorConnected());

    QJsonObject message;
    message["type"] = "message";
    message["sender"] = "testInterlocutor";
    message["text"] = "Hello!";
    message["timestamp"] = "12:00:00";

    simulateServerMessage(client, message);

    QString chatText = client->chatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Hello!"));

    QJsonObject disconnected;
    disconnected["type"] = "interlocutor_disconnected";
    disconnected["message"] = "Interlocutor disconnected";

    simulateServerMessage(client, disconnected);

    QVERIFY(!client->interlocutorConnected());
    QVERIFY(!client->sendButton()->isEnabled());

    QVERIFY(true);
}
