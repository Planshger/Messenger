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
#include <QDataStream>
#include <QJsonParseError>
#include <QCloseEvent>
#include <QDateTime>

Client::Client(QWidget* parent) : QWidget(parent),
    m_socket(nullptr),
    m_isAuthenticated(false),
    m_interlocutorConnected(false),
    m_messageSize(0) {
    setupUI();
    updateConnectionStatus(false);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &Client::cleanup);
}

void Client::cleanup() {
    qDebug() << "Cleanup called";

    if (m_socket) {
        m_socket->blockSignals(true);
        m_socket->abort();
        m_socket->deleteLater();
        m_socket = nullptr;
    }

    qDebug() << "Cleanup completed";
}


void Client::setupUI() {
    m_authGroup = new QGroupBox("Connecting to the server", this);

    QLabel* serverLabel = new QLabel("Server address:", m_authGroup);
    m_serverAddressEdit = new QLineEdit("localhost", m_authGroup);

    QLabel* clientLabel = new QLabel("Your name:", m_authGroup);
    m_clientNameEdit = new QLineEdit(m_authGroup);

    QLabel* interlocutorLabel = new QLabel("Interlocutor:", m_authGroup);
    m_interlocutorNameEdit = new QLineEdit(m_authGroup);

    m_connectButton = new QPushButton("Connect", m_authGroup);
    connect(m_connectButton, &QPushButton::clicked, this, &Client::connectToServer);

    QVBoxLayout* authLayout = new QVBoxLayout(m_authGroup);
    authLayout->addWidget(serverLabel);
    authLayout->addWidget(m_serverAddressEdit);
    authLayout->addWidget(clientLabel);
    authLayout->addWidget(m_clientNameEdit);
    authLayout->addWidget(interlocutorLabel);
    authLayout->addWidget(m_interlocutorNameEdit);
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

    QLabel* changeInterlocutorLabel = new QLabel("Change Interlocutor:", m_chatGroup);
    m_changeInterlocutorEdit = new QLineEdit(m_chatGroup);
    m_changeInterlocutorEdit->setEnabled(false);
    m_changeInterlocutorButton = new QPushButton("Change", m_chatGroup);
    m_changeInterlocutorButton->setEnabled(false);
    connect(m_changeInterlocutorButton, &QPushButton::clicked, this, &Client::changeInterlocutor);

    QHBoxLayout* changeLayout = new QHBoxLayout();
    changeLayout->addWidget(changeInterlocutorLabel);
    changeLayout->addWidget(m_changeInterlocutorEdit);
    changeLayout->addWidget(m_changeInterlocutorButton);

    QHBoxLayout* inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_messageInput);
    inputLayout->addWidget(m_sendButton);

    QVBoxLayout* chatLayout = new QVBoxLayout(m_chatGroup);
    chatLayout->addWidget(m_chatDisplay);
    chatLayout->addLayout(changeLayout);
    chatLayout->addLayout(inputLayout);

    m_statusLabel = new QLabel("Not connected", this);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_authGroup);
    mainLayout->addWidget(m_chatGroup);
    mainLayout->addWidget(m_statusLabel);

    setWindowTitle("Chat Client");
    resize(600, 700);
}

void Client::connectToServer() {
    m_clientName = m_clientNameEdit->text().trimmed();
    m_interlocutorName = m_interlocutorNameEdit->text().trimmed();
    QString serverAddress = m_serverAddressEdit->text().trimmed();

    if (m_clientName.isEmpty() || m_interlocutorName.isEmpty() || serverAddress.isEmpty()) {
        QMessageBox::warning(this, "Error", "Fill in all fields");
        return;
    }

    if (m_clientName == m_interlocutorName) {
        QMessageBox::warning(this, "Error", "Your name and interlocutor name cannot be the same");
        return;
    }

    if (m_socket) {
        disconnectFromServer();
    }

    m_socket = new QTcpSocket(this);
    m_messageSize = 0;
    m_interlocutorConnected = false;

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
    m_interlocutorConnected = false;
    m_chatGroup->setEnabled(false);
    m_messageInput->setEnabled(false);
    m_sendButton->setEnabled(false);
    m_changeInterlocutorEdit->setEnabled(false);
    m_changeInterlocutorButton->setEnabled(false);

    if (m_connectButton) {
        m_connectButton->setEnabled(true);
        m_connectButton->setText("Connect");
    }

    m_chatDisplay->append("<font color='red'>Disconnected from the server</font>");
}

