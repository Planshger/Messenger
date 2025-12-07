#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QTimer>

class ServerTest : public QObject {
    Q_OBJECT

private slots:
    void testValidateConnection();
    void testValidateInterlocutorChange();
    void testValidateConnectionEmptyNames();
    void testValidateConnectionSameNames();
    void testValidateConnectionDuplicateClient();
    void testValidateConnectionInterlocutorAlreadyPaired();

    void testProcessAuth();
    void testProcessAuthError();
    void testProcessMessage();
    void testProcessMessageUnauthorized();
    void testProcessMessageNoInterlocutor();
    void testProcessChangeInterlocutor();
    void testProcessChangeInterlocutorUnauthorized();

    void testSendMessageWithSize();
    void testSendMessageWithSizeDisconnected();

    void testRemoveClient();
    void testRemoveClientWithInterlocutor();
    void testSendToClient();
    void testSendToClientNotFound();

    void testOpenServer();

    void testCompleteCommunicationFlow();


private:
    std::unique_ptr<QTcpSocket> createMockSocket();
    QByteArray createMessageData(const QJsonObject& obj);
    void simulateClientMessage(QTcpSocket* socket, const QJsonObject& message);
};
