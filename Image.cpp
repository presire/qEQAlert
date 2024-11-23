#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    #include <QStringEncoder>
#else
    #include <QTextCodec>
#endif

#include <iostream>
#include "Image.h"


Image::Image(EQIMAGEINFO &EQImageInfo, QObject *parent) :
    m_EQImageInfo(EQImageInfo),                                 // 地震情報の震度画像を取得するためのオブジェクトを初期化
    m_pManager(std::make_unique<QNetworkAccessManager>(this)),  // URLにアクセスするネットワークオブジェクトを初期化
    QObject{parent}
{
}


Image::~Image()
{
}


// XPathコンテキストを解放するヘルパ関数
void Image::CleanupXPathContext(xmlXPathContextPtr context)
{
    if (context) {
        xmlXPathFreeContext(context);
    }
}


// XPath評価結果を解放するヘルパ関数
void Image::CleanupXPathObject(xmlXPathObjectPtr result)
{
    if (result) {
        xmlXPathFreeObject(result);
    }
}


// 該当する地震情報の震度画像が存在するURLを取得する
int Image::FetchUrl(bool redirect, bool bShiftJIS)
{
    // リダイレクトを自動的にフォロー
    QNetworkRequest request(m_EQImageInfo.Url);

    // タイムアウトを3[秒]に設定
    request.setTransferTimeout(3000);

    // リダイレクトの設定
    if (redirect) {
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    }

    // Webページの取得
    auto pReply = m_pManager->get(request);

    // レスポンス待機
    QEventLoop loop;
    QObject::connect(pReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // ダウンロードしたHTMLの内容から特定の属性の値を取得する
    if (pReply->error() != QNetworkReply::NoError) {
        // ステータスコードの確認
        int statusCode = pReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 404) {
            // 該当スレッドが存在しない(落ちている)場合
            pReply->deleteLater();
            return 1;
        }

        std::cerr << "エラー : " << pReply->errorString().toStdString();
        pReply->deleteLater();

        return -1;
    }

    QString htmlContent;
    if (bShiftJIS) {
        // Shift-JISからUTF-8へデコード
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QByteArray SJISData = pReply->readAll();

        QStringDecoder decoder("Shift-JIS");
        htmlContent         = decoder.decode(SJISData);
#else
        auto codec  = QTextCodec::codecForName("Shift-JIS");
        htmlContent = codec->toUnicode(pReply->readAll());
#endif

    }
    else {
        htmlContent = pReply->readAll();
    }

    // libxml2の初期化
    xmlInitParser();

    // 文字列からHTMLドキュメントをパース
    // libxml2ではエンコーディングの自動判定において問題があるため、エンコーディングを明示的に指定する
    xmlDocPtr doc = htmlReadDoc((const xmlChar*)htmlContent.toStdString().c_str(), nullptr, "UTF-8", HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == nullptr) {
        std::cerr << QString("エラー : HTMLのパースに失敗しました").toStdString() << std::endl;
        xmlCleanupParser();
        return -1;
    }

    // XPathコンテキストの作成
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    if (context == nullptr) {
        std::cerr << QString("エラー : XPathコンテキストの生成に失敗しました").toStdString() << std::endl;
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return -1;
    }

    // 各地震情報のリストを取得するXPath式
    xmlXPathObjectPtr result = xmlXPathEvalExpression((xmlChar*)m_EQImageInfo.ListXPath.toStdString().data(), context);
    if (result == nullptr) {
        std::cerr << QString("エラー : XPath式の評価に失敗しました").toStdString() << std::endl;
        CleanupXPathContext(context);
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return -1;
    }

    if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
        std::cerr << QString("エラー : 一致するノードが見つかりません").toStdString() << std::endl;
        CleanupXPathObject(result);
        CleanupXPathContext(context);
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return -1;
    }

    // 元の日時 ("yyyy/MM/dd HH:mm:ss"形式) を 地震情報を取得するための形式 に変換
    // 変数formattedDateの例 : "2024年8月9日 19時57分ごろ"
    QDateTime  date          = QDateTime::fromString(m_EQImageInfo.DateStr, "yyyy/MM/dd HH:mm:ss");
    QString    formattedDate = date.toString(m_EQImageInfo.DateFormat);
    QByteArray dateUtf8      = formattedDate.toUtf8();

    // 取得した各地震情報のリストに対して処理を行う
    for (int i = 0; i < result->nodesetval->nodeNr; i++) {
        xmlNodePtr trNode = result->nodesetval->nodeTab[i];

        // 各地震情報の詳細 (tdタグ) を取得
        xmlXPathObjectPtr tdResult = xmlXPathNodeEval(trNode, (xmlChar*)m_EQImageInfo.DetailXPath.toUtf8().constData(), context);
        if (tdResult && !xmlXPathNodeSetIsEmpty(tdResult->nodesetval)) {
            for (int j = 0; j < tdResult->nodesetval->nodeNr; j++) {
                xmlNodePtr tdNode = tdResult->nodesetval->nodeTab[j];
                xmlChar *content  = xmlNodeGetContent(tdNode);

                if (xmlStrEqual(content, (xmlChar*)dateUtf8.constData())) {
                    // 震度に関する画像が存在するURLを取得
                    xmlXPathObjectPtr aResult = xmlXPathNodeEval(tdNode, (xmlChar*)m_EQImageInfo.UrlXPath.toUtf8().constData(), context);
                    if (aResult && !xmlXPathNodeSetIsEmpty(aResult->nodesetval)) {
                        xmlChar *href = xmlNodeGetContent(aResult->nodesetval->nodeTab[0]);
                        m_Url         = QString::fromUtf8(reinterpret_cast<const char*>(href));

#ifdef _DEBUG
                        std::cout << QString("一致する日時の地震情報のURLが見つかりました : %1").arg(m_Url).toStdString() << std::endl;
#endif

                        // クリーンアップ
                        xmlFree(href);
                        CleanupXPathObject(aResult);

                        xmlFree(content);
                        CleanupXPathObject(tdResult);

                        CleanupXPathObject(result);
                        CleanupXPathContext(context);
                        xmlFreeDoc(doc);
                        xmlCleanupParser();

                        return 0;
                    }
                    CleanupXPathObject(aResult);
                }

#ifdef _DEBUG
                std::cout << QString("各地震情報の詳細 : %1")
                             .arg(QString::fromUtf8(reinterpret_cast<const char*>(content))).toStdString()
                          << std::endl;
#endif

                xmlFree(content);
            }
        }
        CleanupXPathObject(tdResult);
    }

    // クリーンアップ
    CleanupXPathObject(result);
    CleanupXPathContext(context);
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return -1;
}


