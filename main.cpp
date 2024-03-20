#include <QCoreApplication>
#include <QTimer>
#include "Runner.h"


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // ランナー開始
    Runner runner(QCoreApplication::arguments());
    QTimer::singleShot(0, &runner, &Runner::run);

    return app.exec();
}