void Client::onReadyRead() {
    if (!m_socket) return;

    QDataStream in(m_socket);
    in.setVersion(QDataStream::Qt_5_15);

    while (true) {
        if (m_messageSize == 0) {
            if (m_socket->bytesAvailable() < static_cast<qint64>(sizeof(quint32))) {
                break;
            }
            in >> m_messageSize;
            qDebug() << "Expecting message of size:" << m_messageSize;
        }

        if (m_socket->bytesAvailable() < m_messageSize) {
            break;
        }

        QByteArray data;
        data.resize(m_messageSize);
        int bytesRead = in.readRawData(data.data(), m_messageSize);

        if (bytesRead != m_messageSize) {
            qDebug() << "Error: Read" << bytesRead << "bytes, expected" << m_messageSize;
            m_messageSize = 0;
            break;
        }

        qDebug() << "Client received full message, size:" << m_messageSize;
        processServerMessage(data);
        m_messageSize = 0;
    }
}

void Client::processServerMessage(const QByteArray& data) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        qDebug() << "Invalid JSON data:" << QString::fromUtf8(data).left(100);
        return;
    }

    if (!doc.isObject()) {
        qDebug() << "Server response is not a JSON object";
        return;
    }

    QJsonObject message = doc.object();
    QString type = message["type"].toString();
    qDebug() << "Processing message type:" << type;

    if (type == "auth_success") {
        m_isAuthenticated = true;
        m_chatGroup->setEnabled(true);
        m_changeInterlocutorEdit->setEnabled(true);
        m_changeInterlocutorButton->setEnabled(true);

        QString interlocutorName = message["interlocutorName"].toString();
        if (!interlocutorName.isEmpty() && m_interlocutorName != interlocutorName) {
            m_interlocutorName = interlocutorName;
            m_interlocutorNameEdit->setText(interlocutorName);
        }

        m_chatGroup->setTitle(QString("Chat with %1").arg(m_interlocutorName));

        QString msg = message["message"].toString();
        m_chatDisplay->append(QString("<font color='green'>%1</font>").arg(msg));

        m_interlocutorConnected = message["interlocutorConnected"].toBool();
        if (m_interlocutorConnected) {
            m_chatDisplay->append(QString("<font color='green'>%1 is connected! You can now chat.</font>").arg(m_interlocutorName));
            m_messageInput->setEnabled(true);
            m_sendButton->setEnabled(true);
        } else {
            m_chatDisplay->append(QString("<font color='blue'>Waiting for %1 to connect...</font>").arg(m_interlocutorName));
            m_messageInput->setEnabled(false);
            m_sendButton->setEnabled(false);
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

        QString formattedMessage = QString("[%1] <b>%2:</b> %3").arg(timestamp, sender, text);

        m_chatDisplay->append(formattedMessage);

        QScrollBar* scrollbar = m_chatDisplay->verticalScrollBar();
        scrollbar->setValue(scrollbar->maximum());
    }
    else if (type == "interlocutor_connected") {
        QString interlocutorName = message["interlocutorName"].toString();
        m_interlocutorName = interlocutorName;
        m_interlocutorConnected = true;
        m_interlocutorNameEdit->setText(interlocutorName);
        m_chatGroup->setTitle(QString("Chat with %1").arg(interlocutorName));
        m_chatDisplay->append(QString("<font color='green'>%1 has connected! You can now chat.</font>").arg(interlocutorName));
        m_messageInput->setEnabled(true);
        m_sendButton->setEnabled(true);
    }
    else if (type == "interlocutor_disconnected") {
        QString msg = message["message"].toString();
        m_interlocutorConnected = false;
        m_chatDisplay->append(QString("<font color='red'>%1</font>").arg(msg));
        m_messageInput->setEnabled(false);
        m_sendButton->setEnabled(false);
        m_chatDisplay->append("<font color='blue'>Waiting for interlocutor to reconnect...</font>");
    }
    else if (type == "interlocutor_offline") {
        QString msg = message["message"].toString();
        m_chatDisplay->append(QString("<font color='orange'>%1</font>").arg(msg));
        m_chatDisplay->append("<font color='blue'>Your message was not delivered. Interlocutor is offline.</font>");
    }
    else if (type == "interlocutor_changed") {
        QString newInterlocutor = message["newInterlocutor"].toString();
        m_interlocutorName = newInterlocutor;
        m_interlocutorNameEdit->setText(newInterlocutor);
        m_changeInterlocutorEdit->clear();
        m_chatGroup->setTitle(QString("Chat with %1").arg(newInterlocutor));
        m_chatDisplay->append(QString("<font color='green'>Interlocutor changed to %1</font>").arg(newInterlocutor));

        m_interlocutorConnected = message["interlocutorConnected"].toBool();
        if (m_interlocutorConnected) {
            m_chatDisplay->append(QString("<font color='green'>%1 is connected! You can now chat.</font>").arg(newInterlocutor));
            m_messageInput->setEnabled(true);
            m_sendButton->setEnabled(true);
        } else {
            m_chatDisplay->append(QString("<font color='blue'>Waiting for %1 to connect...</font>").arg(newInterlocutor));
            m_messageInput->setEnabled(false);
            m_sendButton->setEnabled(false);
        }
    }
    else if (type == "interlocutor_change_error") {
        QString error = message["message"].toString();
        QMessageBox::warning(this, "Change Interlocutor Error", error);
        m_chatDisplay->append(QString("<font color='red'>Error: %1</font>").arg(error));
    }
}

