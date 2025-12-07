#include "client.hpp"
#include <QApplication>

int main(int argc, char **argv) {
    QApplication app(argc, argv);

    Client clientApp(nullptr);
    clientApp.widget()->show();

    return app.exec();
}
