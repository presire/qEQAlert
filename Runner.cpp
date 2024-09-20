#include <QCoreApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QException>
#include <iostream>
#include <utility>
#include <set>
#include "Runner.h"
#include "CommandLineParser.h"


#ifdef Q_OS_LINUX
Runner::Runner(QCoreApplication &app, QStringList _args, QObject *parent) : m_App(app), m_args(std::move(_args)),
    m_SysConfFile(""), m_EQInterval(10 * 1000),
    m_pNotifier(std::make_unique<QSocketNotifier>(fileno(stdin), QSocketNotifier::Read, this)), m_stopRequested(false),
    QObject{parent}
{
    connect(&m_EQTimer, &QTimer::timeout, this, &Runner::fetch);
    connect(m_pNotifier.get(), &QSocketNotifier::activated, this, &Runner::onReadyRead);    // キーボードシーケンスの有効化
}

#elif Q_OS_WIN

Runner::Runner(QCoreApplication &app, QStringList _args, QObject *parent) : m_App(app), m_args(std::move(_args)),
    m_SysConfFile(""), m_interval(30 * 60 * 1000),
    m_pNotifier(std::make_unique<QWinEventNotifier>(fileno(stdin), QWinEventNotifier::Read, this)), m_stopRequested(false),
    QObject{parent}
{
    connect(&m_EQTimer, &QTimer::timeout, this, &Runner::fetch);
    connect(m_pNotifier.get(), &QWinEventNotifier::activated, this, &Runner::onReadyRead);      // キーボードシーケンスの有効化
}
#endif


