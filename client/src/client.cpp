#include "client.hpp"
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollBar>

Client::Client(QWidget* parent) : QWidget(parent),
    m_socket(nullptr),
    m_isAuthenticated(false) {

    setupUI();
    updateConnectionStatus(false);
}

void Client::setupUI() {
    m_authGroup = new QGroupBox("Connecting to the server", this);

    QLabel* serverLabel = new QLabel("Server address:", m_authGroup);
    m_serverAddressEdit = new QLineEdit("localhost", m_authGroup);

    QLabel* clientLabel = new QLabel("Your name:", m_authGroup);
    m_clientNameEdit = new QLineEdit(m_authGroup);

    QLabel* partnerLabel = new QLabel("Interlocutor:", m_authGroup);
    m_partnerNameEdit = new QLineEdit(m_authGroup);

    m_connectButton = new QPushButton("Connect", m_authGroup);
    connect(m_connectButton, &QPushButton::clicked, this, &Client::connectToServer);

    QVBoxLayout* authLayout = new QVBoxLayout(m_authGroup);
    authLayout->addWidget(serverLabel);
    authLayout->addWidget(m_serverAddressEdit);
    authLayout->addWidget(clientLabel);
    authLayout->addWidget(m_clientNameEdit);
    authLayout->addWidget(partnerLabel);
    authLayout->addWidget(m_partnerNameEdit);
    authLayout->addWidget(m_connectButton);

    m_chatGroup = new QGroupBox("Chat", this);
    m_chatGroup->setEnabled(false);

    m_chatDisplay = new QTextEdit(m_chatGroup);
    m_chatDisplay->setReadOnly(true);

    m_messageInput = new QLineEdit(m_chatGroup);
    m_messageInput->setEnabled(false);
    connect(m_messageInput, &QLineEdit::returnPressed, this, &Client::sendMessage);

    m_sendButton = new QPushButton("Send", m_chatGroup);
    m_sendButton->setEnabled(false);
    connect(m_sendButton, &QPushButton::clicked, this, &Client::sendMessage);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendButton);

    QVBoxLayout* chatLayout = new QVBoxLayout(m_chatGroup);
    chatLayout->addWidget(m_chatDisplay);
    chatLayout->addLayout(inputLayout);

    m_statusLabel = new QLabel("Not connected", this);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_authGroup);
    mainLayout->addWidget(m_chatGroup);
    mainLayout->addWidget(m_statusLabel);

    setWindowTitle("Chat Client");
    resize(500, 600);
}

void Client::connectToServer() {
    m_clientName = m_clientNameEdit->text().trimmed();
    m_partnerName = m_partnerNameEdit->text().trimmed();
    QString serverAddress = m_serverAddressEdit->text().trimmed();

    if (m_clientName.isEmpty() || m_partnerName.isEmpty() || serverAddress.isEmpty()) {
        QMessageBox::warning(this, "Error", "Fill in all fields");
        return;
    }

    if (m_clientName == m_partnerName) {
        QMessageBox::warning(this, "Error", "Your name and interlocutor name cannot be the same");
        return;
    }

    if (m_socket) {
        disconnectFromServer();
    }

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qDebug() << "Socket error:" << error << m_socket->errorString();
        updateConnectionStatus(false);
        m_connectButton->setEnabled(true);
        m_connectButton->setText("Connect");
    });

    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);

    qDebug() << "Connecting to server at" << serverAddress << ":5464";
    m_socket->connectToHost(serverAddress, 5464);
    m_connectButton->setEnabled(false);
    m_connectButton->setText("Connecting...");
}

void Client::disconnectFromServer() {
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

void Client::onConnected() {
    qDebug() << "Connected to server successfully";
    updateConnectionStatus(true);
    sendAuthRequest();
}

void Client::onDisconnected() {
    qDebug() << "Disconnected from server";
    updateConnectionStatus(false);
    m_isAuthenticated = false;
    m_chatGroup->setEnabled(false);
    m_messageInput->setEnabled(false);
    m_sendButton->setEnabled(false);

    if (m_connectButton) {
        m_connectButton->setEnabled(true);
        m_connectButton->setText("Connect");
    }

    m_chatDisplay->append("<font color='red'>Disconnected from the server</font>");
}

void Client::onReadyRead() {
    if (!m_socket) return;

    while (m_socket->canReadLine()) {
        QByteArray data = m_socket->readLine().trimmed();
        qDebug() << "Client received:" << data;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << parseError.errorString();
            qDebug() << "Invalid JSON data:" << data;
            continue;
        }

        if (!doc.isObject()) {
            qDebug() << "Server response is not a JSON object";
            continue;
        }

        processServerMessage(doc.object());
    }
}

