#include <QTimeZone>
#include <iostream>
#include <cmath>
#include <utility>
#include "EarthQuake.h"
#include "HtmlFetcher.h"


[[maybe_unused]] EarthQuake::EarthQuake(QObject *parent) : QObject(parent)
{

}


EarthQuake::EarthQuake(bool bEQAlert, bool bEQInfo, int AlertScale, int InfoScale, QString AlertFile, QString InfoFile,
                       QString RequestURL, bool bSubjectTime, bool bEQChangeTitle, THREAD_INFO ThreadInfo, QObject *parent) :
    m_bEQAlert(bEQAlert), m_bEQInfo(bEQInfo), m_AlertScale(AlertScale), m_InfoScale(InfoScale),
    m_AlertFile(std::move(AlertFile)), m_InfoFile(std::move(InfoFile)), m_RequestURL(std::move(RequestURL)), m_bSubjectTime(bSubjectTime),
    m_EQchangeTitle(bEQChangeTitle), m_ThreadInfo(std::move(ThreadInfo)), QObject{parent}
{

}


EarthQuake::~EarthQuake() = default;


int EarthQuake::EQProcess()
{
    // 緊急地震速報(警報)の処理を実行
    // 現在は非同期(マルチスレッド)で実行しない
    if (m_bEQAlert) {
        if (m_pEQAlertWorker == nullptr) {
            COMMONDATA data = {
                m_AlertScale,
                m_InfoScale,
                "https://api.p2pquake.net/v2/history?codes=556&limit=1&offset=0",
                m_RequestURL,
                m_AlertFile,
                m_bSubjectTime,
                false,
            };

            m_pEQAlertWorker = std::make_unique<Worker>(data, m_ThreadInfo);
        }

        m_pEQAlertWorker->ProcessEQAlert();
    }

    // 発生した地震情報の処理を実行
    // 現在は非同期(マルチスレッド)で実行しない
    if (m_bEQInfo) {
        if (m_pEQInfoWorker == nullptr) {
            COMMONDATA data = {
                m_AlertScale,
                m_InfoScale,
                "https://api.p2pquake.net/v2/history?codes=551&limit=1&offset=0",
                m_RequestURL,
                m_InfoFile,
                false,
                m_EQchangeTitle
            };

            m_pEQInfoWorker = std::make_unique<Worker>(data, m_ThreadInfo);
        }

        m_pEQInfoWorker->ProcessEQInfo();
    }

    return 0;
}


Worker::Worker(QObject *parent) : QObject(parent)
{

}


Worker::Worker(COMMONDATA commondata, THREAD_INFO threadInfo, QObject *parent)
    : m_CommonData(std::move(commondata)), m_ThreadInfo(std::move(threadInfo)), QObject(parent)
{

}


// 取得したデータを整形およびスレッド情報へ変換後、新規スレッドを作成する (緊急地震速報用)
int Worker::ProcessEQAlert()
{
    // P2P地震情報からデータを取得
    if (onEQDownloaded()) {
        // P2P地震情報のデータの取得に失敗した場合
        return -1;
    }

    // 取得したデータを整形
    if (FormattingData()) {
        // 古い地震情報、以前スレッドを立てた地震情報、データの整形に失敗した場合
        return -1;
    }

    // 整形したデータをスレッド情報へ変換
    if (FormattingThreadInfo()) {
        return -1;
    }

    // 緊急地震速報(警報)の場合は掲示板にスレッドを新規作成する
    // 発生した地震情報の場合は既存のスレッドが存在すれば該当スレッドに書き込む
    // 該当スレッドが存在しない場合はスレッドを新規作成する
    if (Post(m_Alert.m_Code)) {
        // スレッドの新規作成に失敗した場合
        return -1;
    }

    // 地震IDを地震情報のログファイルに保存
    if (!m_Alert.m_ID.isEmpty()) m_Alert.addInfo(m_CommonData.LogFile);

    return 0;
}


// 取得したデータを整形およびスレッド情報へ変換後、
// 既存スレッドに書き込み、または、新規スレッドを作成する (発生した地震情報用)
int Worker::ProcessEQInfo()
{
    // P2P地震情報からデータを取得
    if (onEQDownloaded()) {
        // P2P地震情報のデータの取得に失敗した場合
        return -1;
    }

    // 取得したデータを整形
    if (FormattingData()) {
        // 古い地震情報、以前スレッドを立てた地震情報、データの整形に失敗した場合
        return -1;
    }

    // 整形したデータをスレッド情報へ変換
    if (FormattingThreadInfo()) {
        return -1;
    }

    // 発生した地震情報のログファイルに同じ震源地が存在し、かつ、該当スレッドが生存している場合のみ既存のスレッドに書き込む
    // それ以外は、スレッドを新規作成する
    // ただし、ログファイルに同じ震源地が存在し、かつ、該当スレッドが生存していない場合は、ログファイルから該当するオブジェクトを削除する
    if (GetExistObject(m_Info.m_Name)) {
        // 発生した地震情報のログファイルに同じ震源地が存在する場合

        // ログファイルから過去に作成したスレッドのURLが生存しているかどうかを確認
        if (isExistThread(QUrl(m_InfoLog.ThreadURL), m_InfoLog.title)) {
            // 過去に作成したスレッドのURLが生存している場合、既存のスレッドに書き込む

            // !chttコマンドを使用する場合 (防弾嫌儲系の掲示板で使用可能)
            if (m_CommonData.bChangeTitle) {
                m_ThreadInfo.message = AddChttCommand(m_ThreadInfo.subject, m_ThreadInfo.message);
            }

            // 地震情報のログファイルからスレッド番号をセット
            if (!m_InfoLog.ThreadNum.isEmpty()) {
                m_ThreadInfo.key = m_InfoLog.ThreadNum;
            }
            else {
                std::cerr << QString("エラー : スレッド番号が不明です").toStdString() << std::endl;
                return -1;
            }

            // 既存のスレッドに書き込む
            if (Post(m_Info.m_Code, false)) {
                // 既存のスレッドの書き込みに失敗した場合
                return -1;
            }

            // 地震IDおよび地震発生日時を地震情報のログファイルに追加・更新
            if (!m_Info.m_ID.isEmpty()) {
                if (m_Info.UpdateInfo(m_CommonData.LogFile)) return -1;
            }
        }
        else {
            // 過去に作成したスレッドのURLが生存していない場合、スレッドを新規作成
            if (Post(m_Info.m_Code)) {
                // スレッドの新規作成に失敗した場合
                return -1;
            }

            // 過去に作成したスレッドがあるオブジェクトをログファイルから削除
            if (DeleteObject(m_Info.m_Name)) {
                // ログファイルから生存していないスレッド情報の削除に失敗した場合
                return -1;
            }

            // 地震ID、地震発生日時、スレッド情報を地震情報のログファイルに新規保存
            if (!m_Info.m_ID.isEmpty()) {
                if (m_Info.addInfo(m_CommonData.LogFile, m_ThreadInfo.subject, m_InfoLog.ThreadURL, m_InfoLog.ThreadNum)) return -1;
            }
        }
    }
    else {
        // 地震情報のログファイルに同じ震源地が存在しない場合

        // スレッドを新規作成する
        if (Post(m_Info.m_Code)) {
            // スレッドの新規作成に失敗した場合
            return -1;
        }

        // 地震ID、地震発生日時、スレッド情報を地震情報のログファイルに新規保存
        if (!m_Info.m_ID.isEmpty()) {
            if (m_Info.addInfo(m_CommonData.LogFile, m_ThreadInfo.subject, m_InfoLog.ThreadURL, m_InfoLog.ThreadNum)) return -1;
        }
    }

    return 0;
}