void Runner::run()
{
    // メイン処理

    // コマンドラインオプションの確認
    // アプリケーションの説明を設定
    CommandLineParser parser;
    parser.setApplicationDescription("qEQAlertは、0ch系の掲示板に地震情報を自動的に書き込むソフトウェアです");

    // --sysconf オプションを追加
    QCommandLineOption sysconfOption(QStringList() << "sysconf",
                                     "設定ファイル(.json)のパスを指定します",
                                     "confFilePath");
    parser.addOption(sysconfOption);

    // --version / -v オプションを追加
    QCommandLineOption versionOption(QStringList() << "version" << "v", "バージョン情報を表示します");
    parser.addOption(versionOption);

    // --help / -h オプションを追加
    QCommandLineOption helpOption(QStringList() << "help" << "h", "ヘルプ情報を表示します");
    parser.addOption(helpOption);

    // 未知のオプションを許可
    parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsOptions);

    // コマンドライン引数を解析
    parser.process(QCoreApplication::arguments());

    // 未知のオプションをチェック
    if (!parser.unknownOptionNames().isEmpty()) {
        std::cerr << QString("エラー : 不明なオプション %1").arg(parser.unknownOptionNames().join(", ")).toStdString() << std::endl;
        QCoreApplication::exit();
        return;
    }

    // 指定されたオプションの数をカウント
    int optionCount = 0;
    QString specifiedOption;

    if (parser.isVersionSet()) {
        optionCount++;
        specifiedOption = "version";
    }

    if (parser.isHelpSet()) {
        optionCount++;
        specifiedOption = "help";
    }

    if (parser.isSysConfSet()) {
        optionCount++;
        specifiedOption = "sysconf";
    }

    const QStringList unknownOptions = parser.unknownOptionNames();
    if (!unknownOptions.isEmpty()) {
        optionCount += unknownOptions.size();
        specifiedOption = unknownOptions.first();
    }

    if (optionCount > 1) {
        std::cerr << QString("エラー : 指定できるオプションは1つのみです").toStdString() << std::endl;
        QCoreApplication::exit();
        return;
    }
    else if (optionCount == 0) {
        std::cerr << QString("エラー : オプションがありません").toStdString() << std::endl;
        QCoreApplication::exit();
        return;
    }

    if (parser.isSet(versionOption)) {
        // --version / -vオプション
        auto version = QString("qEQAlert %1.%2.%3\n").arg(PROJECT_VERSION_MAJOR).arg(PROJECT_VERSION_MINOR).arg(PROJECT_VERSION_PATCH)
                       + QString("ライセンス : UNLICENSE\n")
                       + QString("ライセンスの詳細な情報は、<http://unlicense.org/> を参照してください\n\n")
                       + QString("開発者 : Presire with ﾘ* ﾞㇷﾞ)ﾚ の みんな\n\n");
        std::cout << version.toStdString() << std::endl;

        QCoreApplication::exit();
        return;
    }
    else if (parser.isSet(helpOption)) {
        // --help / -h オプション
        auto help = QString("使用法 : qEQAlert [オプション]\n\n")
                    + QString("  --sysconf=<qEQAlert.jsonファイルのパス>\t\t設定ファイルのパスを指定する\n")
                    + QString("  -v, -V, --version                    \t\tバージョン情報を表示する\n\n");
        std::cout << help.toStdString() << std::endl;

        QCoreApplication::exit();
        return;
    }
    else if (parser.isSet(sysconfOption)) {
        // --sysconfオプションの値を取得
        auto option = parser.value(sysconfOption);

        // 先頭と末尾にクォーテーションが存在する場合は取り除く
        if ((option.startsWith('\"') && option.endsWith('\"')) || (option.startsWith('\'') && option.endsWith('\''))) {
            option = option.mid(1, option.length() - 2);
        }

        if (option.isEmpty()) {
            std::cerr << QString("エラー : 設定ファイルのパスが不明です").toStdString() << std::endl;

            QCoreApplication::exit();
            return;
        }

        m_SysConfFile = option;

        if (getConfiguration(m_SysConfFile)) {
            QCoreApplication::exit();
            return;
        }
    }
    else {
        std::cerr << QString("エラー : 不明なオプションです - %1").arg(parser.isSet(specifiedOption)).toStdString() << std::endl;
        QCoreApplication::exit();
        return;
    }

    // いずれかの地震情報の取得が有効になっているかどうかを確認
    if (!m_bEQAlert && !m_bEQInfo) {
        std::cerr << QString("エラー : 緊急地震速報(警報)および発生した地震情報の取得がいずれも無効に設定されています").toStdString() << std::endl;

        QCoreApplication::exit();
        return;
    }

    // 緊急地震速報(警報)および発生した地震情報を取得するかどうかを確認
    // いずれかが有効の場合、かつ、ワンショット機能が無効の場合は、緊急地震速報(警報)および発生した地震情報のタイマ割り込みを有効化
    if (m_bOneShot) {
        // 自動的に地震情報を取得しない場合、タイマのシグナル / スロットを無効にする
        disconnect(&m_EQTimer, &QTimer::timeout, this, &Runner::fetch);
    }
    else {
        // 地震情報を取得する間隔 (未指定の場合、インターバルは10[秒])
        // ただし、5[秒]未満には設定できない (5[秒]未満に設定した場合は、5[秒]に設定する)
        if (m_EQInterval == 0) {
            std::cerr << "インターバルの値が未指定もしくは0のため、10[秒]に設定されます" << std::endl;
            m_EQInterval = 10 * 1000;
        }
        else if (m_EQInterval < (5 * 1000)) {
            std::cerr << "インターバルの値が5[秒]未満のため、10[秒]に設定されます" << std::endl;
            m_EQInterval = 10 * 1000;
        }
        else if (m_EQInterval < 0) {
            std::cerr << "インターバルの値が不正です" << std::endl;

            QCoreApplication::exit();
            return;
        }

        m_EQTimer.start(static_cast<int>(m_EQInterval));
    }

    // 本ソフトウェア開始直後に地震情報を取得する場合は、コメントを解除して、fetch()メソッドを実行する
    // コメントアウトしている場合、最初に地震情報を取得するタイミングは、タイマの指定時間後となる
    fetch();

    // ソフトウェアの自動起動が無効の場合
    // Cronを使用する場合、または、ワンショットで動作させる場合の処理
    if (m_bOneShot) {
        // 既に[q]キーまたは[Q]キーが押下されている場合は再度終了処理を行わない
        if (!m_stopRequested.load()) {
            // ソフトウェアを終了する
            QCoreApplication::exit();
            return;
        }
    }
}


