#include "MainWindow.h"

#include <QApplication>
#include <BorgmaticManager.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w(std::make_unique<BorgmaticManager>());
    w.show();
    return a.exec();
}
