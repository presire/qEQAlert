#ifndef HTMLFETCHER_H
#define HTMLFETCHER_H

#include <QObject>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <libxml/HTMLparser.h>
#include <libxml/xpath.h>
#include <memory>


class HtmlFetcher : public QObject
{
    Q_OBJECT

private:  // Variables
    std::unique_ptr<QNetworkAccessManager>  m_pManager;                             // スレッドのURLにアクセスするネットワークオブジェクト
    QString                                 m_ThreadPath,                           // スレッドのパス
                                            m_ThreadNum;                            // スレッド番号
    QString                                 m_Element;                              // XPathを使用して取得するエレメント

private:  // Methods
    int fetchElement(QNetworkReply *reply, const QString &_xpath,                   // Webページにアクセスして、特定の属性を取得する
                     bool bShiftJIS = false);
    xmlXPathObjectPtr getNodeset(xmlDocPtr doc, const xmlChar *xpath);              // ダウンロードしたHTMLの内容から特定の属性の値を取得する

public:   // Methods
    explicit HtmlFetcher(QObject *parent = nullptr);
    ~HtmlFetcher() override;
    int     fetch(const QUrl &url, bool redirect, const QString &_xpath,            // Webページにアクセスして、特定の属性を取得する
                  bool bShiftJIS = false);
    int     extractThreadPath(const QString &htmlContent, const QString &bbs);      // 新規作成したスレッドからスレッドのパスおよびスレッド番号を抽出する
    int     fetchLastThreadNum(const QUrl &url, bool redirect,                      // 書き込むスレッドの最後尾のレス番号を取得する
                               const QString &_xpath, int elementType);

    [[nodiscard]] QString GetThreadPath() const;                                    // スレッドのパスを取得する
    [[nodiscard]] QString GetThreadNum() const;                                     // スレッド番号を取得する
    [[nodiscard]] QString GetElement() const;                                       // 要素を取得する

signals:

public slots:

};

#endif // HTMLFETCHER_H
