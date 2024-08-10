#ifndef EARTHQUAKE_H
#define EARTHQUAKE_H

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QObject>
#include <QException>
#include <memory>
#include "Poster.h"


// 緊急地震速報(警報)の対象地域
struct AREA {
    QString  KindCode;      // 警報コード
                            // 10 : 緊急地震速報（警報） 主要動について、まだ未到達と予測
                            // 11 : 緊急地震速報（警報） 主要動について、既に到達と予測
                            // 19 : 緊急地震速報（警報） 主要動の到達予想なし（PLUM法による予想)
    QString  Name;          // 地震が予想される地域
    QString  ArrivalTime;   // 予想される地震の時刻 (主要動の到達予測時刻)
                            // 形式 : yyyy/MM/dd hh:mm:ss
    int      ScaleFrom,     // 予想される最低震度
                            // システムの都合で小数点が付加されるが整数部のみ有効
                            // -1 : 不明
                            // 0  : 震度0
                            // 10 : 震度1
                            // 20 : 震度2
                            // 30 : 震度3
                            // 40 : 震度4
                            // 45 : 震度5弱
                            // 50 : 震度5強
                            // 55 : 震度6弱
                            // 60 : 震度6強
                            // 70 : 震度7
             ScaleTo;       // 予想される最大震度
                            // システムの都合で小数点が付加されるが整数部のみ有効
                            // -1 : 不明
                            // 0  : 震度0
                            // 10 : 震度1
                            // 20 : 震度2
                            // 30 : 震度3
                            // 40 : 震度4
                            // 45 : 震度5弱
                            // 50 : 震度5強
                            // 55 : 震度6弱
                            // 60 : 震度6強
                            // 70 : 震度7
                            // 99 : ～程度以上
};


// 緊急地震速報(警報)のクラス
class EarthQuakeAlert
{
public:     // Variables
    int         m_Code{};       // 地震の情報を表すコード
                                // 551 : 地震の情報
                                // 556 : 緊急地震速報(警報)の情報
    QString     m_ID,           // 地震情報のID
                m_Name,         // 震源地
                m_Depth,        // 震源の深さ
                                // 0の場合は、"ごく浅い"を表す
                                // システムの都合で小数点が付加されるが整数部のみ有効
                m_Magnitude,    // マグニチュード
                                // 震源情報が存在しない場合は、-1
                m_Latitude,     // 緯度 : 震源情報が存在しない場合は、-200または-200.0
                m_Longitude,    // 経度 : 震源情報が存在しない場合は、-200または-200.0
                m_OriginTime,   // 地震発生時刻
                m_ArrivalTime;  // 地震発現(到達)時刻
    QList<AREA> m_Areas;        // 緊急地震速報(警報)の対象地域
    QMutex      m_Mutex;        // 排他制御用オブジェクト

public:     // Methods
    explicit EarthQuakeAlert() = default;                       // デフォルトコンストラクタ
    EarthQuakeAlert(const EarthQuakeAlert &obj) noexcept;       // コピーコンストラクタ
    EarthQuakeAlert& operator=(const EarthQuakeAlert &obj);     // 代入演算子のオーバーロード
    int AddInfo(const QString &fileName);                       // 緊急地震速報(警報)のログファイルに地震IDを保存する
};


// 発生した地震情報の対地域
struct POINT {
    QString  Addr,      // 地震が発生した地域
             Pref;      // 地震が発生した都道府県
    int      Scale;     // その地域における震度
    bool     IsArea;    // 区域名かどうか
};


