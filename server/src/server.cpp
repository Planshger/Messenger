#include "server.hpp"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

Server::Server(QObject* parent) : QTcpServer(parent) {connect(this, &QTcpServer::newConnection, this, &Server::onNewConnection);}

bool Server::open(const QString& port) {
    if (!listen(QHostAddress::Any, port.toInt())) {
        qDebug() << "Unable to start server";
        return false;
    }

    qDebug() << "Server successfully started on port" << port;
    return true;
}

void Server::onNewConnection() {
    QTcpSocket* clientSocket = nextPendingConnection();
    qDebug() << "New connection from" << clientSocket->peerAddress().toString();

    if (!clientSocket) {
        qDebug() << "Error: clientSocket is null!";
        return;
    }

    ClientBuffer buffer;
    buffer.expectedSize = 0;
    buffer.socket = clientSocket;
    m_buffers[clientSocket] = buffer;

    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
}

void Server::onClientDisconnected() {
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QString clientName = m_socketToName.value(clientSocket);

    if (!clientName.isEmpty()) {removeClient(clientName);}

    m_buffers.remove(clientSocket);
    clientSocket->deleteLater();
}

void Server::onReadyRead() {
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    ClientBuffer& buffer = m_buffers[clientSocket];
    QDataStream in(clientSocket);
    in.setVersion(QDataStream::Qt_5_15);

    while (true) {
        if (buffer.expectedSize == 0) {
            if (clientSocket->bytesAvailable() < static_cast<qint64>(sizeof(quint32))) {break;}
            in >> buffer.expectedSize;
            qDebug() << "Expecting message of size:" << buffer.expectedSize << "from socket";
        }

        if (clientSocket->bytesAvailable() < buffer.expectedSize) {break;}

        QByteArray data;
        data.resize(buffer.expectedSize);
        int bytesRead = in.readRawData(data.data(), buffer.expectedSize);

        if (bytesRead != buffer.expectedSize) {
            qDebug() << "Error: Read" << bytesRead << "bytes, expected" << buffer.expectedSize;
            buffer.expectedSize = 0;
            break;
        }

        qDebug() << "Server received full message, size:" << buffer.expectedSize;
        processClientMessage(clientSocket, data);
        buffer.expectedSize = 0;
    }
}

void Server::processClientMessage(QTcpSocket* clientSocket, const QByteArray& data) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        qDebug() << "Invalid JSON:" << data;
        return;
    }

    if (!doc.isObject()) {
        qDebug() << "JSON is not an object";
        return;
    }

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    qDebug() << "Processing message type:" << type;

    if (type == "auth") {
        processAuth(clientSocket, obj);
    }
    else if (type == "message") {
        processMessage(clientSocket, obj);
    }
    else if (type == "change_interlocutor") {
        processChangeInterlocutor(clientSocket, obj);
    }
    else {
        qDebug() << "Unknown message type:" << type;
    }
}

