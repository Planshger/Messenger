#pragma once
#include <QObject>
#include "network/network_client.hpp"
#include "ui/client_widget.hpp"

class TestableClient;

class Client : public QObject {
    friend class TestableClient;
    Q_OBJECT

public:
    explicit Client(QWidget* parent = nullptr);
    ~Client();

    ClientWidget* widget() const { return m_widget; }

private slots:
    void onConnectClicked(const QString& serverAddress, const QString& clientName, const QString& interlocutorName);
    void onDisconnectClicked();
    void onMessageSent(const QString& text);
    void onChangeInterlocutorRequested(const QString& newInterlocutor);

    void onNetworkConnected();
    void onNetworkDisconnected();
    void onAuthSuccess(const QString& clientName, const QString& interlocutorName, bool interlocutorConnected);
    void onAuthError(const QString& error);
    void onMessageReceived(const QString& sender, const QString& text, const QString& timestamp);
    void onInterlocutorConnected(const QString& name);
    void onInterlocutorDisconnected();
    void onInterlocutorOffline();
    void onInterlocutorChanged(const QString& newInterlocutor, bool isConnected);
    void onInterlocutorChangeError(const QString& error);
    void onConnectionError(const QString& error);

private:
    void validateInput(const QString& clientName, const QString& interlocutorName);
    void setupConnections();

    NetworkClient* m_networkClient;
    ClientWidget* m_widget;
    QString m_clientName;
    QString m_interlocutorName;
};
