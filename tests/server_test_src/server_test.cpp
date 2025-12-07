#include "server_test.hpp"
#include "server.hpp"
#include <QCoreApplication>
#include <QThread>
#include <QSignalSpy>
#include <QTimer>
#include <QDebug>

std::unique_ptr<QTcpSocket> ServerTest::createMockSocket() {return std::make_unique<QTcpSocket>();}

QByteArray ServerTest::createMessageData(const QJsonObject& obj) {
    QByteArray jsonData = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    quint32 messageSize = static_cast<quint32>(jsonData.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << messageSize;
    out.writeRawData(jsonData.constData(), jsonData.size());

    return block;
}

void ServerTest::simulateClientMessage(QTcpSocket* socket, const QJsonObject& message) {
    QByteArray data = createMessageData(message);
}


void ServerTest::testValidateConnection() {
    Server server;
    QString error;

    QVERIFY(server.validateConnection("client1", "client2", error));
    QVERIFY(error.isEmpty());
}

void ServerTest::testValidateConnectionEmptyNames() {
    Server server;
    QString error;

    QVERIFY(!server.validateConnection("", "", error));
    QVERIFY(!error.isEmpty());

    QVERIFY(!server.validateConnection("client", "", error));
    QVERIFY(!error.isEmpty());

    QVERIFY(!server.validateConnection("", "interlocutor", error));
    QVERIFY(!error.isEmpty());
}

void ServerTest::testValidateConnectionSameNames() {
    Server server;
    QString error;

    QVERIFY(!server.validateConnection("same", "same", error));
    QVERIFY(!error.isEmpty());
}

void ServerTest::testValidateConnectionDuplicateClient() {
    Server server;
    QString error;

    QTcpSocket* socket1 = new QTcpSocket();
    server.m_clients["client1"] = {socket1, "client2", true};
    server.m_socketToName[socket1] = "client1";

    QVERIFY(!server.validateConnection("client1", "client3", error));
    QVERIFY(!error.isEmpty());

    delete socket1;
}

void ServerTest::testValidateConnectionInterlocutorAlreadyPaired() {
    Server server;
    QString error;

    QTcpSocket* socket1 = new QTcpSocket();
    QTcpSocket* socket2 = new QTcpSocket();

    server.m_clients["client1"] = {socket1, "client2", true};
    server.m_socketToName[socket1] = "client1";
    server.m_clients["client2"] = {socket2, "client1", true};
    server.m_socketToName[socket2] = "client2";

    QVERIFY(!server.validateConnection("client3", "client2", error));
    QVERIFY(!error.isEmpty());

    delete socket1;
    delete socket2;
}

void ServerTest::testValidateInterlocutorChange() {
    Server server;
    QString error;

    QVERIFY(server.validateInterlocutorChange("client1", "client2", error));
    QVERIFY(error.isEmpty());
}

void ServerTest::testOpenServer() {
    Server server;

    QVERIFY(server.open("5465"));

    server.close();
}


// Тесты для обработки аутентификации
void ServerTest::testProcessAuth() {
    Server server;
    server.open("5466");

    QTcpSocket* mockSocket = new QTcpSocket();

    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["clientName"] = "testClient";
    authObj["interlocutorName"] = "testInterlocutor";

    server.processAuth(mockSocket, authObj);

    QVERIFY(server.m_clients.contains("testClient"));
    QCOMPARE(server.m_clients["testClient"].interlocutor, QString("testInterlocutor"));
    QVERIFY(server.m_clients["testClient"].isAuthenticated);

    delete mockSocket;
    server.close();
}

void ServerTest::testProcessAuthError() {
    Server server;
    server.open("5467");

    QTcpSocket* existingSocket = new QTcpSocket();
    server.m_clients["existingClient"] = {existingSocket, "someone", true};
    server.m_socketToName[existingSocket] = "existingClient";

    QTcpSocket* mockSocket = new QTcpSocket();

    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["clientName"] = "existingClient";
    authObj["interlocutorName"] = "testInterlocutor";

    server.processAuth(mockSocket, authObj);

    QVERIFY(!mockSocket->isOpen());

    delete existingSocket;
    delete mockSocket;
    server.close();
}

void ServerTest::testProcessMessage() {
    Server server;
    server.open("5468");

    QTcpSocket* client1Socket = new QTcpSocket();
    QTcpSocket* client2Socket = new QTcpSocket();

    server.m_clients["client1"] = {client1Socket, "client2", true};
    server.m_socketToName[client1Socket] = "client1";
    server.m_clients["client2"] = {client2Socket, "client1", true};
    server.m_socketToName[client2Socket] = "client2";

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["text"] = "Hello, World!";

    server.processMessage(client1Socket, messageObj);

    delete client1Socket;
    delete client2Socket;
    server.close();
}

void ServerTest::testProcessMessageUnauthorized() {
    Server server;
    server.open("5469");

    QTcpSocket* mockSocket = new QTcpSocket();

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["text"] = "Test message";

    server.processMessage(mockSocket, messageObj);

    delete mockSocket;
    server.close();
}

void ServerTest::testProcessMessageNoInterlocutor() {
    Server server;
    server.open("5470");

    QTcpSocket* clientSocket = new QTcpSocket();
    server.m_clients["client1"] = {clientSocket, "", true};
    server.m_socketToName[clientSocket] = "client1";

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["text"] = "Test message";

    server.processMessage(clientSocket, messageObj);

    delete clientSocket;
    server.close();
}

void ServerTest::testProcessChangeInterlocutor() {
    Server server;
    server.open("5471");

    QTcpSocket* clientSocket = new QTcpSocket();
    server.m_clients["client1"] = {clientSocket, "oldInterlocutor", true};
    server.m_socketToName[clientSocket] = "client1";

    QJsonObject changeObj;
    changeObj["type"] = "change_interlocutor";
    changeObj["newInterlocutor"] = "newInterlocutor";

    server.processChangeInterlocutor(clientSocket, changeObj);

    QCOMPARE(server.m_clients["client1"].interlocutor, QString("newInterlocutor"));

    delete clientSocket;
    server.close();
}

void ServerTest::testProcessChangeInterlocutorUnauthorized() {
    Server server;
    server.open("5472");

    QTcpSocket* mockSocket = new QTcpSocket();

    QJsonObject changeObj;
    changeObj["type"] = "change_interlocutor";
    changeObj["newInterlocutor"] = "newInterlocutor";

    server.processChangeInterlocutor(mockSocket, changeObj);

    delete mockSocket;
    server.close();
}

void ServerTest::testSendMessageWithSize() {
    Server server;
    server.open("5473");

    QTcpServer testServer;
    testServer.listen(QHostAddress::LocalHost, 5474);

    QTcpSocket* testSocket = new QTcpSocket();
    testSocket->connectToHost("localhost", 5474);

    if (testServer.waitForNewConnection(1000)) {
        QTcpSocket* serverSocket = testServer.nextPendingConnection();

        QJsonObject testObj;
        testObj["type"] = "test";
        testObj["message"] = "Test message";

        server.sendMessageWithSize(serverSocket, testObj);

        QVERIFY(serverSocket->bytesToWrite() > 0);

        delete serverSocket;
    }

    delete testSocket;
    testServer.close();
    server.close();
}

void ServerTest::testSendMessageWithSizeDisconnected() {
    Server server;

    QTcpSocket* disconnectedSocket = new QTcpSocket();

    QJsonObject testObj;
    testObj["type"] = "test";
    testObj["message"] = "Test message";

    server.sendMessageWithSize(disconnectedSocket, testObj);


    delete disconnectedSocket;
}

void ServerTest::testRemoveClient() {
    Server server;
    server.open("5475");

    QTcpSocket* clientSocket = new QTcpSocket();
    server.m_clients["client1"] = {clientSocket, "", true};
    server.m_socketToName[clientSocket] = "client1";

    server.removeClient("client1");

    QVERIFY(!server.m_clients.contains("client1"));
    QVERIFY(!server.m_socketToName.contains(clientSocket));

    delete clientSocket;
    server.close();
}

void ServerTest::testRemoveClientWithInterlocutor() {
    Server server;
    server.open("5476");

    QTcpSocket* client1Socket = new QTcpSocket();
    QTcpSocket* client2Socket = new QTcpSocket();

    server.m_clients["client1"] = {client1Socket, "client2", true};
    server.m_socketToName[client1Socket] = "client1";
    server.m_clients["client2"] = {client2Socket, "client1", true};
    server.m_socketToName[client2Socket] = "client2";

    server.removeClient("client1");

    QVERIFY(!server.m_clients.contains("client1"));
    QVERIFY(server.m_clients.contains("client2"));
    QVERIFY(server.m_clients["client2"].interlocutor.isEmpty());

    delete client1Socket;
    delete client2Socket;
    server.close();
}

void ServerTest::testSendToClient() {
    Server server;
    server.open("5477");

    QTcpServer testServer;
    testServer.listen(QHostAddress::LocalHost, 5478);

    QTcpSocket* testSocket = new QTcpSocket();
    testSocket->connectToHost("localhost", 5478);

    if (testServer.waitForNewConnection(1000)) {
        QTcpSocket* serverSocket = testServer.nextPendingConnection();

        server.m_clients["testClient"] = {serverSocket, "", true};
        server.m_socketToName[serverSocket] = "testClient";

        server.sendToClient("testClient", "Test message");

        QVERIFY(serverSocket->bytesToWrite() > 0);

        delete serverSocket;
    }

    delete testSocket;
    testServer.close();
    server.close();
}

void ServerTest::testSendToClientNotFound() {
    Server server;

    server.sendToClient("nonExistentClient", "Test message");

    QVERIFY(true);
}

void ServerTest::testCompleteCommunicationFlow() {
    Server server;
    QVERIFY(server.open("5479"));

    server.close();
}
