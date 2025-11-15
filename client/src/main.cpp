#include "client.hpp"
#include <QApplication>

int main(int argc, char** argv) {
    QApplication s(argc, argv);

    Client client;
    client.show();

    return s.exec();
}