void Client::sendMessageWithSize(const QJsonObject& jsonObj) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray jsonData = QJsonDocument(jsonObj).toJson(QJsonDocument::Compact);
    quint32 messageSize = static_cast<quint32>(jsonData.size());

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_5_15);
    out << messageSize;
    out.writeRawData(jsonData.constData(), jsonData.size());

    qDebug() << "Sending message, size:" << messageSize << "content:" << jsonData;

    qint64 bytesWritten = m_socket->write(block);
    if (bytesWritten == -1) {
        qDebug() << "Failed to send message:" << m_socket->errorString();
    } else if (bytesWritten != block.size()) {
        qDebug() << "Warning: Only" << bytesWritten << "of" << block.size() << "bytes written";
    }
}

void Client::sendAuthRequest() {
    QJsonObject authObj;
    authObj["type"] = "auth";
    authObj["clientName"] = m_clientName;
    authObj["interlocutorName"] = m_interlocutorName;

    qDebug() << "Sending auth request:" << authObj;
    sendMessageWithSize(authObj);
}

void Client::sendMessage() {
    if (!m_socket || !m_isAuthenticated) {
        qDebug() << "Cannot send message: socket not ready or not authenticated";
        return;
    }

    if (!m_interlocutorConnected) {
        m_chatDisplay->append("<font color='orange'>Cannot send message: interlocutor is not connected</font>");
        return;
    }

    QString text = m_messageInput->text().trimmed();
    if (text.isEmpty()) return;

    QJsonObject messageObj;
    messageObj["type"] = "message";
    messageObj["text"] = text;

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage = QString("[%1] <b>You:</b> %2").arg(timestamp, text);

    m_chatDisplay->append(formattedMessage);

    qDebug() << "Sending message:" << messageObj;
    sendMessageWithSize(messageObj);
    m_messageInput->clear();

    QScrollBar* scrollbar = m_chatDisplay->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void Client::changeInterlocutor() {
    if (!m_socket || !m_isAuthenticated) {
        return;
    }

    QString newInterlocutor = m_changeInterlocutorEdit->text().trimmed();
    if (newInterlocutor.isEmpty()) {
        QMessageBox::warning(this, "Error", "Enter interlocutor name");
        return;
    }

    if (newInterlocutor == m_clientName) {
        QMessageBox::warning(this, "Error", "Cannot set yourself as interlocutor");
        return;
    }

    QJsonObject changeObj;
    changeObj["type"] = "change_interlocutor";
    changeObj["newInterlocutor"] = newInterlocutor;

    qDebug() << "Sending change interlocutor request:" << changeObj;
    sendMessageWithSize(changeObj);
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
