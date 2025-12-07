#include <QObject>

class TestableClient;
class QTcpSocket;
class QJsonObject;

class ClientTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void testInitialState();
    void testConnectToServerValidation();
    void testConnectToServerSameNames();
    void testProcessServerMessageAuthSuccess();
    void testProcessServerMessageAuthError();
    void testProcessServerMessageRegular();
    void testProcessServerMessageInterlocutorConnected();
    void testProcessServerMessageInterlocutorDisconnected();
    void testProcessServerMessageInterlocutorOffline();
    void testProcessServerMessageInterlocutorChanged();
    void testProcessServerMessageInterlocutorChangeError();
    void testProcessServerMessageInvalidJson();
    void testProcessServerMessageNotObject();
    void testSendMessage();
    void testSendMessageNotAuthenticated();
    void testSendMessageInterlocutorNotConnected();
    void testSendMessageEmpty();
    void testSendAuthRequest();
    void testChangeInterlocutor();
    void testChangeInterlocutorValidation();
    void testUpdateConnectionStatus();
    void testSocketErrorHandling();
    void testClientLifecycle();

private:
    TestableClient* client = nullptr;
};
