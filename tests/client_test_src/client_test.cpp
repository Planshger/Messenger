#include "client_test.hpp"
#include "testable_client.hpp"
#include "ui/client_widget.hpp"
#include <QTest>
#include <QSignalSpy>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

void ClientTest::init() {
    client = new TestableClient();
    QVERIFY(client != nullptr);
    QVERIFY(client->clientWidget() != nullptr);
    QVERIFY(client->networkClient() != nullptr);
}

void ClientTest::cleanup() {
    delete client;
    client = nullptr;
}

void ClientTest::testClientInitialization() {
    QVERIFY(client != nullptr);

    auto widget = client->clientWidget();
    auto network = client->networkClient();

    QVERIFY(widget != nullptr);
    QVERIFY(network != nullptr);

    QVERIFY(client->currentClientName().isEmpty());
    QVERIFY(client->currentInterlocutorName().isEmpty());

    QVERIFY(widget->getServerAddress() == "localhost");
    QVERIFY(widget->getClientName().isEmpty());
    QVERIFY(widget->getInterlocutorName().isEmpty());
}

void ClientTest::testConnectClickedSignal() {
    auto widget = client->clientWidget();

    QSignalSpy connectSpy(widget, &ClientWidget::connectClicked);

    client->setTestClientName("testUser");
    client->setTestInterlocutorName("testFriend");
    client->setTestServerAddress("127.0.0.1");

    client->simulateConnectButtonClick();

    QCOMPARE(connectSpy.count(), 1);

    QList<QVariant> arguments = connectSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("127.0.0.1"));
    QCOMPARE(arguments.at(1).toString(), QString("testUser"));
    QCOMPARE(arguments.at(2).toString(), QString("testFriend"));
}

void ClientTest::testDisconnectClickedSignal() {
    auto widget = client->clientWidget();

    QSignalSpy connectSpy(widget, &ClientWidget::connectClicked);

    client->setTestClientName("testUser");
    client->setTestInterlocutorName("testFriend");
    client->setTestServerAddress("127.0.0.1");

    client->simulateConnectButtonClick();
    QTest::qWait(100);
    connectSpy.clear();

    widget->setConnectionStatus(true, "Connected");

    client->simulateConnectButtonClick();

    QCOMPARE(connectSpy.count(), 1);
}

void ClientTest::testMessageSentSignal() {
    auto widget = client->clientWidget();

    QSignalSpy messageSpy(widget, &ClientWidget::messageSent);

    client->setTestMessageInput("Hello, World!");

    widget->setConnectionStatus(true, "Connected");

    client->simulateSendButtonClick();

    QCOMPARE(messageSpy.count(), 1);

    QList<QVariant> arguments = messageSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("Hello, World!"));
}

void ClientTest::testChangeInterlocutorSignal() {
    auto widget = client->clientWidget();

    QSignalSpy changeSpy(widget, &ClientWidget::changeInterlocutorRequested);

    client->setTestChangeInterlocutorEdit("newFriend");

    widget->setConnectionStatus(true, "Connected");

    client->simulateChangeInterlocutorButtonClick();

    QCOMPARE(changeSpy.count(), 1);

    QList<QVariant> arguments = changeSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), QString("newFriend"));
}

void ClientTest::testNetworkConnected() {
    QVERIFY(client->getStatusLabel()->text().contains("Not connected"));

    client->simulateNetworkConnected();

    QTest::qWait(100);

    QVERIFY(client->getStatusLabel()->text().contains("Authenticating"));
}

void ClientTest::testNetworkDisconnected() {
    client->simulateNetworkConnected();
    QTest::qWait(100);

    client->simulateNetworkDisconnected();
    QTest::qWait(100);

    QVERIFY(client->getStatusLabel()->text().contains("Not connected"));
    QVERIFY(client->getChatDisplay()->toPlainText().contains("Disconnected"));
}

