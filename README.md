# qEQAlert
<br>

URL : https://github.com/presire/qEQAlert  
<br>

# はじめに  
qEQAlertは、気象庁 (JMA) あるいはP2P地震情報からデータを取得して、0ch系の掲示板に書き込むソフトウェアです。  

緊急地震速報(警報)では、常に新規スレッドを自動作成します。  
これは、緊急地震速報(警報)は緊急性が必要であり、既存のスレッドに書き込むと、  
該当スレッドに多くのレスがある場合はスクロールする手間が掛かるからです。  
<br>

発生した地震情報の場合は、震源地が同じ既存のスレッドが存在するときは該当スレッドに自動書き込み、  
該当スレッドが無ければ新規スレッドを自動作成します。  
<br>

取得する地震情報は、以下の通りです。  
* 緊急地震速報(警報) :  
  30[秒]以内の最新情報 (1件のみ)  
  <br>
* 発生した地震情報 :  
  10[分]以内の最新情報 (1件のみ)  
<br>

地震情報の詳細は、[P2P地震情報](https://www.p2pquake.net/secondary_use/) をご確認ください。  

P2P地震情報では、以下のレート制限があります。  
それを超えるとレスポンスが遅くなったり拒否 (HTTP ステータスコード 429) される場合があります。  
<code>60 リクエスト/分 (IPアドレスごと)</code>  

<br>

**このソフトウェアを動作させるには、Qt 5.15 (Core、Network) および libxml 2.0が必要となります。**  
**Qt 6を使用してビルドおよび動作させることができる可能性もありますが、確認はしておりませんのでご注意ください。**  
<br>

README.mdでは、Red Hat Enterprise LinuxおよびSUSE Linux Enterprise / openSUSEを前提に記載しております。  
また、他のLinuxディストリビューションにもインストールして使用できると思います。  
(例: Linux Mint, Manjaro, MX Linux, ... 等)  

Raspberry Pi上での動作は確認済みです。  
<br>

**注意：**  
**ただし、本ソフトウェアはリアルタイム性を保証できないため、緊急地震速報(警報)の機能の使用は非推奨としております。**  

**version 0.2.0以降の設定ファイル (JSONファイル) は、version 0.1.2以前の設定ファイルと構造が異なります。**  
**そのため、以前のバージョンの設定ファイルをversion 0.2.0以降で使い回すことが出来ないことに注意してください。**  

**また、ご要望があれば、逐次開発を進めていく予定です。**  
<br>
<br>

# 1. ビルドに必要なライブラリをインストール  
<br>

* Qt5 Core
* Qt5 Network
* Qt5 Xml
  * <https://www.qt.io/>  
  * qEQAlertは、Qtライブラリを使用しています。
  * 本ソフトウェアで使用しているQtライブラリは、LGPL v3オープンソースライセンスの下で利用可能です。  
  * Qtのライセンスファイルは、以下に示すファイルで確認できます。  
    <I>**LibraryLicenses/Qt/LICENSE.LGPLv3**</I>  
<br>

* libxml 2.0
  * <https://gitlab.gnome.org/GNOME/libxml2>  
  * qEQAlertは、libxml 2.0を使用しています。  
  * libxml 2.0は、MITライセンスの下で利用可能です。  
  * libxml 2.0のライセンスファイルは、以下に示すファイルで確認できます。  
    <I>**LibraryLicenses/libxml2/LICENSE.MIT**</I>  
<br>


システムをアップデートした後、続いて本ソフトウェアのビルドに必要なライブラリをインストールします。  

    # Red Hat Enterprise Linux
    sudo dnf update   
    sudo dnf install make cmake gcc gcc-c++ \  
                     libxml2 libxml2-devel qt5-qtbase-devel  

    # SUSE Linux Enterprise / openSUSE
    sudo zypper update  
    sudo zypper install make cmake gcc gcc-c++ libxml2-devel        \  
                        libqt5-qtbase-common-devel libQt5Core-devel \  
                        libQt5Network-devel libQt5Xml-devel  

    # Debian GNU/Linux, Raspberry Pi OS  
    sudo apt update && sudo apt upgrade  
    sudo apt install make cmake gcc libxml2 libxml2-dev \  
                     qtbase5-dev libqt5xmlpatterns5-dev  
<br>
<br>

# 2. ビルドおよびインストール
## 2.1. qEQAlertのビルドおよびインストール
GitHubからqEQAlertのソースコードをダウンロードします。  

    git clone https://github.com/presire/qEQAlert.git qEQAlert  

    cd qEQAlert  

    mkdir build && cd build  
<br>

qEQAlertをソースコードからビルドするには、<code>cmake</code>コマンドを使用します。  
Ninjaビルドを使用する場合は、<code>cmake</code>コマンドに<code>-G Ninja</code>オプションを付加します。  
<br>

ビルド時に使用できるオプションを以下に示します。  
<br>

* <code>CMAKE_BUILD_TYPE</code>  
  デフォルト値 : <code>Release</code>  
  リリースビルドまたはデバッグビルドを指定します。  
  <br>
* <code>CMAKE_INSTALL_PREFIX</code>  
  デフォルト値 : <code>/usr/local</code>  
  qEQAlertのインストールディレクトリを変更することができます。  
  <br>
* <code>SYSCONF_DIR</code>  
  デフォルト値 : <code>/etc/qEQAlert/qEQAlert.json</code>  
  設定ファイル qEQAlert.jsonのインストールディレクトリを変更することができます。  
  <br>
* <code>SYSTEMD</code>  
  デフォルト値 : <code>OFF</code>  (無効 : Systemdサービスファイルはインストールされない)  
  <br>
  指定できる値 1 : <code>system</code>を指定する場合 - <code>/etc/systemd/system</code>  
  指定できる値 2 : <code>user</code>を指定する場合 - <code>~/.config/systemd/user</code>  
  <br>
  Systemdサービスファイル qEQAlert.serviceのインストールディレクトリを変更することができます。  
  <br>
* <code>PID</code>  
  デフォルト値 : <code>/var/run</code>  
  Systemdサービスを使用する場合、プロセスファイル qEQAlert.pidのパスを変更することができます。  
  Systemdサービスを使用しない場合は不要です。  
  <br>
* <code>WITH_LIBXML2</code>  
  デフォルト値 : 空欄  
  使用例 : <code>-DWITH_LIBXML2=/opt/libxml2/lib64/pkgconfig</code>  
  ( libxml 2.0ライブラリが、/opt/libxml2ディレクトリにインストールされている場合 )  
  <br>
  libxml 2.0ライブラリのpkgconfigディレクトリのパスを指定することにより、  
  任意のディレクトリにインストールされているlibxml 2.0ライブラリを使用して、このソフトウェアをコンパイルすることができます。  
  通常、あまり使用しないと思われます。  

<br>

    cmake -DCMAKE_BUILD_TYPE=Release \  
          -DCMAKE_INSTALL_PREFIX=<qEQAlertのインストールディレクトリ> \  
          -DSYSCONF_DIR=<設定ファイルのインストールディレクトリ>          \  
          -DSYSTEMD=user  \  # Systemdサービスファイルをホームディレクトリにインストールする場合  
          -DPID=/tmp      \  # Systemdサービスを使用する場合
          ..  
    
    make -j $(nproc)  
    
    make install  または  sudo make install  
<br>

## 2.2. Systemdサービスの使用

**<u>ワンショット機能を有効 (<code>oneshot</code>を<code>true</code>) にしている場合は、Systemdサービスと連携できないことにご注意ください。</u>**  
<br>

qEQAlertのSystemdサービスファイルは、  
/etc/systemd/systemディレクトリ、または、~/.config/systemd/userディレクトリのいずれかにインストールされております。  
<br>

<code>cmake</code>コマンドにおいて、<code>SYSTEMD</code>オプションを変更することによりインストールディレクトリが変更されます。  
<br>

Systemdサービスを再読み込みします。  

    sudo systemctl daemon-reload  
    または  
    systemctl --user daemon-reload  
<br>

qeqalertデーモンは遅延実行を行う必要があるため、**タイマサービスを有効化または実行する必要**があります。  
qEQAlertを実行する時は、タイマデーモンを開始します。  

    sudo systemctl start qeqalert.timer  
    または
    systemctl --user start qeqalert.timer  
<br>

PCの起動時またはユーザのログイン時において、qeqalertデーモンを自動起動する場合は、タイマデーモンを有効にします。  

    sudo systemctl enable qeqalert.timer  
    または  
    systemctl --user enable qeqalert.timer  
<br>

PCの起動時またはユーザのログイン時において、qeqalertデーモンの自動起動を無効にする場合は、タイマデーモンを無効にします。  

    sudo systemctl disable qeqalert.timer  
    または  
    systemctl --user disable qeqalert.timer  
<br>

qEQAlertを停止する場合は、qeqalertデーモンとタイマデーモンを停止します。

    sudo systemctl stop qeqalert.service qeqalert.timer  
    または
    systemctl --user stop qeqalert.service qeqalert.timer  
<br>

**デフォルトでは、PCの起動直後から15[秒]待機した後、本ソフトウェアを遅延起動しています。**  
**もし十分なリソースをもつPCの場合は、この秒数を短くしても正常に動作すると予想されます。**  
<br>

## 2.3. 直接実行する

Systemdサービスを使用せずに、qEQAlertを実行することもできます。  

    sudo qEQAlert --sysconf=<qEQAlert.jsonのパス>
    または
    qEQAlert --sysconf=<qEQAlert.jsonのパス>  
<br>

直接実行した場合において、**[q]キー** または **[Q]キー** ==> **[Enter]キー** を押下することにより、本ソフトウェアを終了することができます。  
<br>

## 2.4 ワンショット機能とCron

Cronを使用して本ソフトウェアを連携する場合は、設定ファイルの<code>oneshot</code>を<code>true</code>に設定して、ワンショット機能を有効にします。  

**<u>ワンショット機能を有効にしている場合は、Systemdサービスと連携できないことにご注意ください。</u>**  
<br>

**※注意**  
**Cronの最小のスケジュール設定単位は分です。**  
**したがって、Cronを使用して秒間隔でソフトウェアを実行することはできません。**  

**本ソフトウェアでは、付属のラッパースクリプトを使用して、そのスクリプト内でループを使用して秒間隔で実行することにより、**  
**Cronを使用して、例えば1分ごとにラッパースクリプトを実行することにより、秒間隔で本ソフトウェアを実行することができます。**  
<br>

以下は、Crontabファイルを編集して、毎日24時間 10秒ごとに本ソフトウェアを実行する設定例です。  
この例では、本ソフトウェアは/usr/local/bin/qEQAlertにインストールされています。  

    sudo crontab -e  
<br>

    # 1[分]ごとにqEQAlertを実行する場合
    * * * * * /usr/local/bin/qEQAlert.sh --sysconf=<qEQAlert.jsonのパス>  
<br>

ラッパースクリプトの内容を以下に示します。  
なお、このスクリプトファイルは、qEQAlertの実行ファイルと同階層のディレクトリにインストールされています。  

    #!/usr/bin/env sh

    appname="qEQAlert"

    # use -f to make the readlink path absolute
    dirname="$(dirname -- "$(readlink -f -- "${0}")" )"

    if [ "$dirname" = "." ]; then
        dirname="$PWD/$dirname"
    fi

    cd $dirname

    # 終了するまでの総秒数
    END=60

    # 現在の秒数
    SECONDS=0

    # 待機時間
    WAIT=10

    # 60秒間 (1分) にわたって秒間隔で実行
    while [ $SECONDS -lt $END ]; do
        # qEQAlertを実行
        "$dirname/$appname" "$@"

        # 指定時間待機
        sleep $WAIT

        SECONDS=$(($SECONDS + $WAIT))
    done
<br>
<br>


# 3. qEQAlertの設定 - qEQAlert.jsonファイル

qEQAlertの設定ファイルであるqEQAlert.jsonファイルでは、  
取得する地震情報の可否、更新時間の間隔、取得した地震情報を書き込むためのログファイルのパス等が設定できます。  

この設定ファイルがあるデフォルトのディレクトリは、<I>**/etc/qEQAlert/qEQAlert.json**</I> です。  
<code>cmake</code>コマンドの実行時にディレクトリを変更することもできます。  
<br>

各設定の説明を記載します。  
<br>

* earthquake
  * alert  
    デフォルト値 : <code>false</code>  
    緊急地震速報(警報)のデータを取得するかどうかを指定します。  
    現在時刻から30[秒]以内に発令された緊急地震速報の場合のみ取得します。  
    デフォルト値は<code>false</code> (無効) です。  
    <br>
  * alerturl  
    * p2p  
      デフォルト値 : <code>"https://api.p2pquake.net/v2/history?codes=556&limit=1&offset=0"</code>  
      <br>
      緊急地震速報(警報)を取得するURLを指定します。  
      現在の仕様では、P2P地震情報からのみ緊急地震速報(警報)を取得することができます。  
      <br>
  * alertlog  
    デフォルト値 : <code>/tmp/eqalert.log</code>  
    緊急地震速報(警報)の地震IDを保存するログファイルのパスを指定します。  
    これは、同じ地震情報を取得して新規スレッドを作成することがないように保存しています。  
    <br>
  * info  
    デフォルト値 : <code>true</code>  
    発生した地震情報のデータを取得するかどうかを指定します。  
    現在時刻から10[分]以内に報告された地震情報の場合のみ取得します。  
    デフォルト値はtrue (有効) です。  
    <br>
  * infourl  
    * jma  
      デフォルト値 : <code>"https://www.data.jma.go.jp/developer/xml/feed/eqvol.xml"</code>  
      <br>
      発生した地震情報を取得する気象庁 (JMA) のURLを指定します。  
      **気象庁から発生した地震情報を取得する場合は、<code>get</code>キーを<code>0</code>に設定する必要があります。**  
      <br>
    * p2p  
      デフォルト値 : <code>"https://api.p2pquake.net/v2/history?codes=551&limit=1&offset=0"</code>  
      <br>
      発生した地震情報から取得するP2P地震情報のURLを指定します。  
      **P2P地震情報から発生した地震情報を取得する場合は、<code>get</code>キーを<code>1</code>に設定する必要があります。**  
      <br>
  * infolog  
    デフォルト値 : <code>/tmp/eqinfo.log</code>  
    発生した地震情報の地震IDや立てたスレッドの情報を保存するログファイルのパスを指定します。  
    これは、地震情報によりスレッドを制御するための情報を保存しています。  
    <br>
  * alertscale / infoscale  
    デフォルト値 : <code>50</code>  
    地震情報を書き込むための基準となる震度を指定します。  
    指定可能な値以外を指定した場合は、強制的に50 (震度5強) に設定されます。  
    <br>
    <code>alertscale</code>は緊急地震速報、<code>infoscale</code>は発生した地震情報の設定です。  
    <br>
    デフォルト値は50 (震度5強) です。  
    <br>
    指定できる値は、以下の通りです。  
    10 (震度1)
    20 (震度2)
    30 (震度3)
    40 (震度4)
    45 (震度5弱)
    50 (震度5強)
    55 (震度6弱)
    60 (震度6強)
    70 (震度7)  
    <br>
  * get  
    デフォルト値 : <code>0</code>  
    JMA (気象庁)、または、P2P地震情報のどちらからデータを取得するかどうかを判別します。  
    <br>
    <code>0</code>の場合 : JMA (気象庁) からデータを取得します。  
    <code>1</code>の場合 : P2P地震情報からデータを取得します。  
    <br>
  * alerturl  
    デフォルト値 : <code>https://api.p2pquake.net/v2/history?codes=556&limit=1&offset=0</code>  
    緊急地震速報(警報)のデータを取得するURLを指定します。  
    <br>
    現在、緊急地震速報(警報)では、P2P地震情報からのデータのみ取得できます。  
    <br>
  * infourl  
    デフォルト値 : <code>https://www.data.jma.go.jp/developer/xml/feed/eqvol.xml</code>  
    発生した地震情報のデータを取得するURLを指定します。  
    <br>
    JMA(気象庁)からデータを取得する場合 :  
    <code>https://www.data.jma.go.jp/developer/xml/feed/eqvol.xml</code>  
    <br>
    P2P地震情報からデータを取得する場合 :  
    <code>https://api.p2pquake.net/v2/history?codes=551&limit=1&offset=0</code>  
    <br>
* thread
  * from  
    デフォルト値 : <code>佐藤</code>  
    地震情報を書き込む時の名前欄を指定します。  
    POSTデータとして送信します。  
    <br>
  * mail  
    デフォルト値 : 空欄  
    地震情報を書き込む時のメール欄を指定します。  
    POSTデータとして送信します。  
    <br>
  * bbs  
    デフォルト値 : 空欄  
    掲示板のBBS名を指定します。  
    POSTデータとして送信します。  
    書き込み時に必須です。  
    <br>
  * shiftjis  
    デフォルト値 : <code>true</code>  
    POSTデータの文字コードをShift-JISに変換するかどうかを指定します。  
    0ch系は、Shift-JISを指定 (<code>**true**</code>) することを推奨します。  
    <br>
  * requesturl  
    デフォルト値 : 空欄  
    地震情報を書き込むため、POSTデータを送信するURLを指定します。  
    <br>
    0ch系の掲示板では、以下のようなURLが多いです。  
    * <code>**http(s)://<ドメイン名>/test/bbs.cgi**</code>  
    * <code>**http(s)://<ドメイン名>/test/bbs.cgi?guid=ON**</code>  

    <br>

    書き込み時に必須です。  
    <br>
  * chtt  
    デフォルト値 : <code>false</code>  
    スレッドタイトルを最新の地震情報のタイトルに変更します。  
    地震情報を既存のスレッドに書き込む時、本文の先頭に!chttという文字列を付加して、POSTデータとして送信します。  
    <br>
    <u>防弾嫌儲系の掲示板において、<code>!chtt</code>コマンドが使用できる場合に有効です。</u>  
    <br>
  * subjecttime  
    デフォルト値 : <code>true</code>  
    **<u>緊急地震速報</u>** で新規スレッドを作成する場合、スレッドタイトルに地震発現(到達)時刻を記載するかどうかを指定します。  
    この値が<code>true</code>の場合、スレッドタイトルに **<u>発現時刻 hh:mm:ss</u>** という文字列が付加されます。  
    <br>
    スレッドタイトルの文字数に制限がある場合は、この値を<code>false</code>にしてください。  
    <br>
    <code>true</code>の時のスレッドタイトル例 : 【緊急地震速報】<震源地名> Mx.x 発現時刻 hh:mm:ss 強い揺れに警戒  
    <code>false</code>の時のスレッドタイトル例 : 【緊急地震速報】<震源地名> Mx.x 強い揺れに警戒  
    <br>
  * expiredxpath  
    デフォルト値 : <code>"/html/head/title"</code>  
    スレッドの生存を判断するときに使用するXPathです。  
    ログファイルに保存されているスレッドタイトルと現在のスレッドタイトルを比較する時に使用します。<br>
    <br>
* oneshot  
  デフォルト値 : <code>false</code>  
  タイマ (<code>interval</code>キーの値を使用) を使用して、地震情報を自動取得するかどうかを指定します。  
  <br>
  Cronを使用して本ソフトウェアをワンショットで実行する場合は、この値を<code>true</code>に指定します。  
  つまり、<code>true</code>に指定する場合、本ソフトウェアをワンショットで実行して、地震情報を1度だけ自動取得することができます。  
  例えば、Systemdサービスが使用できない環境 (Cronのみが使用できる環境) 等で使用します。  
  <br>
* interval  
  デフォルト値 : <code>10</code>  
  P2P地震情報から地震情報を取得する時間間隔 (秒) を指定します。  
  デフォルト値は10[秒]です。  
  <br>
  5[秒]未満、または、60[秒]を超える値を指定した場合は、強制的に10[秒]に指定されます。  
  <br>
  <u>ただし、P2P地震情報では、1分間に60リクエストまでというレート制限があります。</u>  
  <u>それを超えるとレスポンスが遅くなったり拒否 (HTTP ステータスコード 429) される場合があります。</u>  
  <br>
* image (実験的な機能)  
  * enable  
    デフォルト値 : <code>false</code>  
    Yahoo災害情報から該当する地震情報の震度分布の画像を取得するかどうかを指定します。  
    実験的な機能のため、デフォルトでは無効となっています。  
    <br>
  * baseurl  
    デフォルト値 : <code>"https://typhoon.yahoo.co.jp"</code>  
    Yahoo災害情報から地震画像を取得するための起点となるURLを指定します。  
    <br>
  * url  
    デフォルト値 : <code>"https://typhoon.yahoo.co.jp/weather/jp/earthquake/list/"</code>  
    Yahoo災害情報の地震情報一覧のURLを指定します。  
    これは、該当する地震情報を取得するために使用します。  
    <br>
  * eqlistxpath  
    デフォルト値 : <code>"/html/body/div[@id='wrapper']/div[@id='contents']/div[@id='contents-body']/div[@id='main']/div[@class='yjw_main_md']/div[@id='eqhist']/table[@class='yjw_table yjSt boderset']/descendant::tr[position()>1 and position()<=11]"</code>  
    Yahoo災害情報の地震情報一覧にあるテーブルから、該当する地震情報のURLを取得するためのXPath式を指定します。  
    <br>
    XPath式に記述している通り、テーブル先頭の1件目から10件分を抽出して、その中に該当する地震情報が存在するかどうかを確認しています。  
    この件数を調整する場合は、例えば、5件分だけ抽出する場合は<code>11</code>の箇所を<code>6</code>に変更します。  
    <br>
  * eqdetailxpath  
    デフォルト値 : <code>"./td"</code>  
    Yahoo災害情報の地震情報一覧にあるテーブルから、テーブル内の要素を取得するためのXPath式を指定します。  
    <br>
  * equrlxpath  
    デフォルト値 : <code>"./a/@href"</code>  
    Yahoo災害情報の地震情報一覧にあるテーブルから、テーブル内のaタグのhref要素の値を取得するためのXPath式を指定します。  
    <br>
  * eqdateformat  
    デフォルト値 : <code>"yyyy年M月d日 H時m分ごろ"</code>  
    Yahoo災害情報の地震情報一覧にあるテーブルから、該当する地震情報を取得するために使用する文言を指定します。  
    <br>
  * imgxpath  
    デフォルト値 : <code>"/html/body/div[@id='wrapper']/div[@id='contents']/div[@id='contents-body']/div[@id='main']/div[@id='yjw_keihou']/div[@class='earthquakeView']/div[@id='earthquake-01']/img/@src"</code>  
    Yahoo災害情報の該当した地震情報のURLから、震度分布の画像のURLを取得するためのXPath式を指定します。  
    <br>

<br>

    {
        "earthquake": {
            "alert": false,
            "alertlog": "/tmp/eqalert.log",
            "alertscale": 50,
            "alerturl": {
                "p2p": "https://api.p2pquake.net/v2/history?codes=556&limit=1&offset=0"
            },
            "get": 0,
            "info": true,
            "infolog": "/tmp/eqinfo.log",
            "infoscale": 50,
            "infourl": {
                "jma": "https://www.data.jma.go.jp/developer/xml/feed/eqvol.xml",
                "p2p": "https://api.p2pquake.net/v2/history?codes=551&limit=1&offset=0"
            }
        },
        "image": {
            "baseurl": "https://typhoon.yahoo.co.jp",
            "enable": false,
            "eqdateformat": "yyyy年M月d日 H時m分ごろ",
            "eqdetailxpath": "./td",
            "eqlistxpath": "/html/body/div[@id='wrapper']/div[@id='contents']/div[@id='contents-body']/div[@id='main']/div[@class='yjw_main_md']/div[@id='eqhist']/table[@class='yjw_table yjSt boderset']/descendant::tr[position()>1 and position()<=11]",
            "equrlxpath": "./a/@href",
            "imgxpath": "/html/body/div[@id='wrapper']/div[@id='contents']/div[@id='contents-body']/div[@id='main']/div[@id='yjw_keihou']/div[@class='earthquakeView']/div[@id='earthquake-01']/img/@src",
            "url": "https://typhoon.yahoo.co.jp/weather/jp/earthquake/list/"
        },
        "interval": 10,
        "oneshot": false,
        "thread": {
            "bbs": "",
            "chtt": false,
            "expiredxpath": "/html/head/title",
            "from": "佐藤",
            "mail": "",
            "requesturl": "",
            "shiftjis": true,
            "subjecttime": true
        }
    }
<br>
<br>


# 4. ログファイル

## 4.1 緊急地震速報のログファイル - eqalert.log

緊急地震速報の場合は、常にスレッドを新規作成します。  
同じ緊急地震速報でスレッドを新規作成しないように地震IDを保存しています。  
<br>

また、保存された地震IDは削除されることはありません。  
<br>

このファイルは、以下に示すようなフォーマットになっています。  
<br>

    65f3146fd616be440743cfd5
    65f3146fd616be4407lmle40
    65f3146fekfmerkm3r34arge
    ...略

<br>

## 4.2 発生した地震情報のログファイル - eqinfo.log

発生した地震情報の場合、同じ震源地で地震が起きたときは過去に立てたスレッドが生存している場合にのみ、該当スレッドに追加で書き込みます。  
それ以外の場合は、スレッドを新規作成します。  
<br>

また、同じ震源地で地震が起きたとき、かつ、過去に立てたスレッドが存在しない場合は該当するオブジェクトが削除されます。  
<br>

このファイルは、以下に示すようなフォーマットになっています。    
<br>

    [
        {
            "date": "2024/03/19 10:15:00",
            "hypocentre": "十勝沖",
            "id": [
                "65f8e803d616be440743cffc"
            ],
            "prefs": "北海道",  
            "reportdatetime": "2024-06-09T03:24:00+09:00",  
            "thread": "1710837545",
            "title": "【地震】十勝沖 震度2 M4.6",
            "url": "https://www.example.com/test/read.cgi/sample/1710837545/"
        },
        {
            "date": "2024/03/19 19:21:00",
            "hypocentre": "石川県能登地方",
            "id": [
                "65f967c6d616be440743cffe",
                "65f967c6d616be440743cffd"
            ],
            "prefs": "石川県",  
            "reportdatetime": "2024-06-09T03:24:00+09:00",  
            "thread": "1710858633",
            "title": "【地震】石川県能登地方 震度1 M3.2",
            "url": "https://www.example.com/test/read.cgi/sample/1710858633/"
        }
        {
            "date": "2024/06/16 19:35:00",
            "hypocentre": "千葉県北西部",
            "id": [
                "20240616193512"
            ],
            "prefs": "千葉県,東京都,神奈川県",
            "reportdatetime": "2024-06-16T19:38:00+09:00",
            "thread": "1710902547",
            "title": "【地震】千葉県北西部 震度2 M4.1",
            "url": "https://www.example.com/test/read.cgi/sample/1710902547/"
        }
    ]

<br>
<br>
