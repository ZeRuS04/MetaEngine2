#include "engine.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Engine engi("localhost", "root", "123");
//    engi.show();

    return a.exec();
}
