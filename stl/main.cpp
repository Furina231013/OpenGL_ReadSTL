#include "widget.h"

#include <QApplication>
#include <QDebug>
#include <QVector>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Widget w;
    w.show();
        w.resize(800, 600);

        return a.exec();
}
