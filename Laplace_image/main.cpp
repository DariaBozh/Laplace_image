#include "Laplace_image.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Laplace_image window;
    window.show();
    return app.exec();
}
