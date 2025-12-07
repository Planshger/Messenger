#include "testable_client.hpp"
#include "network/network_client.hpp"
#include "ui/client_widget.hpp"

TestableClient::TestableClient(QWidget* parent) : Client(parent) {}

NetworkClient* TestableClient::networkClient() const {
    return m_networkClient;
}

ClientWidget* TestableClient::clientWidget() const {
    return m_widget;
}

QString TestableClient::currentClientName() const {
    return m_clientName;
}

QString TestableClient::currentInterlocutorName() const {
    return m_interlocutorName;
}

void TestableClient::setTestClientName(const QString& name) {
    m_clientName = name;
    m_widget->m_clientNameEdit->setText(name);
}

void TestableClient::setTestInterlocutorName(const QString& name) {
    m_interlocutorName = name;
    m_widget->m_interlocutorNameEdit->setText(name);
}

void TestableClient::setTestServerAddress(const QString& address) {
    m_widget->m_serverAddressEdit->setText(address);
}

void TestableClient::setTestMessageInput(const QString& text) {
    m_widget->m_messageInput->setText(text);
}

void TestableClient::setTestChangeInterlocutorEdit(const QString& text) {
    m_widget->m_changeInterlocutorEdit->setText(text);
}

void TestableClient::simulateConnectButtonClick() {
    m_widget->onConnectButtonClicked();
}

void TestableClient::simulateSendButtonClick() {
    m_widget->onSendButtonClicked();
}

void TestableClient::simulateChangeInterlocutorButtonClick() {
    m_widget->onChangeInterlocutorClicked();
}

void TestableClient::setConnectionStatus(bool connected, const QString& status) {
    m_widget->setConnectionStatus(connected, status);
}

QLineEdit* TestableClient::getMessageInput() const {
    return m_widget->m_messageInput;
}

QTextEdit* TestableClient::getChatDisplay() const {
    return m_widget->m_chatDisplay;
}

QLabel* TestableClient::getStatusLabel() const {
    return m_widget->m_statusLabel;
}

QGroupBox* TestableClient::getChatGroup() const {
    return m_widget->m_chatGroup;
}

QLineEdit* TestableClient::getInterlocutorNameEdit() const {
    return m_widget->m_interlocutorNameEdit;
}

QPushButton* TestableClient::getSendButton() const {
    return m_widget->m_sendButton;
}

void TestableClient::simulateNetworkConnected() {
    emit m_networkClient->connected();
}

void TestableClient::simulateNetworkDisconnected() {
    emit m_networkClient->disconnected();
}

void TestableClient::simulateAuthSuccess(const QString& clientName, const QString& interlocutorName, bool interlocutorConnected) {
    emit m_networkClient->authenticationSuccess(clientName, interlocutorName, interlocutorConnected);
}

void TestableClient::simulateAuthError(const QString& error) {
    emit m_networkClient->authenticationError(error);
}

void TestableClient::simulateMessageReceived(const QString& sender, const QString& text, const QString& timestamp) {
    emit m_networkClient->messageReceived(sender, text, timestamp);
}

void TestableClient::simulateInterlocutorConnected(const QString& name) {
    emit m_networkClient->interlocutorConnected(name);
}

void TestableClient::simulateInterlocutorDisconnected() {
    emit m_networkClient->interlocutorDisconnected();
}

void TestableClient::testValidateInput(const QString& clientName, const QString& interlocutorName) {
    validateInput(clientName, interlocutorName);
}
