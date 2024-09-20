#include <QCoreApplication>
#include <QTimer>
#include "Runner.h"

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    #include <openssl/opensslv.h>

    #if _DEBUG
        #include <iostream>
    #endif
#endif


int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0) && _DEBUG
    std::cout << QString("OpenSSL version: %1").arg(OPENSSL_VERSION_TEXT).toStdString() << std::endl;
#endif

    // アプリケーション名
    app.setApplicationName("qEQAlert");

    // バージョン
    QString version = QString("%1.%2.%3").arg(PROJECT_VERSION_MAJOR).arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH);
    app.setApplicationVersion(version);

    // 組織
    app.setOrganizationDomain("Presire");
    app.setOrganizationName("Presire");

    // ランナー開始
    Runner runner(app, QCoreApplication::arguments());
    QTimer::singleShot(0, &runner, &Runner::run);

    // アプリケーションのイベントループを開始
    return app.exec();
}
