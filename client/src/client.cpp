#include "client.hpp"
#include <QMessageBox>
#include <QDateTime>

Client::Client(QWidget* parent) : QObject(parent), m_networkClient(new NetworkClient(this)), m_widget(new ClientWidget(parent)) {setupConnections();}

Client::~Client() {m_networkClient->disconnectFromServer();}

void Client::setupConnections() {
    connect(m_widget, &ClientWidget::connectClicked, this, &Client::onConnectClicked);
    connect(m_widget, &ClientWidget::disconnectClicked, this, &Client::onDisconnectClicked);
    connect(m_widget, &ClientWidget::messageSent, this, &Client::onMessageSent);
    connect(m_widget, &ClientWidget::changeInterlocutorRequested, this, &Client::onChangeInterlocutorRequested);

    connect(m_networkClient, &NetworkClient::connected, this, &Client::onNetworkConnected);
    connect(m_networkClient, &NetworkClient::disconnected, this, &Client::onNetworkDisconnected);
    connect(m_networkClient, &NetworkClient::authenticationSuccess, this, &Client::onAuthSuccess);
    connect(m_networkClient, &NetworkClient::authenticationError, this, &Client::onAuthError);
    connect(m_networkClient, &NetworkClient::messageReceived, this, &Client::onMessageReceived);
    connect(m_networkClient, &NetworkClient::interlocutorConnected, this, &Client::onInterlocutorConnected);
    connect(m_networkClient, &NetworkClient::interlocutorDisconnected, this, &Client::onInterlocutorDisconnected);
    connect(m_networkClient, &NetworkClient::interlocutorOffline, this, &Client::onInterlocutorOffline);
    connect(m_networkClient, &NetworkClient::interlocutorChanged, this, &Client::onInterlocutorChanged);
    connect(m_networkClient, &NetworkClient::interlocutorChangeError, this, &Client::onInterlocutorChangeError);
    connect(m_networkClient, &NetworkClient::connectionError, this, &Client::onConnectionError);
}

void Client::validateInput(const QString& clientName, const QString& interlocutorName) {
    if (clientName.isEmpty() || interlocutorName.isEmpty()) {
        throw std::runtime_error("Fill in all fields");
    }
    if (clientName == interlocutorName) {
        throw std::runtime_error("Your name and interlocutor name cannot be the same");
    }
}

void Client::onConnectClicked(const QString& serverAddress, const QString& clientName, const QString& interlocutorName) {
    try {
        validateInput(clientName, interlocutorName);

        m_clientName = clientName;
        m_interlocutorName = interlocutorName;

        if (m_networkClient->isConnected()) {
            m_networkClient->disconnectFromServer();
        } else {
            if (m_networkClient->connectToServer(serverAddress, 5464)) {
                m_widget->setConnectionStatus(true, QString("Connecting to %1...").arg(serverAddress));
                m_networkClient->sendAuthRequest(clientName, interlocutorName);
            } else {
                QMessageBox::warning(m_widget, "Connection Error", "Failed to connect to server");
                m_widget->setConnectionStatus(false);
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(m_widget, "Error", e.what());
    }
}

void Client::onDisconnectClicked() {
    m_networkClient->disconnectFromServer();
}

void Client::onMessageSent(const QString& text) {
    m_networkClient->sendMessage(text);

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString formattedMessage = QString("[%1] <b>You:</b> %2").arg(timestamp, text);
    m_widget->appendChatMessage(formattedMessage);
}

void Client::onChangeInterlocutorRequested(const QString& newInterlocutor) {
    if (newInterlocutor.isEmpty()) {
        QMessageBox::warning(m_widget, "Error", "Enter interlocutor name");
        return;
    }
    if (newInterlocutor == m_clientName) {
        QMessageBox::warning(m_widget, "Error", "Cannot set yourself as interlocutor");
        return;
    }

    m_networkClient->changeInterlocutor(newInterlocutor);
}

void Client::onNetworkConnected() {
    m_widget->setConnectionStatus(true, "Authenticating...");
}

void Client::onNetworkDisconnected() {
    m_widget->setConnectionStatus(false);
    m_widget->appendChatMessage("<font color='red'>Disconnected from the server</font>");
}

void Client::onAuthSuccess(const QString& clientName, const QString& interlocutorName, bool interlocutorConnected) {
    m_widget->setConnectionStatus(true, "Connected");
    m_widget->setChatEnabled(true);
    m_widget->setInterlocutorName(interlocutorName);

    m_widget->appendChatMessage("<font color='green'>Successfully authenticated</font>");

    if (interlocutorConnected) {
        m_widget->appendChatMessage(QString("<font color='green'>%1 is connected! You can now chat.</font>").arg(interlocutorName));

        m_widget->setMessageInputEnabled(true);
    } else {
        m_widget->appendChatMessage(QString("<font color='blue'>Waiting for %1 to connect...</font>").arg(interlocutorName));

        m_widget->setMessageInputEnabled(false);
    }

    m_widget->focusMessageInput();
}

void Client::onAuthError(const QString& error) {
    QMessageBox::warning(m_widget, "Authentication Error", error);
    m_widget->appendChatMessage(QString("<font color='red'>Error: %1</font>").arg(error));
    m_networkClient->disconnectFromServer();
}

void Client::onMessageReceived(const QString& sender, const QString& text, const QString& timestamp) {
    QString formattedMessage = QString("[%1] <b>%2:</b> %3").arg(timestamp, sender, text);
    m_widget->appendChatMessage(formattedMessage);
}

void Client::onInterlocutorConnected(const QString& name) {
    m_interlocutorName = name;
    m_widget->setInterlocutorName(name);
    m_widget->appendChatMessage(QString("<font color='green'>%1 has connected! You can now chat.</font>").arg(name));
    m_widget->setMessageInputEnabled(true);
}

void Client::onInterlocutorDisconnected() {
    m_widget->appendChatMessage(QString("<font color='red'>%1 disconnected</font>").arg(m_interlocutorName));
    m_widget->appendChatMessage("<font color='blue'>Waiting for interlocutor to reconnect...</font>");
    m_widget->setMessageInputEnabled(false);
}

void Client::onInterlocutorOffline() {
    m_widget->appendChatMessage("<font color='orange'>Interlocutor is offline</font>");
    m_widget->appendChatMessage("<font color='blue'>Your message was not delivered</font>");
}

void Client::onInterlocutorChanged(const QString& newInterlocutor, bool isConnected) {
    m_interlocutorName = newInterlocutor;
    m_widget->setInterlocutorName(newInterlocutor);
    m_widget->appendChatMessage(QString("<font color='green'>Interlocutor changed to %1</font>").arg(newInterlocutor));

    if (isConnected) {
        m_widget->appendChatMessage(QString("<font color='green'>%1 is connected! You can now chat.</font>").arg(newInterlocutor));
    } else {
        m_widget->appendChatMessage(QString("<font color='blue'>Waiting for %1 to connect...</font>").arg(newInterlocutor));
    }
}

void Client::onInterlocutorChangeError(const QString& error) {
    QMessageBox::warning(m_widget, "Change Interlocutor Error", error);
    m_widget->appendChatMessage(QString("<font color='red'>Error: %1</font>").arg(error));
}

void Client::onConnectionError(const QString& error) {
    QMessageBox::warning(m_widget, "Connection Error", error);
    m_widget->setConnectionStatus(false);
}
