#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>

class TestableClient;

class ClientWidget : public QWidget {
    friend class TestableClient;
    Q_OBJECT

public:
    explicit ClientWidget(QWidget* parent = nullptr);

    QString getServerAddress() const;
    QString getClientName() const;
    QString getInterlocutorName() const;

    void setChatEnabled(bool enabled);
    void appendChatMessage(const QString& message);
    void setInterlocutorName(const QString& name);
    void setConnectionStatus(bool connected, const QString& status = QString());

signals:
    void connectClicked(const QString& serverAddress, const QString& clientName, const QString& interlocutorName);
    void disconnectClicked();
    void messageSent(const QString& text);
    void changeInterlocutorRequested(const QString& newInterlocutor);

public slots:
    void clearChat();
    void focusMessageInput();
    void setMessageInputEnabled(bool enabled);

private slots:
    void onConnectButtonClicked();
    void onSendButtonClicked();
    void onChangeInterlocutorClicked();

private:
    void setupUI();

    QGroupBox* m_authGroup;
    QGroupBox* m_chatGroup;
    QLineEdit* m_serverAddressEdit;
    QLineEdit* m_clientNameEdit;
    QLineEdit* m_interlocutorNameEdit;
    QLineEdit* m_changeInterlocutorEdit;
    QTextEdit* m_chatDisplay;
    QLineEdit* m_messageInput;
    QPushButton* m_connectButton;
    QPushButton* m_sendButton;
    QPushButton* m_changeInterlocutorButton;
    QLabel* m_statusLabel;
};