// 該当する地震情報の震度画像が存在するURLを返す
QString Image::GetUrl() const
{
    return m_Url;
}


// 該当する地震情報の震度画像のURLを取得する
int Image::FetchImageUrl(const QUrl &url, bool redirect, bool bShiftJIS)
{
    // リダイレクトを自動的にフォロー
    QNetworkRequest request(url);

    // タイムアウトを3[秒]に設定
    request.setTransferTimeout(3000);

    // リダイレクトの設定
    if (redirect) {
        request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);
    }

    // Webページの取得
    auto pReply = m_pManager->get(request);

    // レスポンス待機
    QEventLoop loop;
    QObject::connect(pReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (pReply->error() != QNetworkReply::NoError) {
        // 該当する震度画像があるWebページの取得に失敗した場合
        std::cerr << QString("エラー : %1").arg(pReply->errorString()).toStdString() << std::endl;
        pReply->deleteLater();

        return -1;
    }

    QString htmlContent;
    if (bShiftJIS) {
        // Shift-JISからUTF-8へデコード
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QByteArray SJISData = pReply->readAll();

        QStringDecoder decoder("Shift-JIS");
        htmlContent         = decoder.decode(SJISData);
#else
        auto codec  = QTextCodec::codecForName("Shift-JIS");
        htmlContent = codec->toUnicode(pReply->readAll());
#endif
    }
    else {
        htmlContent = pReply->readAll();
    }

    pReply->deleteLater();

    // libxml2の初期化
    xmlInitParser();

    // 文字列からHTMLドキュメントをパース
    // libxml2ではエンコーディングの自動判定において問題があるため、エンコーディングを明示的に指定する
    xmlDocPtr doc = htmlReadDoc((const xmlChar*)htmlContent.toStdString().c_str(),
                                nullptr,
                                "UTF-8",
                                HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == nullptr) {
        std::cerr << QString("エラー: 震度画像が存在するURLのHTMLのパースに失敗しました").toStdString() << std::endl;
        return -1;
    }

    // XPathで特定の要素を検索
    xmlXPathContextPtr context = xmlXPathNewContext(doc);
    xmlXPathObjectPtr  xpathObj = xmlXPathEvalExpression(reinterpret_cast<const xmlChar*>(m_EQImageInfo.ImgXPath.toUtf8().constData()), context);

    QString content;
    if (xpathObj && xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0) {
        xmlNodePtr node = xpathObj->nodesetval->nodeTab[0];
        if (node->type == XML_ATTRIBUTE_NODE) {
            content = QString(reinterpret_cast<const char*>(node->children->content));
        }
    }
    else {
        std::cerr << QString("エラー: 震度画像の取得に失敗しました").toStdString() << std::endl;
        xmlXPathFreeObject(xpathObj);
        xmlXPathFreeContext(context);
        xmlFreeDoc(doc);

        return -1;
    }

    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(context);
    xmlFreeDoc(doc);

    if (content.isEmpty()) {
        std::cerr << QString("エラー: 震度画像が存在しません").toStdString() << std::endl;
        return -1;
    }

    // 震度画像のURLを取得
    m_ImageUrl = content;

    return 0;
}


// 該当する地震情報の震度画像のURLを返す
QString Image::GetImageUrl() const
{
    return m_ImageUrl;
}
