#pragma once

#include <QObject>
#include <QSignalSpy>

class Client;
class TestableClient;
class NetworkClient;
class ClientWidget;

class ClientTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testClientInitialization();
    void testConnectClickedSignal();
    void testDisconnectClickedSignal();
    void testMessageSentSignal();
    void testChangeInterlocutorSignal();

    void testNetworkConnected();
    void testNetworkDisconnected();
    void testAuthSuccess();
    void testAuthError();
    void testMessageReceived();
    void testInterlocutorConnected();
    void testInterlocutorDisconnected();

    void testInputValidation();
    void testSelfInterlocutorValidation();
    void testEmptyFieldsValidation();

private:
    TestableClient* client = nullptr;
};