// 発生した地震情報のクラス
class EarthQuakeInfo
{
public:     // Variables
    int         m_Code{};           // 地震の情報を表すコード
                                    // 551 : 地震の情報
                                    // 556 : 緊急地震速報(警報)の情報
    QString     m_ID;               // 地震情報のID
    QString     m_Headline;         // 地震情報に関する速報テキスト (JMA専用)
    QString     m_Name;             // 震源地
    QString     m_Depth,            // 震源の深さ
                                    // 0の場合は、"ごく浅い"を表す
                                    // -1の場合は、情報が無いことを表す
                m_Magnitude,        // マグニチュード
                                    // 震源情報が存在しない場合は、-1
                m_Latitude,         // 緯度 : 震源情報が存在しない場合は、-200または-200.0
                m_Longitude,        // 経度 : 震源情報が存在しない場合は、-200または-200.0
                m_Time,             // 地震発生時刻
                m_MaxScale;         // 最大震度
                                    // -1 : 震度情報なし (震度情報が存在しない場合は-1)
                                    // 0  : 震度0
                                    // 10 : 震度1
                                    // 20 : 震度2
                                    // 30 : 震度3
                                    // 40 : 震度4
                                    // 45 : 震度5弱
                                    // 50 : 震度5強
                                    // 55 : 震度6弱
                                    // 60 : 震度6強
                                    // 70 : 震度7
    QString     m_DomesticTsunami,  // 国内への津波の有無
                                    // None         : なし
                                    // Unknown      : 不明
                                    // Checking     : 調査中
                                    // NonEffective : 若干の海面変動が予想されるが、被害の心配なし
                                    // Watch        : 津波注意報
                m_ForeignTsunami;   // 海外での津波の有無
                                    // None     : なし
                                    // Unknown  : 不明
                                    // Checking : 調査中
                                    // NonEffectiveNearby   : 震源の近傍で小さな津波の可能性があるが、被害の心配なし
                                    // WarningNearby        : 震源の近傍で津波の可能性がある
                                    // WarningPacific       : 太平洋で津波の可能性がある
                                    // WarningPacificWide   : 太平洋の広域で津波の可能性がある
                                    // WarningIndian        : インド洋で津波の可能性がある
                                    // WarningIndianWide    : インド洋の広域で津波の可能性がある
                                    // Potential            : 一般にこの規模では津波の可能性がある
    QList<POINT> m_Points;          // 発生した地震の地域
    QString      m_Text,            // 固定付加文
                 m_VarComment,      // その他付加文
                 m_FreeFormComment; // 自由付加文
    QString      m_ReportDateTime;  // JMAの地震情報の報告日時
    QStringList  m_MaxIntPrefs;     // 最も震度の大きい都道府県
    QMutex       m_Mutex;           // 排他制御用オブジェクト

public:     // Methods
    explicit EarthQuakeInfo() = default;                        // デフォルトコンストラクタ
    EarthQuakeInfo(const EarthQuakeInfo &obj) noexcept;         // コピーコンストラクタ
    EarthQuakeInfo& operator=(const EarthQuakeInfo &obj);       // 代入演算子のオーバーロード
    int  AddInfo(const QString &fileName, const QString &title, // 発生した地震情報のログファイルに地震情報に追加する
                 const QString &url, const QString &thread,
                 int GetInfo);
    int  UpdateInfo(const QString &fileName,                    // 発生した地震情報のログファイルに地震情報を更新する
                    bool bChangeTitle, const QString &title,    // !chttコマンドを使用する場合 : スレッドタイトル名、発生日時の更新、地震IDを更新
                    int GetInfo);                               // !chttコマンドを使用しない場合 : 発生日時、地震IDを更新
};


// 前方参照
class EarthQuake;


// 各ログファイルの情報と共用するための構造体
struct COMMONDATA {
    int             iGetInfo;       // 地震情報を取得するWebサイト
                                    // 0 : JMA (気象庁)
                                    // 1 : P2P地震情報
    int             AlertScale,     // 設定ファイルにある緊急地震速報(警報)の震度の閾値
                    InfoScale;      // 設定ファイルにある発生した地震情報の震度の閾値
    QString         EQInfoURL,      // JMAまたはP2P地震情報のURL
                    RequestURL,     // POSTデータを送信する掲示板のURL
                    LogFile;        // 取得した地震情報のIDを保存するログファイルのパス
    bool            bSubjectTime;   // 緊急地震地震速報で新規スレッドを作成する場合、スレッドタイトルに地震発現(到達)時刻を記載するかどうか
    bool            bChangeTitle;   // 発生した地震情報のスレッドのタイトルを変更するかどうか
    QString         ExpiredXPath,   // スレッドの生存を判断するときに使用するXPath
                                    // デフォルトは、"/html/head/title"タグを取得する
                    ThreadNumXPath; // スレッドの最後尾のレス番号を取得するXPath
    int             MaxThreadNum;   // スレッドの最大書き込み数
};


