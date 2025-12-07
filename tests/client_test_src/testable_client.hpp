#pragma once

#include "client.hpp"

class TestableClient : public Client {
    Q_OBJECT

public:
    explicit TestableClient(QWidget* parent = nullptr);

    NetworkClient* networkClient() const;
    ClientWidget* clientWidget() const;

    QString currentClientName() const;
    QString currentInterlocutorName() const;

    void setTestClientName(const QString& name);
    void setTestInterlocutorName(const QString& name);
    void setTestServerAddress(const QString& address);
    void setTestMessageInput(const QString& text);
    void setTestChangeInterlocutorEdit(const QString& text);

    void simulateConnectButtonClick();
    void simulateSendButtonClick();
    void simulateChangeInterlocutorButtonClick();
    void setConnectionStatus(bool connected, const QString& status = QString());

    QLineEdit* getMessageInput() const;
    QTextEdit* getChatDisplay() const;
    QLabel* getStatusLabel() const;
    QGroupBox* getChatGroup() const;
    QLineEdit* getInterlocutorNameEdit() const;
    QPushButton* getSendButton() const;

    void simulateNetworkConnected();
    void simulateNetworkDisconnected();
    void simulateAuthSuccess(const QString& clientName,const QString& interlocutorName, bool interlocutorConnected);
    void simulateAuthError(const QString& error);
    void simulateMessageReceived(const QString& sender, const QString& text, const QString& timestamp);
    void simulateInterlocutorConnected(const QString& name);
    void simulateInterlocutorDisconnected();

    void testValidateInput(const QString& clientName, const QString& interlocutorName);
};