void ClientTest::testAuthSuccess() {
    client->setTestClientName("testUser");
    client->setTestInterlocutorName("testFriend");

    client->simulateAuthSuccess("testUser", "testFriend", true);
    QTest::qWait(100);

    QVERIFY(client->getStatusLabel()->text().contains("Connected"));
    QVERIFY(client->getChatGroup()->isEnabled());
    QVERIFY(client->getInterlocutorNameEdit()->text() == "testFriend");
    QVERIFY(client->getChatGroup()->title().contains("testFriend"));

    QString chatText = client->getChatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Successfully authenticated"));
    QVERIFY(chatText.contains("testFriend is connected"));

    QVERIFY(client->getMessageInput()->isEnabled());
    QVERIFY(client->getSendButton()->isEnabled());
}

void ClientTest::testAuthError() {
    client->simulateAuthError("Invalid credentials");
    QTest::qWait(100);

    QVERIFY(client->getStatusLabel()->text().contains("Not connected"));
}

void ClientTest::testMessageReceived() {
    client->setTestClientName("testUser");
    client->setTestInterlocutorName("testFriend");

    client->simulateAuthSuccess("testUser", "testFriend", true);
    QTest::qWait(100);

    client->simulateMessageReceived("testFriend", "Hello from friend!", "12:34:56");
    QTest::qWait(100);

    QString chatText = client->getChatDisplay()->toPlainText();
    QVERIFY(chatText.contains("Hello from friend!"));
    QVERIFY(chatText.contains("testFriend"));
    QVERIFY(chatText.contains("12:34:56"));
}

void ClientTest::testInterlocutorConnected() {
    client->setTestClientName("testUser");
    client->setTestInterlocutorName("testFriend");

    client->simulateAuthSuccess("testUser", "testFriend", false);
    QTest::qWait(100);

    QVERIFY(!client->getMessageInput()->isEnabled());
    QVERIFY(!client->getSendButton()->isEnabled());

    client->simulateInterlocutorConnected("testFriend");
    QTest::qWait(100);

    QVERIFY(client->getMessageInput()->isEnabled());
    QVERIFY(client->getSendButton()->isEnabled());

    QString chatText = client->getChatDisplay()->toPlainText();
    QVERIFY(chatText.contains("testFriend has connected"));
}

void ClientTest::testInterlocutorDisconnected() {
    client->setTestClientName("testUser");
    client->setTestInterlocutorName("testFriend");

    client->simulateAuthSuccess("testUser", "testFriend", true);
    QTest::qWait(100);

    QVERIFY(client->getMessageInput()->isEnabled());
    QVERIFY(client->getSendButton()->isEnabled());

    client->simulateInterlocutorDisconnected();
    QTest::qWait(100);

    QVERIFY(!client->getMessageInput()->isEnabled());
    QVERIFY(!client->getSendButton()->isEnabled());

    QString chatText = client->getChatDisplay()->toPlainText();
    QVERIFY(chatText.contains("testFriend disconnected"));
}

void ClientTest::testInputValidation() {
    try {
        client->testValidateInput("user1", "user2");
        QVERIFY(true);
    } catch (const std::exception&) {
        QVERIFY(false);
    }
}

void ClientTest::testSelfInterlocutorValidation() {
    try {
        client->testValidateInput("sameUser", "sameUser");
        QVERIFY(false);
    } catch (const std::exception& e) {
        QString error(e.what());
        QVERIFY(error.contains("cannot be the same"));
        QVERIFY(true);
    }
}

void ClientTest::testEmptyFieldsValidation() {
    try {
        client->testValidateInput("", "user2");
        QVERIFY(false);
    } catch (const std::exception& e) {
        QString error(e.what());
        QVERIFY(error.contains("Fill in all fields"));
    }

    try {
        client->testValidateInput("user1", "");
        QVERIFY(false);
    } catch (const std::exception& e) {
        QString error(e.what());
        QVERIFY(error.contains("Fill in all fields"));
    }
}
