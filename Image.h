#ifndef IMAGE_H
#define IMAGE_H

#include <QObject>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>
#include <memory>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>


// 地震情報の震度画像を取得するために必要な情報
struct EQIMAGEINFO {
    bool            bEnable;        // 震度画像を取得するかどうか
    QString         BaseUrl;        // 地震分布の画像を取得するための起点となるURL
    QUrl            Url;            // 地震情報一覧から該当する地震情報を取得するためのURL
    QString         DateStr;        // 該当する地震情報の震度画像を取得するための日時
    QString         DateFormat;     // 該当する地震情報の震度画像を取得するための日時形式
    QString         ListXPath,      // 該当する地震情報のURLを取得するためのXPath式 (テーブル)
                    DetailXPath,    // 該当する地震情報のURLを取得するためのXPath式 (テーブル内の要素)
                    UrlXPath;       // 該当する地震情報のURLを取得するためのXPath式 (テーブル内のaタグのhref要素)
    QString         ImgXPath;       // 該当する地震情報の震度画像を取得するためのXPath式
};


class Image : public QObject
{
    Q_OBJECT

private:    // Variables
    std::unique_ptr<QNetworkAccessManager>  m_pManager;                             // URLにアクセスするネットワークオブジェクト
    EQIMAGEINFO m_EQImageInfo;
    QString     m_Url,
                m_ImageUrl;

public:     // Variables


private:    // Methods
    void              CleanupXPathObject(xmlXPathObjectPtr result);
    void              CleanupXPathContext(xmlXPathContextPtr context);

public:     // Methods
    explicit Image(EQIMAGEINFO &EQImageInfo, QObject *parent = nullptr);
    virtual ~Image();

    int     FetchUrl(bool redirect, bool bShiftJIS = false);
    QString GetUrl() const;
    int     FetchImageUrl(const QUrl &url, bool redirect, bool bShiftJIS = false);
    QString GetImageUrl() const;

signals:
};

#endif // IMAGE_H
