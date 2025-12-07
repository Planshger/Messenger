#include "network_client.hpp"
#include <QDataStream>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

NetworkClient::NetworkClient(QObject* parent): QObject(parent), m_socket(nullptr), m_messageSize(0), m_isAuthenticated(false) {}

NetworkClient::~NetworkClient() {disconnectFromServer();}

bool NetworkClient::connectToServer(const QString& address, quint16 port) {
    if (m_socket) {
        disconnectFromServer();
    }

    m_socket = new QTcpSocket(this);
    m_messageSize = 0;
    m_isAuthenticated = false;

    connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkClient::onErrorOccurred);

    m_socket->connectToHost(address, port);
    return m_socket->waitForConnected(5000);
}

void NetworkClient::disconnectFromServer() {
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_isAuthenticated = false;
    m_messageSize = 0;
}

bool NetworkClient::isConnected() const {
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::onConnected() {
    emit connected();
}

void NetworkClient::onDisconnected() {
    m_isAuthenticated = false;
    emit disconnected();
}

void NetworkClient::onReadyRead() {
    if (!m_socket) return;

    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_5_15);

    if (m_messageSize == 0) {
        if (m_socket->bytesAvailable() < static_cast<qint64>(sizeof(quint32))) {
            return;
        }
        in >> m_messageSize;
        qDebug() << "Expecting message of size:" << m_messageSize;
    }

    if (m_socket->bytesAvailable() < m_messageSize) {
        return;
    }

    QByteArray data;
    data.resize(m_messageSize);
    int bytesRead = in.readRawData(data.data(), m_messageSize);

    if (bytesRead != m_messageSize) {
        qDebug() << "Error: Read" << bytesRead << "bytes, expected" << m_messageSize;
        m_messageSize = 0;
        return;
    }

    qDebug() << "Received message, size:" << m_messageSize;
    processServerMessage(data);
    m_messageSize = 0;

    if (m_socket->bytesAvailable() > 0) {
        onReadyRead();
    }
}

void NetworkClient::processServerMessage(const QByteArray& data) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        return;
    }

    QJsonObject message = doc.object();
    QString type = message["type"].toString();

    if (type == "auth_success") {
        QString clientName = message["clientName"].toString();
        QString interlocutorName = message["interlocutorName"].toString();
        bool interlocutorConnected = message["interlocutorConnected"].toBool();
        m_isAuthenticated = true;
        emit authenticationSuccess(clientName, interlocutorName, interlocutorConnected);
    }
    else if (type == "auth_error") {
        QString error = message["message"].toString();
        emit authenticationError(error);
    }
    else if (type == "message") {
        QString sender = message["sender"].toString();
        QString text = message["text"].toString();
        QString timestamp = message["timestamp"].toString();
        emit messageReceived(sender, text, timestamp);
    }
    else if (type == "interlocutor_connected") {
        QString interlocutorName = message["interlocutorName"].toString();
        emit interlocutorConnected(interlocutorName);
    }
    else if (type == "interlocutor_disconnected") {
        emit interlocutorDisconnected();
    }
    else if (type == "interlocutor_offline") {
        emit interlocutorOffline();
    }
    else if (type == "interlocutor_changed") {
        QString newInterlocutor = message["newInterlocutor"].toString();
        bool isConnected = message["interlocutorConnected"].toBool();
        emit interlocutorChanged(newInterlocutor, isConnected);
    }
    else if (type == "interlocutor_change_error") {
        QString error = message["message"].toString();
        emit interlocutorChangeError(error);
    }
}

void NetworkClient::sendMessageWithSize(const QJsonObject& jsonObj) {
    if (!m_socket || !isConnected()) {
        return;
    }

    QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    quint32 messageSize = static_cast<quint32>(jsonData.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << messageSize;
    out.writeRawData(jsonData.constData(), jsonData.size());

    m_socket->write(block);
}

void NetworkClient::sendAuthRequest(const QString& clientName, const QString& interlocutorName) {
    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["clientName"] = clientName;
    authObj["interlocutorName"] = interlocutorName;
    sendRawJson(authObj);
}

void NetworkClient::sendMessage(const QString& text) {
    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["text"] = text;
    sendRawJson(messageObj);
}

void NetworkClient::changeInterlocutor(const QString& newInterlocutor) {
    QJsonObject changeObj;
    changeObj["type"] = "change_interlocutor";
    changeObj["newInterlocutor"] = newInterlocutor;
    sendRawJson(changeObj);
}

void NetworkClient::sendRawJson(const QJsonObject& json) {
    sendMessageWithSize(json);
}

void NetworkClient::onErrorOccurred(QAbstractSocket::SocketError error) {
    Q_UNUSED(error);
    if (m_socket) {
        emit connectionError(m_socket->errorString());
    }
}