// 発生した地震情報のログファイル
struct INFOLOG {
    QStringList     IDs;            // 地震ID群
    QString         Hypocenter,     // 震源地
                    Title,          // 過去に作成したスレッドのタイトル
                    ThreadURL,      // 過去に作成したスレッドのURL
                    ThreadNum,      // 過去に作成したスレッド番号
                    Date;           // 震源地で起きた地震の最新日
    QStringList     MaxIntPrefs;    // 最も震度の大きい都道府県
};


// 地震情報とスレッド情報を管理するクラス
class Worker : public QObject
{
    Q_OBJECT

private:    // Variables
    std::unique_ptr<QNetworkAccessManager>  m_pEQManager;       // 緊急地震速報(警報)および発生した地震情報と通信するネットワークオブジェクト
    QByteArray                              m_ReplyData;        // 緊急地震速報(警報)のデータおよび発生した地震情報のデータを保存するオブジェクト
    COMMONDATA                              m_CommonData;       // 各ログファイルの情報と共用するための構造体
    EarthQuakeAlert                         m_Alert;            // 緊急地震速報(警報)のデータ
    EarthQuakeInfo                          m_Info;             // 発生した地震情報のデータ
    INFOLOG                                 m_InfoLog;          // 発生した地震情報のログファイル
    THREAD_INFO                             m_ThreadInfo;       // 新規スレッドを作成するための情報

public:     // Variables

private:    // Methods
    int         onEQDownloaded_for_JMA();                                       // JMAから発生した地震情報のURLを取得する
    int         DownloadContents(const QUrl &url);                              // JMAから取得した地震情報のデータを取得する
    int         onEQDownloaded_for_P2P();                                       // 緊急地震速報(警報)および発生した地震情報のデータを取得する
    int         FormattingData_for_JMA();                                       // JMAから取得した地震情報を整形する
    int         FormattingData_for_P2P();                                       // P2P地震情報から取得した地震情報を整形する
    int         FormattingThreadInfo();                                         // 整形した地震情報のデータをスレッド情報へ整形する
    int         Post(int EQCode, bool bCreateThread = true);                    // スレッドを新規作成する
    [[nodiscard]] bool  SearchAlertEQID(const QString &ID) const;               // 地震IDを保存しているログファイルから同じ地震IDが存在するかどうかを確認する
    [[nodiscard]] bool  SearchInfoEQID(const QString &ID) const;                // 地震情報のログファイルから同じ地震IDが存在するかどうかを確認する
    [[nodiscard]] bool  SearchInfoEQID(const QString &ID,                       // 地震情報のログファイルから同じ地震IDの"ReportDateTime"キーの日時が存在するかどうかを確認する
                                       const QString &reportDateTime) const;
    bool        GetExistObject(const QString &hypo);                            // 地震情報のログファイルから同じ震源地のオブジェクトを取得する
    bool        GetExistObject();                                               // 地震情報のログファイルから最も震度の大きい都道府県名のオブジェクトを取得する
    bool        isExistThread(const QUrl &url, const QString &title);           // 過去に作成したスレッドが生存しているかどうかを確認する
    int         CompareThreadTitle(const QUrl &url, const QString &title);      // !chttコマンドでスレッドのタイトルが正常に変更されているかどうかを判断する
                                                                                // !chttコマンドは、防弾嫌儲系のみ使用可能
    int         CheckLastThreadNum();                                           // 書き込むスレッドのレス数が上限に達しているかどうかを確認する
    int         DeleteObject(const QString &hypo) const;                        // ログファイルから生存していないスレッドの地震情報を削除する
    int         DeleteObject(const QStringList &prefs) const;                   // ログファイルから生存していないスレッドの地震情報を削除する
    static QString      AddChttCommand(QString &Subject, QString &Message);     // 既存スレッドに書き込む場合、スレッド本文の先頭に!chttコマンドを挿入する
                                                                                // 防弾嫌儲系の掲示板で使用可能
    static QString      ConvertScale(int Scale);                                // 震度の数値を特定の文字列に変換する
    static QString      ConvertMagnitude(double Magnitude);                     // マグニチュードの数値を特定の文字列に変換する
    static int          ConvertNumberToInt(const QVariant &value);              // 本来の値は整数値であるがシステムの都合で小数点が付加される場合があるため、
    static QString      ConvertNumberToString(const QVariant &value);           // 小数点以下を除去して整数の文字列に変換する
                                                                                // 緊急地震速報(警報)の"depth", "scaleFrom", "scaleTo"が対象
    static bool sortAreas(const AREA &a, const AREA &b);                        // 震度の大きさで降順ソートする (ただし、-1および99の場合は無視する)
    static bool sortPoints(const POINT &a, const POINT &b);                     // 震度の大きさで降順ソートする (ただし、-1の場合は無視する)
    [[nodiscard]] static qint64      GetEpocTime();                             // 現在のエポックタイム (UNIX時刻) を秒単位で取得する
    QStringList GetMaxIntPrefs();                                               // 最も震度の大きい都道府県を取得する
    QString     ConvertDateTimeFormat(const QString &inputDateTime);            // ISO 8601形式の時刻を"yyyy年M月d日 h時m分"形式に変換する
    template <typename T>
    T           ConvertJMAScale(const QString &strScale);                       // JMAから取得した震度をP2P地震情報の震度の形式に変換する

public:     // Methods
    explicit Worker(QObject *parent = nullptr);
    Worker(COMMONDATA CommonData, THREAD_INFO threadInfo, QObject *parent = nullptr);

signals:

public slots:
    int         ProcessEQAlert();           // 取得したデータを整形およびスレッド情報へ変換後、新規スレッドを作成する (緊急地震速報用)
    int         ProcessEQInfo();            // 取得したデータを整形およびスレッド情報へ変換後、
                                            // 既存スレッドに書き込み、または、新規スレッドを作成する (発生した地震情報用)
};


