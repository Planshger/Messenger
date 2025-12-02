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

    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);
    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
}

void Server::onClientDisconnected() {
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QString clientName = m_socketToName.value(clientSocket);
    if (!clientName.isEmpty()) {
        removeClient(clientName);
    }

    clientSocket->deleteLater();
}

void Server::onReadyRead() {
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (!clientSocket) return;

    QByteArray data = clientSocket->readAll();
    qDebug() << "Server received:" << data;

    QList<QByteArray> lines = data.split('\n');
    foreach (const QByteArray& line, lines) {
        if (line.trimmed().isEmpty()) continue;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << parseError.errorString();
            qDebug() << "Invalid JSON:" << line;
            continue;
        }

        if (!doc.isObject()) {
            qDebug() << "JSON is not an object";
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();

        if (type == "auth") {
            processAuth(clientSocket, obj);
        }
        else if (type == "message") {
            processMessage(clientSocket, obj);
        }
        else {
            qDebug() << "Unknown message type:" << type;
        }
    }
}

void Server::processAuth(QTcpSocket* clientSocket, const QJsonObject& obj) {
    QString clientName = obj["clientName"].toString();
    QString partnerName = obj["partnerName"].toString();

    qDebug() << "Auth request from:" << clientName << "to partner:" << partnerName;

    QString error;
    if (validateConnection(clientName, partnerName, error)) {
        // Регистрируем клиента
        ClientInfo info;
        info.socket = clientSocket;
        info.partner = partnerName;
        info.isAuthenticated = true;

        m_clients[clientName] = info;
        m_socketToName[clientSocket] = clientName;

        QJsonObject response;
        response["type"] = "auth_success";
        response["message"] = "Authentication successful";
        response["partnerName"] = partnerName;

        QByteArray responseData = QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n";
        qDebug() << "Sending auth_success to" << clientName;

        if (clientSocket->state() == QAbstractSocket::ConnectedState) {
            clientSocket->write(responseData);
        } else {
            qDebug() << "Socket not connected, cannot send auth_success";
            return;
        }

        qDebug() << "Client" << clientName << "authorized. Partner:" << partnerName;

        if (m_clients.contains(partnerName)) {
            m_clients[partnerName].partner = clientName;

            QJsonObject partnerOnline;
            partnerOnline["type"] = "partner_connected";
            partnerOnline["partnerName"] = partnerName;
            clientSocket->write(QJsonDocument(partnerOnline).toJson(QJsonDocument::Compact) + "\n");

            QJsonObject youAreOnline;
            youAreOnline["type"] = "partner_connected";
            youAreOnline["partnerName"] = clientName;
            m_clients[partnerName].socket->write(QJsonDocument(youAreOnline).toJson(QJsonDocument::Compact) + "\n");

            qDebug() << "Both clients are now connected and notified";
        } else {
            QJsonObject waitingMsg;
            waitingMsg["type"] = "message";
            waitingMsg["sender"] = "System";
            waitingMsg["text"] = QString("Waiting for %1 to connect...").arg(partnerName);
            waitingMsg["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");
            clientSocket->write(QJsonDocument(waitingMsg).toJson(QJsonDocument::Compact) + "\n");
        }
    } else {
        qDebug() << "Authentication failed:" << error;
        QJsonObject response;
        response["type"] = "auth_error";
        response["message"] = error;

        QByteArray responseData = QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n";
        clientSocket->write(responseData);
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
    QString partnerName = m_clients[senderName].partner;

    if (partnerName.isEmpty()) {
        qDebug() << "Partner not set for client" << senderName;

        QJsonObject notification;
        notification["type"] = "message";
        notification["sender"] = "System";
        notification["text"] = "Your partner is not connected yet. Please wait for them to connect.";
        notification["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");
        clientSocket->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
        return;
    }

    if (!m_clients.contains(partnerName)) {
        qDebug() << "Partner" << partnerName << "is not online. Message from" << senderName << "cannot be delivered.";

        QJsonObject notification;
        notification["type"] = "partner_offline";
        notification["message"] = QString("Partner %1 is offline. Message not delivered.").arg(partnerName);
        clientSocket->write(QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n");
        return;
    }

    if (m_clients[partnerName].partner != senderName) {
        qDebug() << "Partner" << partnerName << "is not paired with" << senderName;
        return;
    }

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["sender"] = senderName;
    messageObj["text"] = text;
    messageObj["timestamp"] = QDateTime::currentDateTime().toString("hh:mm:ss");

    QByteArray messageData = QJsonDocument(messageObj).toJson(QJsonDocument::Compact) + "\n";

    QTcpSocket* partnerSocket = m_clients[partnerName].socket;
    if (partnerSocket && partnerSocket->state() == QAbstractSocket::ConnectedState) {
        partnerSocket->write(messageData);
        qDebug() << "Message from" << senderName << "to" << partnerName << "delivered";
    }
}

bool Server::validateConnection(const QString& clientName, const QString& partnerName, QString& error) {
    if (clientName.isEmpty() || partnerName.isEmpty()) {
        error = "Client and partner names cannot be empty";
        return false;
    }

    if (clientName == partnerName) {
        error = "Client and partner names cannot be the same";
        return false;
    }

    if (m_clients.contains(clientName)) {
        error = "Username '" + clientName + "' is already taken";
        return false;
    }

    if (m_clients.contains(partnerName)) {
        QString existingPartner = m_clients[partnerName].partner;
        // Если партнер уже общается с другим клиентом
        if (!existingPartner.isEmpty() && existingPartner != clientName) {
            error = "Selected partner is already communicating with another user";
            return false;
        }
        if (existingPartner.isEmpty()) {
            error = "Selected partner is waiting for another connection";
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
        socket->write(message.toUtf8());
    }
}

void Server::removeClient(const QString& clientName) {
    if (!m_clients.contains(clientName)) return;

    QString partnerName = m_clients[clientName].partner;

    qDebug() << "Client" << clientName << "disconnected";

    m_socketToName.remove(m_clients[clientName].socket);
    m_clients.remove(clientName);

    if (!partnerName.isEmpty() && m_clients.contains(partnerName)) {
        m_clients[partnerName].partner = "";

        QJsonObject notification;
        notification["type"] = "partner_disconnected";
        notification["message"] = "Partner disconnected";

        QByteArray notifyData = QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n";
        m_clients[partnerName].socket->write(notifyData);

        qDebug() << "Notified partner" << partnerName << "about disconnection";
    }
}

void Server::notifyPartnerDisconnected(const QString& clientName) {
    if (m_clients.contains(clientName)) {
        QJsonObject notification;
        notification["type"] = "partner_disconnected";
        notification["message"] = "Partner disconnected";

        QByteArray notifyData = QJsonDocument(notification).toJson(QJsonDocument::Compact) + "\n";
        m_clients[clientName].socket->write(notifyData);
    }
}
