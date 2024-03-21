#ifndef RUNNER_H
#define RUNNER_H

#include <QCoreApplication>

#ifdef Q_OS_LINUX
    #include <QSocketNotifier>
#elif Q_OS_WIN
    #include <QWinEventNotifier>
    #include <windows.h>
#endif

#include <QObject>
#include <QTimer>
#include <memory>
#include "EarthQuake.h"


class Runner : public QObject
{
    Q_OBJECT

private:  // Variables
    // 共通
    QStringList                             m_args;         // コマンドラインオプション
    QString                                 m_SysConfFile;  // このソフトウェアの設定ファイルのパス
    bool                                    m_bOneShot;     // ワンショット機能の有効 / 無効
                                                            // メンバ変数m_EQIntervalの値を使用して自動的に地震情報を取得するかどうか
    QString                                 m_RequestURL;   // 地震情報を書き込むためのPOSTデータを送信するURL
    THREAD_INFO                             m_ThreadInfo;   // 地震情報を書き込むスレッドの情報

    // 地震の情報
    bool                                    m_bEQAlert,         // 緊急地震速報(警報)の有効 / 無効
                                            m_bEQInfo;          // 発生した地震情報の有効 / 無効
    int                                     m_AlertScale,       // 緊急地震速報(警報)における震度の閾値 (この震度以上の場合は新規スレッドを作成する)
                                            m_InfoScale,        // 発生した地震情報における震度の閾値 (この震度以上の場合は新規スレッドを作成または既存のスレッドに書き込む)
                                            m_EQInterval;       // 地震の情報を取得する時間間隔 (デフォルト : 10[秒]〜)
    QString                                 m_AlertFile,        // 緊急地震速報(警報)のIDを保存するファイルパス
                                            m_InfoFile;         // 発生した地震情報のIDを保存するファイルパス
    bool                                    m_EQsubTime;        // 緊急地震地震速報で新規スレッドを作成する場合、スレッドタイトルに地震発現(到達)時刻を記載するかどうか
    bool                                    m_EQchangeTitle;    // 発生した地震情報のスレッドのタイトルを変更するかどうか
                                                                // 防弾嫌儲およびニュース速報(Libre)等のスレッドのタイトルが変更できる掲示板で使用可能
    QTimer                                  m_EQTimer;          // 地震の情報を取得するためのインターバル時間をトリガとするタイマ
    std::unique_ptr<EarthQuake>             m_pEarthQuake;      // 地震情報クラスを管理するオブジェクト
    std::atomic<bool>                       m_stopRequested;    // [q]キーまたは[Q]キーを押下した場合のフラグ

#ifdef Q_OS_LINUX
    std::unique_ptr<QSocketNotifier>        m_pNotifier;    // このソフトウェアを終了するためのキーボードシーケンスオブジェクト
#elif Q_OS_WIN
    std::unique_ptr<QWinEventNotifier>      m_pNotifier;    // このソフトウェアを終了するためのキーボードシーケンスオブジェクト
#endif

public:     // Variables

private:    // Methods
    int         getConfiguration(QString &filepath);        // このソフトウェアの設定ファイルの情報を取得

public:  // Methods
    explicit    Runner(QStringList args, QObject *parent = nullptr);
    ~Runner() override = default;

signals:

public slots:
    void run();             // このソフトウェアを最初に実行する時にのみ実行するメイン処理
    void fetch();           // 地震情報を取得するスロット
    void onReadyRead();     // ノンブロッキングでキー入力を受信するスロット
};

#endif // RUNNER_H