void Runner::fetch()
{
    if (m_stopRequested.load()) return;

    if (!m_bOneShot) {
        // 地震情報の取得タイマを一時停止
        QMetaObject::invokeMethod(&m_EQTimer, "stop", Qt::QueuedConnection);
    }

#ifdef _DEBUG
    // 処理開始時刻
    auto start = std::chrono::high_resolution_clock::now();
#endif

    // 緊急地震速報(警報)および発生した地震情報を取得
    // デフォルト
    // 緊急地震速報(警報) : 30[秒]以内の最新情報 (1件のみ)
    // 発生した地震情報 : 5[分]以内の最新情報 (1件のみ)

    // ただし、緊急地震速報(警報)としての内容や配信品質は無保証であるため、利用は非推奨となっている
    // 遅延や欠落のリスクは以下の通り
    // - 処理遅延 : WebSocket APIは約70[ms]、JSON APIは約1000[ms] (高負荷時はさらに遅延する)
    // - サーバ所在地 : 東京 (Linode Tokyo 2)
    // - 欠落リスク : サーバや受信プログラムは冗長化しておらず、障害時は配信できず復旧時の再配信もない
    if (!m_pEarthQuake) {
        m_pEarthQuake.reset(nullptr);
    }

    m_pEarthQuake = std::make_unique<EarthQuake>(m_iGetInfo, m_EQAlertURL, m_EQInfoURL,
                                                 m_bEQAlert, m_bEQInfo, m_AlertScale, m_InfoScale, m_AlertFile, m_InfoFile,
                                                 m_RequestURL, m_EQsubTime, m_EQchangeTitle, m_ThreadInfo, m_ExpiredXPath,
                                                 this);

    // 実行
    [[maybe_unused]] auto ret = m_pEarthQuake->EQProcess(m_EQImageInfo);

#ifdef _DEBUG
    // 処理終了時刻
    // 経過時間を計算 (ミリ秒単位)
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
    std::cout << QString("地震情報の処理に掛かった時間 : %1 [mS]").arg(duration).toStdString() << std::endl;
#endif

    if (!m_bOneShot) {
        // 地震情報の取得タイマを再開
        m_EQTimer.setInterval(m_EQInterval);
        QMetaObject::invokeMethod(&m_EQTimer, "start", Qt::QueuedConnection);
    }

    // [q]キーまたは[Q]キー ==> [Enter]キーが押下されているかどうかを確認
    if (m_stopRequested.load()) return;
}


