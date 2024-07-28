#include <QTextCodec>
#include <iostream>
#include "Poster.h"
#include "HtmlFetcher.h"


Poster::Poster(QObject *parent) : m_pManager(std::make_unique<QNetworkAccessManager>(this)), QObject{parent}
{

}


// 掲示板のクッキーを取得する
int Poster::fetchCookies(const QUrl &url)
{
    // レスポンス待機の設定
    QEventLoop loop;
    connect(m_pManager.get(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    // クッキーの取得
    QNetworkRequest request(url);
    auto pReply = m_pManager->get(request);

    // レスポンス待機
    loop.exec();

    return replyCookieFinished(pReply);
}


// GETデータ(クッキー)を確認する
int Poster::replyCookieFinished(QNetworkReply *reply)
{
    // レスポンスからクッキーを取得
    QList<QNetworkCookie> receivedCookies = QNetworkCookie::parseCookies(reply->rawHeader("Set-Cookie"));
    if (!receivedCookies.isEmpty()) {
        // クッキーを保存
        m_Cookies = receivedCookies;
    }
    else {
        // クッキーの取得に失敗した場合
        std::cerr << QString("エラー : クッキーの取得に失敗").toStdString() << std::endl;
        reply->deleteLater();

        return -1;
    }

    reply->deleteLater();

    return 0;
}


// 新規スレッドを作成する
int Poster::PostforCreateThread(const QUrl &url, THREAD_INFO &ThreadInfo)
{
    // リクエストの作成
    QNetworkRequest request(url);

    // POSTデータの生成 (<form>タグの<input>要素に基づいてデータを設定)
    // 新規スレッドを作成する場合は、<input>要素のname属性の値"key"を除去する必要がある
    QTextCodec *codec;                  // Shift-JIS用エンコードオブジェクト
    QByteArray encodedPostData = {};    // エンコードされたバイト列

    if (!ThreadInfo.shiftjis) {
        // UTF-8用 (QUrlQueryクラスはShift-JISに非対応)
        QUrlQuery query;

        query.addQueryItem("subject",   ThreadInfo.subject);    // スレッドタイトル名 (スレッドを立てる場合は入力する)
        query.addQueryItem("FROM",      ThreadInfo.from);
        query.addQueryItem("mail",      ThreadInfo.mail);       // メール欄
        query.addQueryItem("MESSAGE",   ThreadInfo.message);    // 書き込む内容
        query.addQueryItem("bbs",       ThreadInfo.bbs);        // BBS名
        query.addQueryItem("time",      ThreadInfo.time);       // エポックタイム (UNIXタイムまたはPOSIXタイム)

        encodedPostData = query.toString(QUrl::FullyEncoded).toUtf8();  // POSTデータをバイト列へ変換
    }
    else {
        // Shift-JIS用
        QString postMessage = QString("subject=%1&FROM=%2&mail=%3&MESSAGE=%4&bbs=%5&time=%6")
                                  .arg(ThreadInfo.subject,  // スレッドのタイトル (スレッドを立てる場合のみ入力)
                                       ThreadInfo.from,
                                       ThreadInfo.mail,     // メール欄
                                       ThreadInfo.message,  // 書き込む内容
                                       ThreadInfo.bbs,      // BBS名
                                       ThreadInfo.time      // エポックタイム (UNIXタイムまたはPOSIXタイム)
                                       );
        auto postData   = postMessage.toUtf8();             // POSTデータをバイト列へ変換

        // POSTデータの文字コードをShift-JISへエンコード
        codec           = QTextCodec::codecForName("Shift-JIS");
        encodedPostData = codec->fromUnicode(postData);
    }

    // ContentTypeHeaderをHTTPリクエストに設定
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(encodedPostData.length()));

    // クッキーをHTTPリクエストに設定
    QVariant var;
    var.setValue(m_Cookies);
    request.setHeader(QNetworkRequest::CookieHeader, var);

    // レスポンス待機の設定
    QEventLoop loop;
    connect(m_pManager.get(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    // HTTPリクエストの送信
    auto pReply = m_pManager->post(request, encodedPostData);

    // レスポンス待機
    loop.exec();

    // レスポンス情報の取得
    return replyPostFinished(pReply, url, ThreadInfo);
}


// 特定のスレッドに書き込む
int Poster::PostforWriteThread(const QUrl &url, THREAD_INFO &ThreadInfo)
{
    // リクエストの作成
    QNetworkRequest request(url);

    // POSTデータの生成 (<form>タグの<input>要素に基づいてデータを設定)
    // 既存のスレッドに書き込む場合は、<input>要素のname属性の値"key"にスレッド番号を指定する必要がある
    QTextCodec *codec;        // Shift-JIS用エンコードオブジェクト
    QByteArray encodedPostData = {};    // エンコードされたバイト列

    if (!ThreadInfo.shiftjis) {
        // UTF-8用 (QUrlQueryクラスはShift-JISに非対応)
        QUrlQuery query;

        query.addQueryItem("subject",   ThreadInfo.subject);    // スレッドタイトル名 (スレッドに書き込む場合は空欄にする)
        query.addQueryItem("FROM",      ThreadInfo.from);
        query.addQueryItem("mail",      ThreadInfo.mail);       // メール欄
        query.addQueryItem("MESSAGE",   ThreadInfo.message);    // 書き込む内容
        query.addQueryItem("bbs",       ThreadInfo.bbs);        // BBS名
        query.addQueryItem("time",      ThreadInfo.time);       // エポックタイム (UNIXタイムまたはPOSIXタイム)
        query.addQueryItem("key",       ThreadInfo.key);        // 書き込むスレッド番号 (スレッドに書き込む場合は入力する)

        encodedPostData = query.toString(QUrl::FullyEncoded).toUtf8();  // POSTデータをバイト列へ変換
    }
    else {
        // Shift-JIS用
        QString postMessage = QString("subject=%1&FROM=%2&mail=%3&MESSAGE=%4&bbs=%5&time=%6&key=%7")
                                  .arg(ThreadInfo.subject,  // スレッドのタイトル (スレッドに書き込む場合は空欄にする)
                                       ThreadInfo.from,
                                       ThreadInfo.mail,     // メール欄
                                       ThreadInfo.message,  // 書き込む内容
                                       ThreadInfo.bbs,      // BBS名
                                       ThreadInfo.time,     // エポックタイム (UNIXタイムまたはPOSIXタイム)
                                       ThreadInfo.key       // 書き込むスレッド番号 (スレッドに書き込む場合は入力する)
                                       );
        auto postData   = postMessage.toUtf8();             // POSTデータをバイト列へ変換

        // POSTデータの文字コードをShift-JISへエンコード
        codec           = QTextCodec::codecForName("Shift-JIS");
        encodedPostData = codec->fromUnicode(postData);
    }

    // ContentTypeHeaderをHTTPリクエストに設定
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(encodedPostData.length()));

    // クッキーをHTTPリクエストに設定
    QVariant var;
    var.setValue(m_Cookies);
    request.setHeader(QNetworkRequest::CookieHeader, var);

    // レスポンス待機の設定
    QEventLoop loop;
    connect(m_pManager.get(), &QNetworkAccessManager::finished, &loop, &QEventLoop::quit);

    // HTTPリクエストの送信
    auto pReply = m_pManager->post(request, encodedPostData);

    // レスポンス待機
    loop.exec();

    // レスポンス情報の取得
    return replyPostFinished(pReply, ThreadInfo);
}


// POSTデータ送信後のレスポンスを確認する (既存のスレッド用)
int Poster::replyPostFinished(QNetworkReply *reply, const THREAD_INFO &ThreadInfo)
{
    if (reply->error()) {
        std::cerr << QString("書き込みエラー : %1").arg(reply->errorString()).toStdString() << std::endl;
        reply->deleteLater();

        return -1;
    }
    else {
        QTextCodec *codec;
        QString replyData;

        if (ThreadInfo.shiftjis) {
            // Shift-JISからUTF-8へエンコード
            codec     = QTextCodec::codecForName("Shift-JIS");
            replyData = codec->toUnicode(reply->readAll());
        }
        else {
            replyData = reply->readAll();
        }

#ifdef _DEBUG
        std::cout << replyData.toStdString() << std::endl;
#endif

        // 書き込みした既存のスレッドのURLのパスを取得
        HtmlFetcher fetcher(nullptr);
        if (fetcher.extractThreadPath(replyData, ThreadInfo.bbs)) {
            std::cerr << QString("エラー : 書き込みした既存のスレッドのURLとスレッド番号の取得に失敗").toStdString() << std::endl;
            std::cerr << QString("スレッドの書き込みに失敗した可能性があります").toStdString() << std::endl;
            reply->deleteLater();

            return -1;
        }

        if (fetcher.GetThreadPath().isEmpty() || fetcher.GetThreadNum().isEmpty()) {
            std::cerr << QString("エラー : 書き込みしたスレッドのURLまたはスレッド番号がありません").toStdString() << std::endl;
            std::cerr << QString("スレッドの書き込みに失敗した可能性があります").toStdString() << std::endl;
            reply->deleteLater();

            return -1;
        }
    }

    reply->deleteLater();

    return 0;
}


// POSTデータ送信後のレスポンスを確認する (新規作成したスレッド用)
int Poster::replyPostFinished(QNetworkReply *reply, const QUrl &url, const THREAD_INFO &ThreadInfo)
{
    if (reply->error()) {
        std::cerr << QString("書き込みエラー : %1").arg(reply->errorString()).toStdString() << std::endl;
        reply->deleteLater();

        return -1;
    }
    else {
        QTextCodec *codec;
        QString replyData;

        if (ThreadInfo.shiftjis) {
            // Shift-JISからUTF-8へエンコード
            codec     = QTextCodec::codecForName("Shift-JIS");
            replyData = codec->toUnicode(reply->readAll());
        }
        else {
            replyData = reply->readAll();
        }

#ifdef _DEBUG
        std::cout << replyData.toStdString() << std::endl;
#endif

        // 新規作成したスレッドのURLのパスを取得
        HtmlFetcher fetcher(nullptr);
        if (fetcher.extractThreadPath(replyData, ThreadInfo.bbs)) {
            std::cerr << QString("エラー : 新規作成したスレッドのURLとスレッド番号の取得に失敗").toStdString() << std::endl;
            std::cerr << QString("スレッドの新規作成に失敗した可能性があります").toStdString() << std::endl;
            reply->deleteLater();

            return -1;
        }

        if (fetcher.GetThreadPath().isEmpty() || fetcher.GetThreadNum().isEmpty()) {
            std::cerr << QString("エラー : 新規作成したスレッドのURLまたはスレッド番号がありません").toStdString() << std::endl;
            std::cerr << QString("スレッドの新規作成に失敗した可能性があります").toStdString() << std::endl;
            reply->deleteLater();

            return -1;
        }

        // ベースURLを構築
        QUrl baseUrl = url.adjusted(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::RemoveFragment);

        // ベースURLとパスを繋げて新規作成したスレッドのURLを取得
        m_NewThreadURL = baseUrl.scheme() + "://" + baseUrl.host() + fetcher.GetThreadPath();

        // 新規作成したスレッド番号を取得
        m_NewThreadNum = fetcher.GetThreadNum();
    }

    reply->deleteLater();

    return 0;
}


// 新規作成したスレッドのパスを取得する
QString Poster::GetNewThreadURL() const
{
    return m_NewThreadURL;
}


// 新規作成したスレッドのスレッド番号を取得する
QString Poster::GetNewThreadNum() const
{
    return m_NewThreadNum;
}


// 文字列をShift-JISにエンコードする
[[maybe_unused]] QByteArray Poster::encodeStringToShiftJIS(const QString &str)
{
    QTextCodec *codec = QTextCodec::codecForName("Shift-JIS");
    return codec->fromUnicode(str);
}


// エンコードされたバイト列をURLエンコードする
[[maybe_unused]] QString Poster::urlEncode(const QByteArray &byteArray)
{
    return QString::fromUtf8(byteArray.toPercentEncoding());
}
