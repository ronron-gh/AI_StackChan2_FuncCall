# AI_StackChan2_FuncCall
OpenAIのFunction Callingを使って、robo8080さんの[AIｽﾀｯｸﾁｬﾝ2](https://github.com/robo8080/AI_StackChan2)に様々な機能を追加しました。  

※Function Callingを使わない通常の会話もできます。  
※v0.2.0からウェイクワードにも対応しています。使い方は[AIｽﾀｯｸﾁｬﾝ2のREADME](https://github.com/robo8080/AI_StackChan2_README/)を参照ください。

## 開発環境
- M5Stack Core2 / CoreS3
- Platformio

## Function Callingで呼び出せる機能
Function Callingで呼び出せる機能の一覧を下表に示します。

プロンプトや関数の実装は FunctionCall.cpp にまとめています。指示に応じてｽﾀｯｸﾁｬﾝが関数を使いこなしてくれます。  
FunctionCall.cppを改造することで、新たな機能を追加するなどのカスタマイズができます。


| No. | 機能 | 使用例 | デモ | 補足 |
| --- | --- | --- | --- | --- |
| 1 | 時計、タイマー | 「今何時？」<br>「今日は何日？」<br>「3分たったら教えて」<br>「1時間後にパワーオフして」<br> 「タイマーをキャンセルして」| [動画(Twitter)](https://twitter.com/motoh_tw/status/1675171545533251584) |
| 2 | メモ（SDカード） | 「～をメモして」<br>「これから言うことをメモして」<br>「メモを読んで」<br>「メモを消去して」<br>※次回電源投入時にSDカードにメモが残っていたら、「メモが保存されています」とｽﾀｯｸﾁｬﾝが教えてくれます。||メモは notepad.txt というファイル名でSDカードに保存されます。|
| 3 | メール送信 | 「メモをメールして」|[動画(Twitter)](https://twitter.com/motoh_tw/status/1686403120698736640)|GmailのアプリパスワードをSDカードに保存しておく必要があります（「各種設定ファイル」参照）。|
| 4 | メール受信 | 「メールを読んで」|[動画(Twitter)](https://twitter.com/motoh_tw/status/1688132338293882880)|・送信と同じアプリパスワードを使います。<br>・メールサーバを3分毎に確認し、新しいメールがあれば「〇件のメールを受信しています」と教えてくれます。|
| 5 | バス（電車）時刻表 | 「次のバスの時間は？」<br>「その次は？」 |[動画(Twitter)](https://twitter.com/motoh_tw/status/1686404335121686528)|時刻表をSDカードのファイルに保存しておく必要があります（「各種設定ファイル」参照）。|
| 6 | 天気予報 | 「今日の天気は？」<br>「明日の天気は？」 || ・robo8080さんが[Gistに公開してくださったコード](https://gist.github.com/robo8080/60a7bb619f6bae66aa97496371884386)を参考にさせていただきました。<br>・City IDはSDカードの設定ファイルで変更することができます（「各種設定ファイル」参照）。|
| 7 | ウェイクワード登録 | 「ウェイクワード登録」<br>「ウェイクワード有効化」|||




## 各種設定ファイル
#### ●各種APIキー
ｽﾀｯｸﾁｬﾝとお話するためには次のAPIキーをSDカードに保存する必要があります。APIキーの取得方法などの詳細は[AIｽﾀｯｸﾁｬﾝ2のREADME](https://github.com/robo8080/AI_StackChan2_README/)を参照ください。
  - ChatGPT
  - Google Cloud TTS
  - VoiceVox

#### ●メール送受信用のGmailアカウント、アプリパスワード
SDカードに次の順で保存してください。
- 自分のGmailアカウントのメールアドレス
- アプリパスワード（[こちら](https://support.google.com/mail/answer/185833?hl=ja)の説明に沿って取得してください）
- 送信先のメールアドレス

ファイル名：gmail.txt
```
XXXXXXXX@gmail.com 
XXXXXXXXXXXXXXXX
XXXXXXXX@XXXX.com
```

#### ●バス（電車）の時刻表
SDカードに次のように時刻表を保存してください。
※現行は祝日の判別はできません。

ファイル名：bus_timetable.txt（平日）、bus_timetable_sat.txt（土曜）、bus_timetable_holiday.txt（日曜）
```
06:01
06:33
06:53
・
・
・
21:33
22:03
22:33
```
#### ●天気予報のCity ID
SDカードに次のようにCity IDを保存してください（例は神奈川県のID）。

ファイル名：weather_city_id.txt
```
140010
```
City IDは[こちら](https://weather.tsukumijima.net/primary_area.xml)で調べることができます。

## LEDパネルによる状態表示（CoreS3のみ）
CoreS3の場合、ポートAにLEDパネル（NeoPixel互換LED搭載 HEXボード ）を接続すると、次の状態に応じて点灯します。

- 起動完了（ドット絵の起動メッセージが流れる）
- 聞き取り中（青と緑の輪が点灯）
- 聞き取り成功（青と緑の輪が回転）
- 受信メールあり（黄色の輪がゆっくり点滅）

なお、起動完了時の流れるドット絵は次の手順で編集できます。

① tool/LED_Scroll_Pattern.xlsmでドット絵を編集してコードに変換する。  
② HexLED.cpp内の配列led_scroll_pattern01の値を①のコードに差し替える。

## 注意事項
- フォルダ名が長いため、ワークスペースの場所によってはライブラリのインクルードパスが通らない場合があります。
なるべくCドライブ直下に近い場所をワークスペースにしてください。(例 C:\Git)

## バージョン履歴
- v0.0.1
  - 初回リリース
- v0.1.0
  - ChatGPTへのリクエストを10回までループできるようにして、Function Callingで複数の関数を連携できるようにした。
- v0.2.0
  - ウェイクワードに対応。
  - Function Callingで呼び出せる機能を追加。
    - 追加した機能：メモ（SDカード）、メール送信、バス（電車）時刻表、天気予報
- v0.3.0
  - メール受信機能を追加。
  - その他軽微な改善や不具合修正。
- v0.4.0（mainタグ）
  - Function Callingの機能にウェイクワード登録、有効化を追加。
  - LEDパネル（NeoPixel互換LED搭載 HEXボード ）による状態表示に対応（CoreS3のみ）。