// 地震に関する情報を管理するクラス
class EarthQuake : public QObject
{
    Q_OBJECT

private:    // Variables
    int                                     m_iGetInfo;         // 地震情報を取得するWebサイト
                                                                // 0 : JMA (気象庁)
                                                                // 1 : P2P地震情報
    QString                                 m_EQAlertURL,       // 緊急地震速報(警報)を取得するURL
                                            m_EQInfoURL;        // 発生した地震情報を取得するURL
    bool                                    m_bEQAlert;         // 緊急地震速報(警報)の有効 / 無効
    bool                                    m_bEQInfo;          // 地震情報の有効 / 無効
    int                                     m_AlertScale,       // ユーザが設定した緊急地震速報(警報)の震度の閾値
                                            m_InfoScale;        // ユーザが設定した発生した地震情報の震度の閾値
    QString                                 m_AlertFile,        // 緊急地震速報(警報)のIDを保存するファイルパス
                                            m_InfoFile,         // 発生した地震情報のIDを保存するファイルパス
                                            m_RequestURL;       // スレッドを立てるためのPOSTデータを送信するURL
    bool                                    m_bSubjectTime;     // 緊急地震地震速報で新規スレッドを作成する場合、スレッドタイトルに地震発現(到達)時刻を記載するかどうか
    bool                                    m_EQchangeTitle;    // 発生した地震情報のスレッドのタイトルを変更するかどうか
    THREAD_INFO                             m_ThreadInfo;       // スレッドを立てるための情報オブジェクト
    QString                                 m_ExpiredXPath;     // スレッドの生存を判断するときに使用するXPath
                                                                // デフォルトは、"/html/head/title"タグを取得する
    std::unique_ptr<Worker>                 m_pEQAlertWorker;   // 緊急地震速報(警報)オブジェクト
    std::unique_ptr<Worker>                 m_pEQInfoWorker;    // 発生した地震情報オブジェクト

public:     // Variables

private:    // Methods

public:     // Methods
    [[maybe_unused]] explicit EarthQuake(QObject *parent = nullptr);
    explicit EarthQuake(int iGetInfo, QString EQAlertURL, QString EQInfoURL,
                        bool bEQAlert, bool bEQInfo, int AlertScale, int InfoScale, QString AlertFile, QString InfoFile,
                        QString RequestURL, bool bSubjectTime, bool bEQchangeTitle, THREAD_INFO ThreadInfo, QString ExpiredXPath,
                        QObject *parent = nullptr);
    ~EarthQuake() override;
    int     EQProcess();    // 緊急地震速報(警報)および発生した地震情報を取得して新規スレッドを作成する

signals:

public slots:

};

#endif // EARTHQUAKE_H
