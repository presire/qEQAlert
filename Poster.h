#ifndef POSTER_H
#define POSTER_H

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QUrl>
#include <QUrlQuery>
#include <QObject>
#include <memory>
#include <utility>


// スレッド情報
struct THREAD_INFO
{
    THREAD_INFO() = default;

    [[maybe_unused]] THREAD_INFO(QString _subject, QString _from, QString _mail, QString _message,
                                 QString _bbs, QString _time, QString _key) :
                                 subject(std::move(_subject)), from(std::move(_from)), mail(std::move(_mail)), bbs(std::move(_bbs)),
                                 message(std::move(_message)), time(std::move(_time)), key(std::move(_key)) {}
    QString subject     = "";   // スレッドタイトル名 (スレッドを立てる場合のみ入力)
    QString from        = "";
    QString mail        = "";   // メール欄
    QString message     = "";   // 書き込む内容
    QString bbs         = "";   // BBS名
    QString time        = "";   // エポックタイム (UNIXタイムまたはPOSIXタイムとも呼ばれる)
    QString key         = "";   // スレッド番号 (スレッドに書き込む場合のみ入力)
    bool    shiftjis    = true;
};


class Poster : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QNetworkAccessManager> m_pManager;      // 掲示板にアクセスするためのネットワークオブジェクト
    QList<QNetworkCookie>                  m_Cookies;       // 掲示板のクッキー
    QUrl                                   m_URL;           // 書き込み用URL
    QString                                m_NewThreadURL,  // 新規作成したスレッドのURL
                                           m_NewThreadNum;  // 新規作成したスレッド番号

private:
    int         replyCookieFinished(QNetworkReply *reply);                      // GETデータ(クッキー)を確認する
    int         replyPostFinished(QNetworkReply *reply,                         // POSTデータ送信後のレスポンスを確認する
                                  const THREAD_INFO &ThreadInfo);
    int         replyPostFinished(QNetworkReply *reply, const QUrl &url,        // POSTデータ送信後のレスポンスを確認する
                                  const THREAD_INFO &ThreadInfo);

    [[maybe_unused]] static QByteArray  encodeStringToShiftJIS(const QString &str);    // 文字列をShift-JISにエンコードする
    [[maybe_unused]] static QString     urlEncode(const QByteArray& byteArray);        // エンコードされたバイト列をURLエンコードする

public:
    explicit    Poster(QObject *parent = nullptr);
    ~Poster() override = default;
    int         fetchCookies(const QUrl &url);                                  // 掲示板のクッキーを取得する
    int         PostforWriteThread(const QUrl &url, THREAD_INFO &threadInfo);   // 特定のスレッドに書き込む
    int         PostforCreateThread(const QUrl &url, THREAD_INFO &threadInfo);  // スレッドを新規作成する
    [[nodiscard]] QString     GetNewThreadURL() const;                          // 新規作成したスレッドのURLを取得する
    [[nodiscard]] QString     GetNewThreadNum() const;                          // 新規作成したスレッド番号を取得する

signals:

};

#endif // POSTER_H
