#include "server.hpp"
#include <QCoreApplication>

int main(int argc, char** argv) {
    QCoreApplication a(argc, argv);

    Server s;
    s.open("5464");

    return a.exec();
}
