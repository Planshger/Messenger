#include "server.hpp"

Server::Server() : QTcpServer() {}

bool Server::open(const QString& port) {
    if (!listen(QHostAddress::Any, port.toInt())) {
        qDebug() << "Unable to start server";
        return false;
    }
    connect(this, &QTcpServer::newConnection, this, &Server::newClientConnected);

    qDebug() << "Server succsessull start";

    return true;
}

void Server::newClientConnected() {
    QTcpSocket* client = nextPendingConnection();
    if (client->state() == QAbstractSocket::SocketState::ConnectedState) {
        qDebug() << "New connection!";
        mActiveClients.insert(client->socketDescriptor(), client);
        connect(client, &QTcpSocket::readyRead, this, &Server::receiveMessage);
        connect(client, &QAbstractSocket::disconnected, this, &Server::clientStateChanged);
    } else {
        qDebug() << "Failed connection";
    }
}

void Server::clientStateChanged() {
    QObject* senderObject = sender();

    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(senderObject);

    if (clientSocket) {
        int clientDescriptor = clientSocket->socketDescriptor();

        if (mActiveClients.contains(clientDescriptor)) {
            mActiveClients.remove(clientDescriptor);
            qDebug() << "Client with descriptor" << clientDescriptor << "disconnected and removed from active clients";
        } else {
            qDebug() << "Client with descriptor" << clientDescriptor << "not found in active clients";
        }

        qDebug() << "Active clients count:" << mActiveClients.size();

        clientSocket->deleteLater();
    } else {
        qDebug() << "Error: Could not cast sender to QTcpSocket";
    }
}

void Server::receiveMessage() {
    QTcpSocket* client = static_cast<QTcpSocket*>(QObject::sender());
    QString message;
    message = QString::fromLocal8Bit(client->readAll());
    qDebug() << "Recived message: " << message;
}



