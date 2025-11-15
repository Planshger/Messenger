#include "client.hpp"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDebug>

Client::Client(QWidget* parent) : QWidget(parent) {

    messageInput = new QLineEdit(this);
    sendButton = new QPushButton("Send", this);


    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(messageInput);
    layout->addWidget(sendButton);


    m_sock = new QTcpSocket(this);
    const QString add("localhost");
    const QString port("5464");
    m_sock->connectToHost(add, port.toInt());


    connect(m_sock, &QTcpSocket::connected, this, &Client::socketConnect);
    connect(m_sock, &QTcpSocket::readyRead, this, &Client::readyRead);
    connect(sendButton, &QPushButton::clicked, this, &Client::sendMessage);
    connect(messageInput, &QLineEdit::returnPressed, this, &Client::sendMessage);

    setWindowTitle("Chat Client");
    resize(300, 100);
}

void Client::socketConnect() {
    qDebug() << "Connected to server!";
}

void Client::sendMessage() {
    QString message = messageInput->text();
    if (!message.isEmpty()) {
        m_sock->write(message.toUtf8());
        m_sock->waitForBytesWritten();
        messageInput->clear();
        qDebug() << "Sent:" << message;
    }
}

void Client::readyRead() {
    QByteArray data = m_sock->readAll();
    QString message = QString::fromUtf8(data);
    qDebug() << "Received:" << message;
}