void Client::sendAuthRequest() {
    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["clientName"] = m_clientName;
    authObj["partnerName"] = m_partnerName;

    QByteArray jsonData = QJsonDocument(authObj).toJson(QJsonDocument::Compact) + "\n";
    qDebug() << "Sending auth request:" << jsonData;

    if (m_socket->write(jsonData) == -1) {
        qDebug() << "Failed to send auth request:" << m_socket->errorString();
    } else {
        qDebug() << "Auth request sent successfully";
    }
}

void Client::processServerMessage(const QJsonObject& message) {
    QString type = message["type"].toString();
    qDebug() << "Processing message type:" << type;

    if (type == "auth_success") {
        m_isAuthenticated = true;
        m_chatGroup->setEnabled(true);
        m_messageInput->setEnabled(true);
        m_sendButton->setEnabled(true);
        m_chatGroup->setTitle(QString("Chat with %1").arg(m_partnerName));

        QString msg = message["message"].toString();
        m_chatDisplay->append(QString("<font color='green'>%1</font>").arg(msg));

        // Проверяем, указан ли партнер
        QString partnerName = message["partnerName"].toString();
        if (!partnerName.isEmpty()) {
            m_chatDisplay->append(QString("<font color='blue'>Waiting for %1 to connect...</font>").arg(partnerName));
        }

        m_messageInput->setFocus();
    }
    else if (type == "auth_error") {
        QString error = message["message"].toString();
        QMessageBox::warning(this, "Authentication Error", error);
        m_chatDisplay->append(QString("<font color='red'>Error: %1</font>").arg(error));
        disconnectFromServer();
    }
    else if (type == "message") {
        QString sender = message["sender"].toString();
        QString text = message["text"].toString();
        QString timestamp = message["timestamp"].toString();

        QString formattedMessage = QString("[%1] <b>%2:</b> %3")
                                       .arg(timestamp)
                                       .arg(sender)
                                       .arg(text);
        m_chatDisplay->append(formattedMessage);

        QScrollBar* scrollbar = m_chatDisplay->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }
    else if (type == "partner_connected") {
        QString partnerName = message["partnerName"].toString();
        m_chatDisplay->append(QString("<font color='green'>%1 has connected! You can now chat.</font>").arg(partnerName));
        m_messageInput->setEnabled(true);
        m_sendButton->setEnabled(true);
    }
    else if (type == "partner_disconnected") {
        QString msg = message["message"].toString();
        m_chatDisplay->append(QString("<font color='red'>%1</font>").arg(msg));
        m_messageInput->setEnabled(false);
        m_sendButton->setEnabled(false);
        m_chatDisplay->append("<font color='blue'>Waiting for partner to reconnect...</font>");
    }
    else if (type == "partner_offline") {
        QString msg = message["message"].toString();
        m_chatDisplay->append(QString("<font color='orange'>%1</font>").arg(msg));
        m_chatDisplay->append("<font color='blue'>Your message was not delivered. Partner is offline.</font>");
    }
}

void Client::sendMessage() {
    if (!m_socket || !m_isAuthenticated) {
        qDebug() << "Cannot send message: socket not ready or not authenticated";
        return;
    }

    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty()) return;

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["text"] = text;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage = QString("[%1] <b>You:</b> %2")
                                   .arg(timestamp)
                                   .arg(text);
    m_chatDisplay->append(formattedMessage);

    QByteArray jsonData = QJsonDocument(messageObj).toJson(QJsonDocument::Compact) + "\n";
    qDebug() << "Sending message:" << jsonData;

    if (m_socket->write(jsonData) == -1) {
        qDebug() << "Failed to send message:" << m_socket->errorString();
        m_chatDisplay->append("<font color='red'>Failed to send message. Check connection.</font>");
    }

    m_messageInput->clear();

    QScrollBar* scrollbar = m_chatDisplay->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void Client::updateConnectionStatus(bool connected) {
    if (connected) {
        m_statusLabel->setText(QString("Connected to %1").arg(m_serverAddressEdit->text()));
        m_statusLabel->setStyleSheet("color: green;");
    } else {
        m_statusLabel->setText("Not connected");
        m_statusLabel->setStyleSheet("color: red;");
    }
}