// 緊急地震速報(警報)および発生した地震情報のデータを取得する
int Worker::onEQDownloaded()
{
    // レスポンス待機の設定
    QEventLoop loop;
    m_pEQManager = std::make_unique<QNetworkAccessManager>();
    connect(m_pEQManager.get(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    // P2P地震情報へGETリクエストを送信
    QUrl url(m_CommonData.P2PEQInfoURL);
    QNetworkRequest request(url);

    // タイムアウトを3[秒]に設定
    request.setTransferTimeout(3000);

    auto pReply = m_pEQManager->get(request);

    // レスポンス待機
    loop.exec();

    // レスポンスの確認
    if (pReply->error() == QNetworkReply::NoError) {
        // 正常に取得した場合
        m_ReplyData = pReply->readAll();

#ifdef _DEBUG
        std::cout << m_ReplyData.constData() << std::endl;
#endif

    }
    else {
        // 地震情報の取得に失敗した場合
        std::cerr << QString("エラー : 地震情報の取得に失敗 %1").arg(pReply->errorString()).toStdString() << std::endl;
        pReply->deleteLater();

        return -1;
    }

    pReply->deleteLater();

    return 0;
}


// 取得した地震情報を整形する
int Worker::FormattingData()
{
    // ダウンロードした地震情報のデータを読み込む
    auto responseData = m_ReplyData;
    auto doc          = QJsonDocument::fromJson(responseData);

    // ダウンロードした地震情報のデータを確認
    if (!doc.isArray()) {
        std::cerr << QString("エラー : ダウンロードした地震情報のデータに異常があります").toStdString() << std::endl;
        return -1;
    }

    // 新しい地震情報かどうかを判断するフラグ
    bool bNewEarthQuake = false;

    // ユーザが設定した震度以上のエリアが1つ以上存在するかどうかを確認
    // 存在する場合は、緊急地震速報(警報)とする
    // ダウンロードした情報はJSONの配列となっているが、最新情報の1件のみ取得していることに注意する
    auto rootArray = doc.array();
    for (const auto &value : rootArray) {
        auto obj = value.toObject();

        // "code"キーを確認
        // 556 : 緊急地震速報(警報)
        // 551 : 発生した地震情報
        auto code = obj["code"].toInt();

        if (code == 556) {
            // 緊急地震速報(警報)

            /// "issue"キー内の"time"キーの値を取得して時刻を変換
            auto issueObj       = obj["issue"].toObject();
            auto timeStr        = issueObj["time"].toString();
            auto issueTime      = QDateTime::fromString(timeStr, "yyyy/MM/dd HH:mm:ss");
            auto currentTime    = QDateTime::currentDateTime();
            currentTime.setTimeZone(QTimeZone("Asia/Tokyo"));

            /// 現在時刻と比較して、緊急地震速報(警報)の最新情報が30[秒]以内かどうかを確認
            /// 30[秒]以内の地震情報の場合は取得
            qint64 diff = static_cast<int>(issueTime.msecsTo(currentTime) / 1000);
            if (!(diff >= 0 && diff <= 30)) {
                /// 30[秒]を超過している緊急地震速報(警報)の情報は無視する
                continue;
            }

            /// 以前と同じ地震情報かどうかをログファイルに保存されている地震IDから確認
            /// 過去に同じIDの緊急地震速報(警報)を取得しているかどうか
            auto id = obj["id"].toString();
            if (!SearchAlertEQID(id)) {
                /// ログファイルに同じ地震IDが存在する場合は無視する
                continue;
            }

            /// 緊急地震速報(警報)のオブジェクトをクリア
            m_Alert = EarthQuakeAlert();

            /// ユーザが設定した震度の閾値を確認
            /// 1つでも閾値以上の地震が各地域で予測される場合は、緊急地震速報(警報)の情報を取得
            auto areas = obj["areas"].toArray();
            bool AboveSeismicIntensity = false;
            for (const auto &areaValue : areas) {
                QJsonObject areaObj = areaValue.toObject();
                int scaleTo = areaObj["scaleTo"].toInt(-1);

                if (scaleTo >= m_CommonData.AlertScale) {
                    AboveSeismicIntensity = true;
                    break;
                }
            }

            /// ユーザが設定した震度以上の地域が存在する場合、緊急地震速報(警報)の情報を取得
            if (AboveSeismicIntensity) {
                /// 緊急地震速報(警報)が出ているエリアを取得
                bNewEarthQuake = true;

                for (const auto &areaValue : areas) {
                    AREA area;
                    auto areaObj        = areaValue.toObject();
                    area.KindCode       = areaObj["kindCode"].toString("");
                    area.Name           = areaObj["name"].toString("");
                    area.ArrivalTime    = areaObj["arrivalTime"].toString("");

                    QVariant from       = areaObj["scaleFrom"].toDouble(0.0f);
                    area.ScaleFrom      = ConvertNumberToInt(from);

                    QVariant to         = areaObj["scaleTo"].toInt(-1);
                    area.ScaleTo        = ConvertNumberToInt(to);

                    m_Alert.m_Areas.append(area);
                }

                /// 緊急地震速報(警報)の地域に対して、最大震度の大きさで降順にソート
                if (m_Alert.m_Areas.count() > 1) {
                    std::sort(m_Alert.m_Areas.begin(), m_Alert.m_Areas.end(), sortAreas);
                }

                /// 地震の情報を表すコード
                m_Alert.m_Code        = obj["code"].toInt(556);

                /// "earthquake"キーから値を取得
                auto earthquakeObj    = obj["earthquake"].toObject();
                m_Alert.m_OriginTime  = earthquakeObj["originTime"].toString();       // 地震発生時刻
                m_Alert.m_ArrivalTime = earthquakeObj["arrivalTime"].toString();      // 地震発現(到達)時刻

                auto hypocenterObj    = earthquakeObj["hypocenter"].toObject();
                m_Alert.m_Name        = hypocenterObj["name"].toString();             // 震源地

                auto magnitude        = hypocenterObj["magnitude"].toDouble(0.0f);    // マグニチュード
                m_Alert.m_Magnitude   = ConvertMagnitude(magnitude);

                QVariant depthVal     = hypocenterObj["depth"].toDouble(0.0f);        // 震源の深さ
                m_Alert.m_Depth       = ConvertNumberToString(depthVal);

                auto latitude         = hypocenterObj["latitude"].toDouble(0.0f);     // 緯度  震源情報が存在しない場合は、-200または-200.0
                m_Alert.m_Latitude    = QString::number(latitude, 'f', 1);

                auto longitude        = hypocenterObj["longitude"].toDouble(0.0f);    // 経度  震源情報が存在しない場合は、-200または-200.0
                m_Alert.m_Longitude   = QString::number(longitude, 'f', 1);

                /// IDを緊急地震速報(警報)のログファイルに保存
                m_Alert.m_ID = obj["id"].toString();
            }
        }
        else if (code == 551) {
            // 発生した地震情報

            /// "earthquake"オブジェクトの"time"キーに対応する値の取得
            auto earthquakeObject   = obj["earthquake"].toObject();
            auto timeStr            = earthquakeObject["time"].toString();
            auto issueTime          = QDateTime::fromString(timeStr, "yyyy/MM/dd HH:mm:ss");
            auto currentTime        = QDateTime::currentDateTime();
            currentTime.setTimeZone(QTimeZone("Asia/Tokyo"));

            /// 現在時刻と比較して、緊急地震速報(警報)の最新情報が5[分]以内かどうかを確認
            /// 5[分]以内の地震情報の場合は取得
            qint64 diff = static_cast<int>(issueTime.msecsTo(currentTime) / 1000);
            if (!(diff >= 0 && diff <= 600)) {
                /// 発生した地震情報において、600[秒](10[分])を超過している場合は無視する
                continue;
            }

            /// ユーザが設定した震度の閾値を確認
            /// 1つでも閾値以上の地震が各地域で発生した場合は、発生した地震情報を取得
            auto scale = obj["earthquake"].toObject()["maxScale"].toInt(-1);
            if (scale < m_CommonData.InfoScale) {
                continue;
            }

            /// 震源地の情報が存在するかどうかを確認
            auto hypo  = obj["earthquake"].toObject()["hypocenter"].toObject()["name"].toString("");
            if (hypo.isEmpty()) {
                /// 震源地が不明の場合は無視する
                continue;
            }

            /// 以前と同じ地震情報かどうかをログファイルに保存されている地震IDから確認
            /// 過去に同じIDの地震情報を取得しているかどうか
            auto id = obj["id"].toString();
            if (!SearchInfoEQID(id)) {
                /// ログファイルに同じ地震IDが存在する場合は無視する
                continue;
            }

            bNewEarthQuake = true;

            /// 発生した地震情報のオブジェクトをクリア
            m_Info = EarthQuakeInfo();

            /// ユーザが設定した震度以上の地域が存在する場合、発生した地震情報を取得
            /// 発生した地震情報が出ている地域を取得
            auto points = obj["points"].toArray();
            for (const auto &pointValue : points) {
                POINT point;
                auto pointObj   = pointValue.toObject();
                point.Addr      = pointObj["addr"].toString("");
                point.Pref      = pointObj["pref"].toString("");
                point.Scale     = pointObj["scale"].toInt(-1);
                point.IsArea    = pointObj["isArea"].toBool(false);
                m_Info.m_Points.append(point);
            }

            /// 発生した地震情報の地域に対して、震度の大きさで降順にソート
            if (m_Info.m_Points.count() > 1) {
                std::sort(m_Info.m_Points.begin(), m_Info.m_Points.end(), sortPoints);
            }

            /// 地震の情報を表すコード
            m_Info.m_Code        = obj["code"].toInt(551);

            /// "earthquake"キーから値を取得
            auto earthquakeObj       = obj["earthquake"].toObject();
            auto maxscale            = ConvertScale(earthquakeObj["maxScale"].toInt(-1));   // 最大震度
            m_Info.m_MaxScale        = maxscale;
            m_Info.m_Time            = timeStr;                                             // 地震発生時刻
            m_Info.m_DomesticTsunami = earthquakeObj["domesticTsunami"].toString();         // 国内での津波の有無
            m_Info.m_ForeignTsunami  = earthquakeObj["foreignTsunami"].toString();          // 海外での津波の有無

            auto hypocenterObj   = earthquakeObj["hypocenter"].toObject();
            m_Info.m_Name        = hypocenterObj["name"].toString();             // 震源地

            auto magnitude       = hypocenterObj["magnitude"].toDouble(0.0f);    // マグニチュード
            m_Info.m_Magnitude   = ConvertMagnitude(magnitude);

            auto depth           = hypocenterObj["depth"].toDouble(0.0f);        // 震源の深さ
            auto intDepth        = static_cast<int>(depth);                      // P2P地震情報ではシステムの都合で小数点が付加される場合はあるが、整数部のみ有効である
            m_Info.m_Depth       = QString::number(intDepth);

            auto latitude        = hypocenterObj["latitude"].toDouble(0.0f);     // 緯度  震源情報が存在しない場合は、-200または-200.0
            m_Info.m_Latitude    = QString::number(latitude, 'f', 1);

            auto longitude       = hypocenterObj["longitude"].toDouble(0.0f);    // 経度  震源情報が存在しない場合は、-200または-200.0
            m_Info.m_Longitude   = QString::number(longitude, 'f', 1);

            /// IDを緊急地震速報(警報)のログファイルに保存
            m_Info.m_ID = obj["id"].toString();
        }
    }

    if (!bNewEarthQuake) return -1;

    return 0;
}


// 整形した地震情報のデータをスレッド情報へ整形する
int Worker::FormattingThreadInfo()
{
    // スレッド情報を作成
    if (m_Alert.m_Code == 556) {
        // 緊急地震速報(警報)の場合

        // スレッドのタイトル
        auto name      = !m_Alert.m_Name.isEmpty() ? QString("%1 ").arg(m_Alert.m_Name) : QString("");
        auto magnitude = m_Alert.m_Magnitude != -1 ? QString("M%1 ").arg(m_Alert.m_Magnitude) : QString("");
        auto dateTime  = QDateTime::fromString(m_Alert.m_ArrivalTime, "yyyy/MM/dd HH:mm:ss");
        auto timeStr   = dateTime.isValid() && m_CommonData.bSubjectTime ? dateTime.time().toString("HH:mm:ss") : QString("");
        m_ThreadInfo.subject = QString("【緊急地震速報】%1%2%3強い揺れに警戒").arg(name,
                                                                              magnitude,
                                                                              !timeStr.isEmpty() ? QString("発現時刻 %1 ").arg(timeStr) : QString(""));

        // スレッドの内容
        /// 震源地
        m_ThreadInfo.message  = name.isEmpty() ? QString("震源地 : 不明") + "\n" : QString("震源地 : %1").arg(name + "\n");

        /// マグニチュード
        m_ThreadInfo.message += magnitude.isEmpty() ? QString("マグニチュードの情報なし") + "\n" : QString(magnitude + "\n");

        /// 震源の深さ
        auto depth = m_Alert.m_Depth == "0" ? QString("ごく浅い") : m_Alert.m_Depth == "-1" ? QString("情報なし") : QString(m_Alert.m_Depth + "[km]");
        m_ThreadInfo.message += QString("震源の深さ : %1").arg(depth + "\n\n");

        /// 緯度
        if (m_Alert.m_Latitude.compare("-200", Qt::CaseInsensitive) == 0 || m_Alert.m_Latitude.compare("-200.0", Qt::CaseInsensitive) == 0) {
            m_ThreadInfo.message += QString("緯度 : 情報なし") + "\n";
        }
        else {
            m_ThreadInfo.message += QString("北緯 : %1度").arg(m_Alert.m_Latitude) + "\n";
        }

        /// 経度
        if (m_Alert.m_Longitude.compare("-200", Qt::CaseInsensitive) == 0 || m_Alert.m_Longitude.compare("-200.0", Qt::CaseInsensitive) == 0) {
            m_ThreadInfo.message += QString("経度 : 情報なし") + "\n";
        }
        else {
            m_ThreadInfo.message += QString("東経 : %1度").arg(m_Alert.m_Longitude) + "\n";
        }

        /// 地震発生時刻
        auto originTime     = m_Alert.m_OriginTime.isEmpty() ? QString("") + "\n" : QString("地震発生時刻 : %1").arg(m_Alert.m_OriginTime + "\n");
        m_ThreadInfo.message += originTime;

        /// 地震発現(到達)時刻
        auto arrivalTime    = m_Alert.m_ArrivalTime.isEmpty() ? QString("") + "\n" : QString("地震発現(到達)時刻 : %1").arg(m_Alert.m_ArrivalTime + "\n\n");
        m_ThreadInfo.message += arrivalTime;

        /// 緊急地震速報(警報)の対象地域
        /// 現在の仕様では、最大で7つのエリアまで表示する
        if (m_Alert.m_Areas.count() > 0) {
            /// メッセージを追加
            m_ThreadInfo.message += QString("地震が予想される地域") + "\n";
        }

        auto count    = 0;
        auto kindcode = false;
        for (auto &area : m_Alert.m_Areas) {
            // area.ScaleToが99の場合は"以上"を表す
            if (area.ScaleTo == 99) {
                m_ThreadInfo.message += QString("%1 : 震度 %2%3").arg(area.Name,
                                                                     area.ScaleFrom == -1 ? "不明" : ConvertScale(area.ScaleFrom),
                                                                     ConvertScale(area.ScaleTo));
            }
            else {
                if (area.ScaleFrom == area.ScaleTo) m_ThreadInfo.message += QString("%1 : 震度 %2")
                                                                            .arg(area.Name,
                                                                                 area.ScaleFrom == -1 ? "不明" : ConvertScale(area.ScaleFrom));
                else m_ThreadInfo.message += QString("%1 : 震度 %2 〜 %3").arg(area.Name,
                                                                              area.ScaleFrom == -1 ? "不明" : ConvertScale(area.ScaleFrom),
                                                                              area.ScaleTo   == -1 ? "" : ConvertScale(area.ScaleTo));
            }

            m_ThreadInfo.message +=  "\n";

            /// 既に地震が到達しているかどうかを確認
            if (area.KindCode.compare("11", Qt::CaseSensitive) == 0) {
                kindcode = true;
            }

            /// 7つのエリアを超えるエリアが存在する場合、それ以上は記載しない
            if (count >= 6) break;
            else            count++;
        }

        /// 7つの地域を超える地域が存在する場合、"その他の地域"と記載する
        if (m_Alert.m_Areas.count() > 7) {
            m_ThreadInfo.message += QString("その他の地域") + "\n";
        }

        if (kindcode) {
            m_ThreadInfo.message += "\n" + QString("既に地震が到達していると予想されます") + "\n";
        }
    }
    else if (m_Info.m_Code == 551) {
        // 発生した地震情報の場合

        // スレッドのタイトル
        auto name      = !m_Info.m_Name.isEmpty() ? QString("%1").arg(m_Info.m_Name) : QString("");
        auto maxscale  = m_Info.m_MaxScale  != "不明" ? QString("震度%1").arg(m_Info.m_MaxScale) : QString("");
        auto magnitude = m_Info.m_Magnitude != "-1" ? QString("M%1").arg(m_Info.m_Magnitude) : QString("");
        m_ThreadInfo.subject = QString("【地震】%1%2%3").arg(name.isEmpty() ? QString("") : name + QString(" "),
                                                            maxscale.isEmpty() ? QString("") : maxscale + QString(" "),
                                                            magnitude);

        // スレッドの内容
        /// 震源地
        m_ThreadInfo.message  = name.isEmpty() ? QString("震源地 : 不明") + "\n" : QString("震源地 : %1").arg(name + "\n");

        /// 震度
        m_ThreadInfo.message += maxscale.isEmpty() ? QString("最大震度情報なし") + "\n" : QString("最大%1").arg(maxscale + "\n");

        /// マグニチュード
        m_ThreadInfo.message += magnitude.isEmpty() ? QString("マグニチュードの情報なし") + "\n" : QString(magnitude + "\n");

        /// 震源の深さ
        auto depth = m_Info.m_Depth == "0" ? QString("ごく浅い") : m_Info.m_Depth == "-1" ? QString("情報なし") : QString(m_Info.m_Depth + "[km]");
        m_ThreadInfo.message += QString("震源の深さ : %1").arg(depth + "\n");

        /// 地震発生時刻
        if (m_Info.m_Time.isEmpty()) {
            m_ThreadInfo.message += QString("") + "\n";
        }
        else {
            auto dateTime    = QDateTime::fromString(m_Info.m_Time, "yyyy/MM/dd HH:mm:ss");
            QString convertTime;

            if (dateTime.time().second() == 0) convertTime = dateTime.toString("yyyy年M月d日 h時m分頃");  // 秒の部分が00の場合
            else convertTime = dateTime.toString("yyyy年M月d日 h時m分s秒");                               // 秒の部分が00以外の場合

            m_ThreadInfo.message += QString("地震発生時刻 : %1").arg(convertTime + "\n\n");
        }

        /// 緯度
        if (m_Info.m_Latitude.compare("-200", Qt::CaseInsensitive) == 0 || m_Info.m_Latitude.compare("-200.0", Qt::CaseInsensitive) == 0) {
            m_ThreadInfo.message += QString("緯度 : 情報なし") + "\n";
        }
        else {
            m_ThreadInfo.message += QString("北緯 : %1度").arg(m_Info.m_Latitude) + "\n";
        }

        /// 経度
        if (m_Info.m_Longitude.compare("-200", Qt::CaseInsensitive) == 0 || m_Info.m_Longitude.compare("-200.0", Qt::CaseInsensitive) == 0) {
            m_ThreadInfo.message += QString("経度 : 情報なし") + "\n";
        }
        else {
            m_ThreadInfo.message += QString("東経 : %1度").arg(m_Info.m_Longitude) + "\n\n";
        }

        /// 発生した地震の地域
        /// 現在の仕様では、最大で7つのエリアまで表示する
        if (m_Info.m_Points.count() > 0) {
            /// メッセージを追加
            m_ThreadInfo.message += QString("発生した地震の地域") + "\n";
        }

        auto count = 0;
        for (auto &point : m_Info.m_Points) {
            auto scale = point.Scale;
            m_ThreadInfo.message += QString("%1 : 震度 %2").arg(point.Addr, ConvertScale(scale)) + "\n";

            /// 7つの地域を超える地域が存在する場合、それ以上は記載しない
            if (count >= 6) break;
            else            count++;
        }

        /// 7つの地域を超える地域が存在する場合、"その他の地域"と記載する
        if (m_Info.m_Points.count() > 7) {
            m_ThreadInfo.message += QString("その他の地域") + "\n";
        }

        /// 国内への津波の有無
        if (!m_Info.m_DomesticTsunami.isEmpty()) {
            m_ThreadInfo.message += "\n" + QString("国内への津波の有無") + "\n";

            if (m_Info.m_DomesticTsunami.compare("None", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("なし") + "\n";
            else if (m_Info.m_DomesticTsunami.compare("Unknown", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("不明") + "\n";
            else if (m_Info.m_DomesticTsunami.compare("Checking", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("調査中") + "\n";
            else if (m_Info.m_DomesticTsunami.compare("NonEffective", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("若干の海面変動が予想されるが、被害の心配なし") + "\n";
            else if (m_Info.m_DomesticTsunami.compare("Watch", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("津波注意報") + "\n";
        }

        /// 海外での津波の有無
        if (!m_Info.m_ForeignTsunami.isEmpty()) {
            m_ThreadInfo.message += "\n" + QString("海外への津波の有無") + "\n";

            if (m_Info.m_ForeignTsunami.compare("None", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("なし") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("Unknown", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("不明") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("Checking", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("調査中") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("NonEffectiveNearby", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("震源の近傍で小さな津波の可能性があるが、被害の心配なし") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("WarningNearby", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("震源の近傍で津波の可能性がある") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("WarningPacific", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("太平洋で津波の可能性がある") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("WarningPacificWide", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("太平洋の広域で津波の可能性がある") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("WarningIndian", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("インド洋で津波の可能性がある") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("WarningIndianWide", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("インド洋の広域で津波の可能性がある") + "\n";
            else if (m_Info.m_ForeignTsunami.compare("Potential", Qt::CaseSensitive) == 0)
                m_ThreadInfo.message += QString("一般にこの規模では津波の可能性がある") + "\n";
        }
    }

    // 現在時刻をエポックタイムで取得
    auto epocTime = GetEpocTime();
    m_ThreadInfo.time = QString::number(epocTime);

    return 0;
}


int Worker::Post(int EQCode, bool bCreateThread)
{
    Poster poster(nullptr);

    // 掲示板のクッキーを取得
    if (poster.fetchCookies(QUrl(m_CommonData.RequestURL))) {
        // クッキーの取得に失敗した場合
        return -1;
    }

    // POSTデータの送信
    if (bCreateThread) {
        // スレッドを新規作成
        // 緊急地震速報(警報)の場合、または、発生した地震情報において既存のスレッドが存在しない場合
        if (poster.PostforCreateThread(QUrl(m_CommonData.RequestURL), m_ThreadInfo)) {
            // スレッドの新規作成に失敗した場合、または、スレッドの書き込みに失敗した場合
            return -1;
        }

        // 発生した地震情報の場合、新規作成したスレッドのURLとスレッド番号を取得
        if (EQCode == 551) {
            m_InfoLog.ThreadURL = poster.GetNewThreadURL();
            m_InfoLog.ThreadNum = poster.GetNewThreadNum();
        }
    }
    else {
        // 既存のスレッドに書き込む
        // 発生した地震情報において既存のスレッドが存在する場合
        if (poster.PostforWriteThread(QUrl(m_CommonData.RequestURL), m_ThreadInfo)) {
            // スレッドの新規作成に失敗した場合、または、スレッドの書き込みに失敗した場合
            return -1;
        }
    }

    return 0;
}


// 地震IDを保存している緊急地震速報のログファイルから地震IDを検索する
bool Worker::SearchAlertEQID(const QString &ID) const
{
    // ログファイルを読み込む
    QFile File(m_CommonData.LogFile);
    if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << QString("エラー : 地震情報のログファイルのオープンに失敗しました %1").arg(File.errorString()).toStdString() << std::endl;
        return false;
    }

    try {
        QTextStream in(&File);
        while (!in.atEnd()) {
            auto line = in.readLine();
            if (line == ID) {

#ifdef _DEBUG
                std::cout << QString("同じ地震IDが存在するため、この地震情報を無視します : %1").arg(ID).toStdString() << std::endl;
#endif

                File.close();
                return false;
            }
        }
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 地震情報のログファイルの読み込みに失敗しました %1").arg(ex.what()).toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return false;
    }

    File.close();

#ifdef _DEBUG
    std::cout << QString("同じ地震IDは存在しないため、地震情報の取得を開始します : %1").arg(ID).toStdString() << std::endl;
#endif

    return true;
}


// 地震IDを保存している発生した地震情報のログファイルから地震IDを検索する
bool Worker::SearchInfoEQID(const QString &ID) const
{
    // ログファイルを読み込む
    QFile File(m_CommonData.LogFile);
    if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << QString("エラー : 地震情報のログファイルのオープンに失敗しました %1").arg(File.errorString()).toStdString() << std::endl;
        return false;
    }

    QJsonDocument document;
    try {
        document = QJsonDocument::fromJson(File.readAll());
        File.close();
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 地震情報のログファイルの読み込みに失敗しました %1").arg(ex.what()).toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return false;
    }

    if (!document.isArray()) {
        std::cerr << QString("エラー : 地震情報のログファイルの値が不正です").toStdString() << std::endl;
        return false;
    }

    QJsonArray jsonArray = document.array();
    for (const auto &value : jsonArray) {
        auto obj     = value.toObject();
        auto idArray = obj["id"].toArray();

        for (const auto &idValue : idArray) {
            if (idValue.toString().compare(ID, Qt::CaseSensitive) == 0) {
                // 既に地震IDが存在する場合
#ifdef _DEBUG
                std::cout << QString("同じ地震IDが存在するため、この地震情報を無視します : %1").arg(ID).toStdString() << std::endl;
#endif
                return false;
            }
        }
    }

#ifdef _DEBUG
    std::cout << QString("同じ地震IDは存在しないため、地震情報の取得を開始します : %1").arg(ID).toStdString() << std::endl;
#endif

    return true;
}


// 地震情報のログファイルから同じ震源地のオブジェクトを取得する
bool Worker::GetExistObject(const QString &hypo)
{
    // 地震情報のログファイルを開く
    QFile File(m_CommonData.LogFile);
    if (!File.open(QIODevice::ReadOnly)) {
        std::cerr << QString("エラー : 発生した地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
        return false;
    }

    try {
        // 地震情報のログファイルを読み込む
        auto document = QJsonDocument::fromJson(File.readAll());

        // ファイルを閉じる
        File.close();

        // 地震情報のログファイルから同じ震源地のオブジェクトが存在するかどうかを検索する
        auto jsonArray = document.array();
        for (const auto &value : jsonArray) {
            auto obj    = value.toObject();
            auto hypocentre = obj["hypocentre"].toString();

            // 同じ震源地かどうかを確認
            if (hypocentre.compare(hypo, Qt::CaseSensitive) == 0) {
                // 同じ震源地が存在する場合
                auto idArray = obj["id"].toArray();
                for(const auto &id : idArray) {
                    if(id.isString()) {
                        m_InfoLog.IDs.append(id.toString());
                    }
                }
                m_InfoLog.hypocenter  = obj["hypocentre"].toString();
                m_InfoLog.title       = obj["title"].toString();
                m_InfoLog.ThreadURL   = obj["url"].toString();
                m_InfoLog.ThreadNum   = obj["thread"].toString();
                m_InfoLog.Date        = obj["date"].toString();

                return true;
            }
        }
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 発生した地震情報のログファイルの読み込みに失敗").toStdString() << std::endl;
        if (File.isOpen()) File.close();
    }

    return false;
}


// 過去に作成したスレッドが生存しているかどうかを確認する
// HTMLの<head>タグ内の<title>タグを確認することにより判断する
bool Worker::isExistThread(const QUrl &url, const QString &title)
{
    // 過去に作成したスレッドから<title>タグをXPathを使用して抽出する
    HtmlFetcher fetcher(this);
    if (fetcher.fetch(url, true, "//head/title", m_ThreadInfo.shiftjis)) {
        // <title>タグの取得に失敗した場合
        std::cerr << QString("エラー : スレッドの生存確認に失敗").toStdString() << std::endl;
        return false;
    }

    // ログファイルに保存しているスレッドタイトルと同じスレッドタイトルか存在するかどうかを確認する
    // これは、スレッドが生存していない場合でもHTTPレスポンスが200(成功)を返す可能性があるためである
    auto ExistThread = fetcher.GetElement();
    if (ExistThread.compare(title, Qt::CaseSensitive) == 0) {
        // 既存のスレッドが生存している場合
        return true;
    }

    return false;
}


// ログファイルから生存していないスレッド情報を削除する
int Worker::DeleteObject(const QString &hypo) const
{
    // ログファイルを開く
    QFile File(m_CommonData.LogFile);
    if (!File.open(QIODevice::ReadOnly)) {
        std::cerr << QString("エラー : 発生した地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
        return -1;
    }

    // ログファイルの読み込み
    QJsonDocument document;
    try {
        document = QJsonDocument::fromJson(File.readAll());
        File.close();
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 地震情報のログファイルの読み込みに失敗しました %1").arg(ex.what()).toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return -1;
    }

    if (!document.isArray()) {
        std::cerr << QString("エラー : 地震情報のログファイルの値が不正です").toStdString() << std::endl;
        return -1;
    }

    QJsonArray jsonInArray = document.array();
    QJsonArray jsonOutArray = {};

    // 生存していないスレッド情報を削除
    for (const auto &value : jsonInArray) {
        auto obj = value.toObject();
        if (obj["hypocentre"].toString().compare(hypo, Qt::CaseSensitive) != 0) {
            jsonOutArray.append(obj);
        }
    }

    QJsonDocument outputDoc(jsonOutArray);

    // ログファイルを開く
    if (!File.open(QIODevice::WriteOnly)) {
        std::cerr << QString("エラー : 発生した地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
        return -1;
    }

    // ログファイルの更新
    try {
        File.write(outputDoc.toJson());
        File.close();
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 発生した地震情報のログファイルの更新に失敗").toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return -1;
    }

    return 0;
}


// 既存スレッドに書き込む場合、スレッド本文の先頭に!chttコマンドを挿入する
// 防弾嫌儲系の掲示板で使用可能
QString Worker::AddChttCommand(QString &Subject, QString &Message)
{
    return Message.prepend(QString("!chtt%1\n\n").arg(Subject));
}


// JSONファイルの震度の数値を実際の数値(文字列)に変換する
QString Worker::ConvertScale(int Scale)
{
    QString ScaleStr;

    switch (Scale) {
    case -1:
        ScaleStr = QString("不明");
        break;
    case 10:
        ScaleStr = QString("1");
        break;
    case 20:
        ScaleStr = QString("2");
        break;
    case 30:
        ScaleStr = QString("3");
        break;
    case 40:
        ScaleStr = QString("4");
        break;
    case 45:
        ScaleStr = QString("5弱");
        break;
    case 50:
        ScaleStr = QString("5強");
        break;
    case 55:
        ScaleStr = QString("6弱");
        break;
    case 60:
        ScaleStr = QString("6強");
        break;
    case 70:
        ScaleStr = QString("7");
        break;
    case 99:
        ScaleStr = QString("以上");
        break;
    default:
        ScaleStr = QString("不明");
    }

    return ScaleStr;
}


QString Worker::ConvertMagnitude(double Magnitude)
{
    // 数値が整数かどうかを確認
    if (std::floor(Magnitude) == Magnitude) {
        // 整数の場合は'.0'を除去
        return QString::number(static_cast<int>(Magnitude));
    }

    // 小数点以下がある場合はそのまま文字列に変換
    return QString::number(Magnitude);
}


// 緊急地震速報(警報)のdepth, scaleFrom, scaleToにおいて、
// 本来は整数値であるがシステムの都合で小数点が付加される場合があるため、小数点以下を除去して整数の文字列に変換する
int Worker::ConvertNumberToInt(const QVariant &value)
{
    if (value.type() == QVariant::Int || value.type() == QVariant::LongLong) {
        // 数値が整数の場合
        return value.toInt();
    }
    else if (value.type() == QVariant::Double) {
        // 数値が浮動小数点の場合
        double number = value.toDouble();

        // 小数点以下を切り捨て
        return static_cast<int>(std::floor(number));
    }

    // その他の型の場合は不明とする
    return -1;
}


QString Worker::ConvertNumberToString(const QVariant &value)
{
    if (value.type() == QVariant::Int || value.type() == QVariant::LongLong) {
        // 数値が整数の場合
        return value.toString();
    }
    else if (value.type() == QVariant::Double) {
        // 数値が浮動小数点の場合
        double number = value.toDouble();

        // 小数点以下を切り捨て
        double intPart;
        std::modf(number, &intPart);

        return QString::number(intPart, 'f', 0);
    }

    // その他の型の場合は文字列"-1"
    return {"-1"};
}

// 緊急地震速報(警報)の地域に対して、最大震度の大きさで降順にソート
bool Worker::sortAreas(const AREA &a, const AREA &b)
{
    // ScaleFromでの比較、-1は無視
    if (a.ScaleFrom != -1 && b.ScaleFrom != -1) {
        if (a.ScaleFrom != b.ScaleFrom) {
            return a.ScaleFrom > b.ScaleFrom;
        }
    }
    else if (a.ScaleFrom != -1) {
        return true;
    }
    else if (b.ScaleFrom != -1) {
        return false;
    }

    // ScaleToでの比較、-1と99は無視
    if ((a.ScaleTo != -1 && a.ScaleTo != 99) && (b.ScaleTo != -1 && b.ScaleTo != 99)) {
        return a.ScaleTo > b.ScaleTo;
    }
    else if (a.ScaleTo != -1 && a.ScaleTo != 99) {
        return true;
    }
    else {
        return false;
    }
}


// 発生した地震情報の地域に対して、震度の大きさで降順にソート
bool Worker::sortPoints(const POINT& a, const POINT& b)
{
    // Scaleが-1の場合は無視するため、常にfalseを返して後ろに配置
    if (a.Scale == -1) return false;
    if (b.Scale == -1) return true;

    // Scaleで降順ソート
    return a.Scale > b.Scale;
}


// コピーコンストラクタ
EarthQuakeAlert::EarthQuakeAlert(const EarthQuakeAlert &obj) noexcept
{
    // 共通のデータ
    this->m_Code        = obj.m_Code;
    this->m_ID          = obj.m_ID;
    this->m_Name        = obj.m_Name;
    this->m_Depth       = obj.m_Depth;
    this->m_Magnitude   = obj.m_Magnitude;
    this->m_Latitude    = obj.m_Latitude;
    this->m_Longitude   = obj.m_Longitude;

    // 緊急地震速報(警報)のデータ
    this->m_OriginTime   = obj.m_OriginTime;
    this->m_ArrivalTime  = obj.m_ArrivalTime;
    this->m_Areas        = obj.m_Areas;
}


// 代入演算子のオーバーロード
EarthQuakeAlert& EarthQuakeAlert::operator=(const EarthQuakeAlert &obj)
{
    if (this != &obj) {
        // リソースをコピー

        // 緊急地震速報(警報)のデータ
        this->m_Code            = obj.m_Code;
        this->m_ID              = obj.m_ID;
        this->m_Name            = obj.m_Name;
        this->m_Depth           = obj.m_Depth;
        this->m_Magnitude       = obj.m_Magnitude;
        this->m_Latitude        = obj.m_Latitude;
        this->m_Longitude       = obj.m_Longitude;
        this->m_OriginTime      = obj.m_OriginTime;
        this->m_ArrivalTime     = obj.m_ArrivalTime;
        this->m_Areas           = obj.m_Areas;
    }

    // このオブジェクトの参照を返す
    return *this;
}


int EarthQuakeAlert::addInfo(const QString &fileName)
{
    // ミューテックスをロックして、スレッドセーフを保証
    QMutexLocker locker(&m_Mutex);

    // 緊急地震速報(警報)の情報を追加
    QFile File(fileName);
    if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << QString("エラー : 地震情報のファイルオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
        return -1;
    }

    try {
        // ファイル内容を読み込む
        QTextStream in(&File);
        auto originalContent = in.readAll();
        File.close();

        // ファイル内容を書き込むために再度ファイルを開く
        if (!File.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            std::cerr << QString("エラー : 地震情報のファイルオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
            return -1;
        }

        // 新しい地震IDを先頭にして書き込む
        QTextStream out(&File);
        QString newContent = m_ID + "\n" + originalContent;
        out << newContent;

        // ファイルを閉じる
        File.close();
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 地震情報の書き込みに失敗 %1").arg(ex.what()).toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return -1;
    }

    return 0;
}


// コピーコンストラクタ
EarthQuakeInfo::EarthQuakeInfo(const EarthQuakeInfo &obj) noexcept
{
    // 共通のデータ
    this->m_Code        = obj.m_Code;
    this->m_ID          = obj.m_ID;
    this->m_Name        = obj.m_Name;
    this->m_Depth       = obj.m_Depth;
    this->m_Magnitude   = obj.m_Magnitude;
    this->m_Latitude    = obj.m_Latitude;
    this->m_Longitude   = obj.m_Longitude;

    // 発生した地震情報のデータ
    this->m_Time            = obj.m_Time;
    this->m_MaxScale        = obj.m_MaxScale;
    this->m_DomesticTsunami = obj.m_DomesticTsunami;
    this->m_ForeignTsunami  = obj.m_ForeignTsunami;
    this->m_Points          = obj.m_Points;
}


// 代入演算子のオーバーロード
EarthQuakeInfo& EarthQuakeInfo::operator=(const EarthQuakeInfo &obj)
{
    if (this != &obj) {
        // リソースをコピー

        // 発生した地震情報のデータ
        this->m_Code            = obj.m_Code;
        this->m_ID              = obj.m_ID;
        this->m_Name            = obj.m_Name;
        this->m_Depth           = obj.m_Depth;
        this->m_Magnitude       = obj.m_Magnitude;
        this->m_Latitude        = obj.m_Latitude;
        this->m_Longitude       = obj.m_Longitude;
        this->m_Time            = obj.m_Time;
        this->m_MaxScale        = obj.m_MaxScale;
        this->m_DomesticTsunami = obj.m_DomesticTsunami;
        this->m_ForeignTsunami  = obj.m_ForeignTsunami;
        this->m_Points          = obj.m_Points;
    }

    // このオブジェクトの参照を返す
    return *this;
}


// 発生した地震情報のログファイルに地震情報を追加する
int EarthQuakeInfo::addInfo(const QString &fileName, const QString &title, const QString &url, const QString &thread)
{
    // ミューテックスをロックして、スレッドセーフを保証
    QMutexLocker locker(&m_Mutex);

    // 地震情報の更新
    QFile File(fileName);
    if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << QString("エラー : 地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
        return -1;
    }

    try {
        // ログファイルの読み込み
        auto byaryJson = File.readAll();
        File.close();

        // qEQAlert.jsonファイルの設定を取得
        auto JsonDocument = QJsonDocument::fromJson(byaryJson);
        if (!JsonDocument.isArray()) {
            JsonDocument.array() = QJsonArray();
        }
        auto jsonArray = JsonDocument.array();

        // 新しいJSONオブジェクトを作成する
        QJsonObject newObject;
        newObject["id"]         = QJsonArray({m_ID});  // 発生した地震情報の地震ID
        newObject["hypocentre"] = m_Name;   // 発生した地震情報の震源地
        newObject["title"]      = title;    // 新規作成したスレッドのタイトル
        newObject["url"]        = url;      // 新規作成したスレッドのURL
        newObject["thread"]     = thread;   // 新規作成したスレッド番号
        newObject["date"]       = m_Time;   // 新規作成したスレッド番号

        // 新しいオブジェクトを配列に追加する
        jsonArray.append(newObject);

        // 更新されたJSON配列をファイルに書き込む
        JsonDocument.setArray(jsonArray);

        if (!File.open(QIODevice::WriteOnly | QIODevice::Text)) {
            std::cerr << QString("エラー : 地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
            return -1;
        }

        // ログファイルへ書き込み
        File.write(JsonDocument.toJson());

        // ログファイルを閉じる
        File.close();
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 地震情報の追加に失敗 %1").arg(ex.what()).toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return -1;
    }

    return 0;
}


// 発生した地震情報のログファイルに地震情報を更新する (発生日時の更新、地震IDの追加)
int EarthQuakeInfo::UpdateInfo(const QString &fileName)
{
    // ミューテックスをロックして、スレッドセーフを保証
    QMutexLocker locker(&m_Mutex);

    // 地震情報の更新
    QFile File(fileName);
    if (!File.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << QString("エラー : 地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
        return -1;
    }

    try {
        // ログファイルの読み込み
        auto byaryJson = File.readAll();
        File.close();

        // qEQAlert.jsonファイルの設定を取得
        auto JsonDocument = QJsonDocument::fromJson(byaryJson);
        if (JsonDocument.isNull()) {
            std::cerr << QString("エラー : 地震情報のログファイルに異常があります").toStdString() << std::endl;
            return -1;
        }

        auto jsonArray = JsonDocument.array();
        for (auto &&i : jsonArray) {
            QJsonObject obj = i.toObject();

            // 震源地("hypocentre"キーの値)が存在するかどうかを確認
            if (obj["hypocentre"].toString() == m_Name) {
                // 地震発生日時("date"キー)を更新
                obj["date"] = m_Time;

                // 地震ID("id"キー配列)を追加
                QJsonArray idArray = obj["id"].toArray();
                idArray.append(m_ID);
                obj["id"] = idArray;

                // 更新されたオブジェクトを配列に戻す
                i = obj;
            }
        }

        // 更新したJSONオブジェクトの配列をセット
        JsonDocument.setArray(jsonArray);

        if (!File.open(QIODevice::WriteOnly | QIODevice::Text)) {
            std::cerr << QString("エラー : 地震情報のログファイルのオープンに失敗 %1").arg(File.errorString()).toStdString() << std::endl;
            return -1;
        }

        // 更新したJSONオブジェクトの配列をログファイルに書き込む
        File.write(JsonDocument.toJson());

        // ログファイルを閉じる
        File.close();
    }
    catch (QException &ex) {
        std::cerr << QString("エラー : 地震情報の更新に失敗 %1").arg(ex.what()).toStdString() << std::endl;
        if (File.isOpen()) File.close();

        return -1;
    }

    return 0;
}


// 現在のエポックタイム (UNIX時刻) を秒単位で取得する
qint64 Worker::GetEpocTime()
{
    // 現在の日時を取得
    QDateTime now = QDateTime::currentDateTime();
    now.setTimeZone(QTimeZone("Asia/Tokyo"));

    // エポックタイム (UNIX時刻) を秒単位で取得
    qint64 epochTime = now.toSecsSinceEpoch();

    return epochTime;
}