void Server::sendMessageWithSize(QTcpSocket* socket, const QJsonObject& jsonObj) {
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    quint32 messageSize = static_cast<quint32>(jsonData.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << messageSize;
    out.writeRawData(jsonData.constData(), jsonData.size());

    qDebug() << "Sending to socket, size:" << messageSize << "content:" << jsonData;

    qint64 bytesWritten = socket->write(block);
    if (bytesWritten == -1) {
        qDebug() << "Failed to send message to socket:" << socket->errorString();
    } else if (bytesWritten != block.size()) {
        qDebug() << "Warning: Only" << bytesWritten << "of" << block.size() << "bytes written";
    }
}

void Server::processAuth(QTcpSocket* clientSocket, const QJsonObject& obj) {
    QString clientName = obj["clientName"].toString();
    QString interlocutorName = obj["interlocutorName"].toString();

    qDebug() << "Auth request from:" << clientName << "to interlocutor:" << interlocutorName;

    QString error;
    if (validateConnection(clientName, interlocutorName, error)) {
        ClientInfo info;
        info.socket = clientSocket;
        info.interlocutor = interlocutorName;
        info.isAuthenticated = true;

        m_clients[clientName] = info;
        m_socketToName[clientSocket] = clientName;

        QJsonObject response;
        response["type"] = "auth_success";
        response["message"] = "Authentication successful";
        response["interlocutorName"] = interlocutorName;

        bool interlocutorConnected = m_clients.contains(interlocutorName);
        response["interlocutorConnected"] = interlocutorConnected;

        qDebug() << "Sending auth_success to" << clientName;
        sendMessageWithSize(clientSocket, response);

        qDebug() << "Client" << clientName << "authorized. Interlocutor:" << interlocutorName;

        if (m_clients.contains(interlocutorName)) {
            m_clients[interlocutorName].interlocutor = clientName;
            m_clients[clientName].interlocutor = interlocutorName;

            QJsonObject interlocutorOnline;
            interlocutorOnline["type"] = "interlocutor_connected";
            interlocutorOnline["interlocutorName"] = clientName;
            sendMessageWithSize(m_clients[interlocutorName].socket, interlocutorOnline);

            QJsonObject youAreOnline;
            youAreOnline["type"] = "interlocutor_connected";
            youAreOnline["interlocutorName"] = interlocutorName;
            sendMessageWithSize(clientSocket, youAreOnline);

            qDebug() << "Both clients are now connected and notified";
        } else {
            QJsonObject waitingMsg;
            waitingMsg["type"] = "message";
            waitingMsg["sender"] = "System";
            waitingMsg["text"] = QString("Waiting for %1 to connect...").arg(interlocutorName);
            waitingMsg["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");
            sendMessageWithSize(clientSocket, waitingMsg);
        }
    } else {
        qDebug() << "Authentication failed:" << error;
        QJsonObject response;
        response["type"] = "auth_error";
        response["message"] = error;

        sendMessageWithSize(clientSocket, response);
        clientSocket->disconnectFromHost();
    }
}

void Server::processMessage(QTcpSocket* clientSocket, const QJsonObject& obj) {
    QString senderName = m_socketToName.value(clientSocket);
    if (senderName.isEmpty() || !m_clients.contains(senderName)) {
        qDebug() << "Unauthorized client trying to send message";
        return;
    }

    QString text = obj["text"].toString();
    QString interlocutorName = m_clients[senderName].interlocutor;

    if (interlocutorName.isEmpty()) {
        qDebug() << "Interlocutor not set for client" << senderName;

        QJsonObject notification;
        notification["type"] = "message";
        notification["sender"] = "System";
        notification["text"] = "Your interlocutor is not connected yet. Please wait for them to connect.";
        notification["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");
        sendMessageWithSize(clientSocket, notification);
        return;
    }

    if (!m_clients.contains(interlocutorName)) {
        qDebug() << "Interlocutor" << interlocutorName << "is not online. Message from" << senderName << "cannot be delivered.";

        QJsonObject notification;
        notification["type"] = "interlocutor_offline";
        notification["message"] = QString("Interlocutor %1 is offline. Message not delivered.").arg(interlocutorName);
        sendMessageWithSize(clientSocket, notification);
        return;
    }

    if (m_clients[interlocutorName].interlocutor != senderName) {
        qDebug() << "Interlocutor" << interlocutorName << "is not paired with" << senderName;
        return;
    }

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["sender"] = senderName;
    messageObj["text"] = text;
    messageObj["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");

    QTcpSocket* interlocutorSocket = m_clients[interlocutorName].socket;
    if (interlocutorSocket && interlocutorSocket->state() == QAbstractSocket::ConnectedState) {
        sendMessageWithSize(interlocutorSocket, messageObj);
        qDebug() << "Message from" << senderName << "to" << interlocutorName << "delivered";
    }
}

void Server::processChangeInterlocutor(QTcpSocket* clientSocket, const QJsonObject& obj) {
    QString clientName = m_socketToName.value(clientSocket);
    if (clientName.isEmpty() || !m_clients.contains(clientName)) {
        qDebug() << "Unauthorized client trying to change interlocutor";
        return;
    }

    QString newInterlocutor = obj["newInterlocutor"].toString();
    QString error;

    if (validateInterlocutorChange(clientName, newInterlocutor, error)) {
        QString oldInterlocutor = m_clients[clientName].interlocutor;

        if (!oldInterlocutor.isEmpty() && m_clients.contains(oldInterlocutor)) {
            m_clients[oldInterlocutor].interlocutor = "";

            QJsonObject oldInterlocutorMsg;
            oldInterlocutorMsg["type"] = "interlocutor_disconnected";
            oldInterlocutorMsg["message"] = QString("%1 changed interlocutor").arg(clientName);
            sendMessageWithSize(m_clients[oldInterlocutor].socket, oldInterlocutorMsg);
        }

        m_clients[clientName].interlocutor = newInterlocutor;

        QJsonObject response;
        response["type"] = "interlocutor_changed";
        response["newInterlocutor"] = newInterlocutor;

        bool newInterlocutorConnected = m_clients.contains(newInterlocutor);
        response["interlocutorConnected"] = newInterlocutorConnected;

        sendMessageWithSize(clientSocket, response);

        if (newInterlocutorConnected) {
            m_clients[newInterlocutor].interlocutor = clientName;

            QJsonObject newInterlocutorMsg;
            newInterlocutorMsg["type"] = "interlocutor_connected";
            newInterlocutorMsg["interlocutorName"] = clientName;
            sendMessageWithSize(m_clients[newInterlocutor].socket, newInterlocutorMsg);

            qDebug() << "Client" << clientName << "changed interlocutor to" << newInterlocutor << "(connected)";
        } else {
            qDebug() << "Client" << clientName << "changed interlocutor to" << newInterlocutor << "(waiting for connection)";
        }
    } else {
        QJsonObject errorResponse;
        errorResponse["type"] = "interlocutor_change_error";
        errorResponse["message"] = error;
        sendMessageWithSize(clientSocket, errorResponse);
    }
}

bool Server::validateConnection(const QString& clientName, const QString& interlocutorName, QString& error) {
    if (clientName.isEmpty() || interlocutorName.isEmpty()) {
        error = "Client and interlocutor names cannot be empty";
        return false;
    }

    if (clientName == interlocutorName) {
        error = "Client and interlocutor names cannot be the same";
        return false;
    }

    if (m_clients.contains(clientName)) {
        error = "Username '" + clientName + "' is already taken";
        return false;
    }

    if (m_clients.contains(interlocutorName)) {
        QString existingInterlocutor = m_clients[interlocutorName].interlocutor;
        if (!existingInterlocutor.isEmpty() && existingInterlocutor != clientName) {
            error = "Selected interlocutor is already communicating with another user";
            return false;
        }
        if (existingInterlocutor.isEmpty()) {
            error = "Selected interlocutor is waiting for another connection";
            return false;
        }
    }

    return true;
}

bool Server::validateInterlocutorChange(const QString& clientName, const QString& newInterlocutor, QString& error) {
    if (newInterlocutor.isEmpty()) {
        error = "Interlocutor name cannot be empty";
        return false;
    }

    if (clientName == newInterlocutor) {
        error = "Cannot set yourself as interlocutor";
        return false;
    }

    if (m_clients.contains(newInterlocutor)) {
        QString existingInterlocutor = m_clients[newInterlocutor].interlocutor;
        if (!existingInterlocutor.isEmpty() && existingInterlocutor != clientName) {
            error = "Selected interlocutor is already communicating with another user";
            return false;
        }
    }

    return true;
}

void Server::sendToClient(const QString& receiverName, const QString& message) {
    if (!m_clients.contains(receiverName)) {
        qDebug() << "Receiver" << receiverName << "not found";
        return;
    }

    QTcpSocket* socket = m_clients[receiverName].socket;
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        QJsonObject msgObj;
        msgObj["type"] = "message";
        msgObj["sender"] = "System";
        msgObj["text"] = message;
        msgObj["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");
        sendMessageWithSize(socket, msgObj);
    }
}

void Server::removeClient(const QString& clientName) {
    if (!m_clients.contains(clientName)) return;

    QString interlocutorName = m_clients[clientName].interlocutor;

    qDebug() << "Client" << clientName << "disconnected";

    m_socketToName.remove(m_clients[clientName].socket);
    m_clients.remove(clientName);

    if (!interlocutorName.isEmpty() && m_clients.contains(interlocutorName)) {
        m_clients[interlocutorName].interlocutor = "";

        QJsonObject notification;
        notification["type"] = "interlocutor_disconnected";
        notification["message"] = "Interlocutor disconnected";

        sendMessageWithSize(m_clients[interlocutorName].socket, notification);

        qDebug() << "Notified interlocutor" << interlocutorName << "about disconnection";
    }
}

void Server::notifyInterlocutorDisconnected(const QString& clientName) {
    if (m_clients.contains(clientName)) {
        QJsonObject notification;
        notification["type"] = "interlocutor_disconnected";
        notification["message"] = "Interlocutor disconnected";

        sendMessageWithSize(m_clients[clientName].socket, notification);
    }
}