int Runner::getConfiguration(QString &filepath)
{
    // 設定ファイルのパスが空の場合
    if(filepath.isEmpty()) {
        std::cerr << QString("エラー : オプションが不明です").toStdString() + QString("\n").toStdString() +
                     QString("使用可能なオプションは次の通りです : --sysconf=<qEQAlert.jsonのパス>").toStdString() << std::endl;
        return -1;
    }

    // 指定された設定ファイルが存在しない場合
    if(!QFile::exists(filepath)) {
        std::cerr << QString("エラー : 設定ファイルが存在しません : %1").arg(filepath).toStdString() << std::endl;
        return -1;
    }

    // 設定ファイルを読み込む
    try {
        QFile File(filepath);
        if(!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
            std::cerr << QString("エラー : 設定ファイルのオープンに失敗しました %1").arg(File.errorString()).toStdString() << std::endl;
            return -1;
        }

        auto byaryJson = File.readAll();
        File.close();

        // qEQAlert.jsonファイルの設定を取得
        auto JsonDocument = QJsonDocument::fromJson(byaryJson);
        if (JsonDocument.isNull()) {
            std::cerr << QString("不正なJSONファイルです").toStdString() << std::endl;
            return -1;
        }

        auto JsonObject   = JsonDocument.object();

        // 緊急地震速報(警報)および発生した地震情報を取得するかどうか
        QJsonObject earthquakeObj = JsonObject.value("earthquake").toObject();

        // 地震情報を取得するWebサイトを選択
        // 0 : JMA (気象庁), 1 : P2P地震情報
        m_iGetInfo = earthquakeObj.value("get").toInt();
        if (m_iGetInfo != 0 && m_iGetInfo != 1) {
            std::cerr << QString("\"get\"キーの値が不正です\n0または1を指定してください").toStdString() << std::endl;
            return -1;
        }

        // 緊急地震速報(警報)を取得するURL
        // 現在の仕様では、緊急地震速報(警報)はP2P地震情報から取得する
        QJsonObject alertURLObj = earthquakeObj.value("alerturl").toObject();
        if (m_iGetInfo == 0)      m_EQAlertURL = alertURLObj.value("p2p").toString("");
        else if (m_iGetInfo == 1) m_EQAlertURL = alertURLObj.value("p2p").toString("");

        if (m_EQAlertURL.isEmpty()) {
            std::cerr << QString("\"alerturl\"キーの値が空欄です\n緊急地震速報(警報)を取得するURLを指定してください").toStdString() << std::endl;
            return -1;
        }

        // 発生した地震情報を取得するURL
        QJsonObject infoURLObj = earthquakeObj.value("infourl").toObject();
        if (m_iGetInfo == 0)      m_EQInfoURL = infoURLObj.value("jma").toString("");
        else if (m_iGetInfo == 1) m_EQInfoURL = infoURLObj.value("p2p").toString("");

        if (m_EQInfoURL.isEmpty()) {
            std::cerr << QString("\"infourl\"キーの値が空欄です\n発生した地震情報を取得するURLを指定してください").toStdString() << std::endl;
            return -1;
        }

        // 緊急地震速報(警報)の有効 / 無効
        m_bEQAlert = earthquakeObj.value("alert").toBool(false);
        if (m_bEQAlert) {
            /// 読み書き権限の確認
            m_AlertFile = earthquakeObj.value("alertlog").toString("/tmp/eqalert.log");
            if (m_AlertFile.isEmpty()) {
                m_AlertFile = QString("/tmp/eqalert.log");
            }

            /// 書き込み済み(ログ用)のJSONファイルが存在しない場合は空のログファイルを作成
            QFile EQAlertFile(m_AlertFile);
            if (!EQAlertFile.exists()) {
                std::cout << QString("緊急地震速報(警報)のログファイルが存在しないため作成します %1").arg(m_AlertFile).toStdString() << std::endl;

                if (EQAlertFile.open(QIODevice::WriteOnly)) {
                    EQAlertFile.close();
                }
                else {
                    std::cerr << QString("エラー : 緊急地震速報(警報)のログファイルの作成に失敗 %1").arg(EQAlertFile.errorString()).toStdString() << std::endl;
                    return -1;
                }
            }
            else {
                QFileInfo EQFileInfo(m_AlertFile);
                if (!EQFileInfo.permission(QFile::ReadUser |QFile::WriteUser)) return -1;
            }
        }

        // 発生した地震情報の有効 / 無効
        m_bEQInfo = earthquakeObj.value("info").toBool(false);
        if (m_bEQInfo) {
            /// 読み書き権限の確認
            m_InfoFile = earthquakeObj.value("infolog").toString("/tmp/eqinfo.log");
            if (m_InfoFile.isEmpty()) {
                m_InfoFile = QString("/tmp/eqinfo.log");
            }

            /// 書き込み済み(ログ用)のJSONファイルが存在しない場合は空のログファイルを作成
            QFile EQInfoFile(m_InfoFile);
            if (!EQInfoFile.exists()) {
                std::cout << QString("発生した地震情報のログファイルが存在しないため作成します %1").arg(m_InfoFile).toStdString() << std::endl;

                try {
                    if (EQInfoFile.open(QIODevice::WriteOnly)) {
                        // 空のJSON配列を作成
                        QJsonArray jsonArray;

                        // 配列を使用してJSONドキュメントを作成
                        QJsonDocument jsonDoc(jsonArray);

                        // 空のJSONドキュメントをログファイルに書き込む
                        EQInfoFile.write(jsonDoc.toJson());

                        EQInfoFile.close();
                    }
                    else {
                        std::cerr << QString("エラー : 発生した地震情報のログファイルの作成に失敗 %1").arg(EQInfoFile.errorString()).toStdString() << std::endl;
                        return -1;
                    }
                }
                catch (QException &ex) {
                    std::cerr << QString("エラー : 発生した地震情報のログファイルの作成に失敗 %1").arg(ex.what()).toStdString() << std::endl;
                    return -1;
                }
            }
            else {
                QFileInfo EQFileInfo(m_InfoFile);
                if (!EQFileInfo.permission(QFile::ReadUser |QFile::WriteUser)) return -1;
            }
        }

        // 震度の閾値
        const std::set<int> allowedScale = {10, 20, 30, 40, 45, 50, 55, 60, 70};

        m_AlertScale = earthquakeObj.value("alertscale").toInt(50);
        if (allowedScale.find(m_AlertScale) == allowedScale.end()) {
            std::cout << QString("警告 : 震度の閾値が不正です(緊急地震速報) - 設定値 : %1").arg(m_AlertScale).toStdString() << std::endl;
            std::cout << QString("強制的に50 (震度5強) に設定されます").toStdString() << std::endl;

            m_AlertScale = 50;
        }

        m_InfoScale = earthquakeObj.value("infoscale").toInt(50);
        if (allowedScale.find(m_InfoScale) == allowedScale.end()) {
            std::cout << QString("警告 : 震度の閾値が不正です(発生した地震情報) - 設定値 : %1").arg(m_InfoScale).toStdString() << std::endl;
            std::cout << QString("強制的に50 (震度5強) に設定されます").toStdString() << std::endl;

            m_InfoScale = 50;
        }

        // 震度画像を取得するための設定オブジェクト
        QJsonObject imageObj = JsonObject.value("image").toObject();

        /// 震度画像の取得機能の有効 / 無効
        m_EQImageInfo.bEnable = imageObj.value("enable").toBool(false);

        if (m_EQImageInfo.bEnable) {
            /// Yahoo天気・災害の地震情報一覧のURL
            m_EQImageInfo.Url = QUrl(imageObj.value("url").toString(""));

            /// Yahoo天気・災害の地震情報の起点となるURL
            m_EQImageInfo.BaseUrl = imageObj.value("baseurl").toString("");

            /// 地震情報の震度画像を取得するための日時形式
            m_EQImageInfo.DateFormat = imageObj.value("eqdateformat").toString("");

            /// 該当する地震情報のURLを取得するためのXPath式 (テーブル)
            m_EQImageInfo.ListXPath = imageObj.value("eqlistxpath").toString("");

            /// 該当する地震情報のURLを取得するためのXPath式 (テーブル内の要素)
            m_EQImageInfo.DetailXPath = imageObj.value("eqdetailxpath").toString("");

            /// 該当する地震情報のURLを取得するためのXPath式 (テーブル内のaタグのhref要素)
            m_EQImageInfo.UrlXPath = imageObj.value("equrlxpath").toString("");

            /// 該当する地震情報の震度画像を取得するためのXPath式
            m_EQImageInfo.ImgXPath = imageObj.value("imgxpath").toString("");
        }

        // メンバ変数m_EQIntervalの値を使用して自動的に地震情報を取得するかどうか
        // ワンショット機能の有効 / 無効
        m_bOneShot = JsonObject.value("oneshot").toBool(false);

        // ワンショット機能が有効の場合、タイマ割り込みの設定
        if (!m_bOneShot) {
            // 地震情報の取得の時間間隔が5秒未満、または、60秒を超える場合は、強制的に10秒に設定
            m_EQInterval = JsonObject.value("interval").toInt(10);
            if (m_EQInterval < 5 || m_EQInterval > 60) {
                std::cout << QString("地震情報の取得間隔が不正です 設定値 : %1").arg(m_EQInterval).toStdString() << std::endl;
                std::cout << QString("強制的に10[秒]に設定されます").toStdString() << std::endl;

                m_EQInterval = 10;
            }
            m_EQInterval *= 1000;
        }

        // スレッド情報の設定
        QJsonObject threadObj = JsonObject.value("thread").toObject();

        /// POSTデータを送信するURL
        m_RequestURL          = threadObj.value("requesturl").toString("");

        /// 地震情報で新規スレッドを作成するための名前欄
        m_ThreadInfo.from     = threadObj.value("from").toString("佐藤");

        /// スレッドに入力するメール欄
        m_ThreadInfo.mail     = threadObj.value("mail").toString("");

        /// BBS名
        m_ThreadInfo.bbs      = threadObj.value("bbs").toString("");

        /// Shift-JISの有効 / 無効
        m_ThreadInfo.shiftjis = threadObj.value("shiftjis").toBool(true);

        /// 緊急地震地震速報で新規スレッドを作成する場合、スレッドタイトルに地震発現(到達)時刻を記載するかどうか
        /// trueの場合、スレッドタイトルに"発現時刻 hh:mm:ss"という文字列が付加される
        m_EQsubTime         = threadObj.value("subjecttime").toBool("true");

        /// スレッドの生存を判断するときに使用するXPath
        /// デフォルトは、"/html/head/title"タグを取得する
        m_ExpiredXPath      = threadObj.value("expiredxpath").toString("/html/head/title");

        /// 発生した地震情報において、既存のスレッドに書き込む場合、スレッドのタイトルを変更するかどうか
        /// この機能は、防弾嫌儲およびニュース速報(Libre)等のスレッドタイトルが変更できる掲示板で使用可能
        m_EQchangeTitle     = threadObj.value("chtt").toBool(false);
    }
    catch(QException &ex) {
        std::cerr << QString("エラー : %1").arg(ex.what()).toStdString() << std::endl;
        return -1;
    }

    return 0;
}


// [q]キーまたは[Q]キー ==> [Enter]キーを押下した場合、メインループを抜けて本ソフトウェアを終了する
void Runner::onReadyRead()
{
    // 標準入力から1行のみ読み込む
    QTextStream tstream(stdin);
    QString line = tstream.readLine();

    if (line.compare("q", Qt::CaseInsensitive) == 0) {
        m_stopRequested.store(true);

        QCoreApplication::exit();
        return;
    }
}
