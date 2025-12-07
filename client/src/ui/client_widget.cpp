#include "client_widget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QMessageBox>

ClientWidget::ClientWidget(QWidget* parent): QWidget(parent) {setupUI();}

void ClientWidget::setupUI() {
    m_authGroup = new QGroupBox("Connecting to the server", this);

    QLabel* serverLabel = new QLabel("Server address:", m_authGroup);
    m_serverAddressEdit = new QLineEdit("localhost", m_authGroup);

    QLabel* clientLabel = new QLabel("Your name:", m_authGroup);
    m_clientNameEdit = new QLineEdit(m_authGroup);

    QLabel* interlocutorLabel = new QLabel("Interlocutor:", m_authGroup);
    m_interlocutorNameEdit = new QLineEdit(m_authGroup);

    m_connectButton = new QPushButton("Connect", m_authGroup);
    connect(m_connectButton, &QPushButton::clicked, this, &ClientWidget::onConnectButtonClicked);

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
    connect(m_messageInput, &QLineEdit::returnPressed, this, &ClientWidget::onSendButtonClicked);

    m_sendButton = new QPushButton("Send", m_chatGroup);
    m_sendButton->setEnabled(false);
    connect(m_sendButton, &QPushButton::clicked, this, &ClientWidget::onSendButtonClicked);

    QLabel* changeInterlocutorLabel = new QLabel("Change Interlocutor:", m_chatGroup);
    m_changeInterlocutorEdit = new QLineEdit(m_chatGroup);
    m_changeInterlocutorEdit->setEnabled(false);
    m_changeInterlocutorButton = new QPushButton("Change", m_chatGroup);
    m_changeInterlocutorButton->setEnabled(false);
    connect(m_changeInterlocutorButton, &QPushButton::clicked, this, &ClientWidget::onChangeInterlocutorClicked);

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

void ClientWidget::setMessageInputEnabled(bool enabled) {
    m_messageInput->setEnabled(enabled);
    m_sendButton->setEnabled(enabled);
}

void ClientWidget::setChatEnabled(bool enabled) {
    m_chatGroup->setEnabled(enabled);
    m_changeInterlocutorEdit->setEnabled(enabled);
    m_changeInterlocutorButton->setEnabled(enabled);
}

void ClientWidget::onConnectButtonClicked() {
    emit connectClicked(m_serverAddressEdit->text().trimmed(),
                        m_clientNameEdit->text().trimmed(),
                        m_interlocutorNameEdit->text().trimmed());
}

void ClientWidget::onSendButtonClicked() {
    QString text = m_messageInput->text().trimmed();
    if (!text.isEmpty()) {
        emit messageSent(text);
        m_messageInput->clear();
    }
}

void ClientWidget::onChangeInterlocutorClicked() {
    QString newInterlocutor = m_changeInterlocutorEdit->text().trimmed();
    if (!newInterlocutor.isEmpty()) {
        emit changeInterlocutorRequested(newInterlocutor);
        m_changeInterlocutorEdit->clear();
    }
}

QString ClientWidget::getServerAddress() const {
    return m_serverAddressEdit->text().trimmed();
}

QString ClientWidget::getClientName() const {
    return m_clientNameEdit->text().trimmed();
}

QString ClientWidget::getInterlocutorName() const {
    return m_interlocutorNameEdit->text().trimmed();
}

void ClientWidget::appendChatMessage(const QString& message) {
    m_chatDisplay->append(message);
    QScrollBar* scrollbar = m_chatDisplay->verticalScrollBar();
    scrollbar->setValue(scrollbar->maximum());
}

void ClientWidget::setInterlocutorName(const QString& name) {
    m_interlocutorNameEdit->setText(name);
    m_chatGroup->setTitle(QString("Chat with %1").arg(name));
}

void ClientWidget::setConnectionStatus(bool connected, const QString& status) {
    if (connected) {
        m_statusLabel->setText(status.isEmpty() ? "Connected" : status);
        m_statusLabel->setStyleSheet("color: green;");
        m_connectButton->setText("Disconnect");
        setChatEnabled(true);
    } else {
        m_statusLabel->setText("Not connected");
        m_statusLabel->setStyleSheet("color: red;");
        m_connectButton->setText("Connect");
        setChatEnabled(false);
        setMessageInputEnabled(false);
    }
}
void ClientWidget::clearChat() {
    m_chatDisplay->clear();
}

void ClientWidget::focusMessageInput() {
    m_messageInput->setFocus();
}
