# AI_StackChan2_FuncCall
OpenAIのFunction Callingを使って、robo8080さんの[AIｽﾀｯｸﾁｬﾝ2](https://github.com/robo8080/AI_StackChan2)にタイマー機能やメモ機能などを追加しました。

### デモ : タイマー機能
「○○秒たったら教えて」  
「○○分後にパワーオフして」  
「タイマーを○○秒に変更して」  
「タイマーをキャンセルして」  
といった指示に対応できます。[Twitter](https://twitter.com/motoh_tw/status/1675171545533251584)に動画を公開しています。

### デモ : メモ機能
「これから言うことをメモして」  
「メモを読み上げて」  
といった指示に対応できます。[Twitter]()に動画を公開しています。

### Function Callingに登録した関数
FunctionCall.cppのJSONに関数を定義しています。  
指示に応じてｽﾀｯｸﾁｬﾝが関数を使いこなしてくれます。  
※登録した関数に関係ない内容を話しかけると、関数を使わない通常の返答をします。

### 補足
- ｽﾀｯｸﾁｬﾝとお話するためには次のAPIキーが必要です。取得方法は[AIｽﾀｯｸﾁｬﾝ2のREADME](https://github.com/robo8080/AI_StackChan2_README/)を参照ください。
  - ChatGPT
  - Google Cloud TTS
  - VoiceVox
- フォルダ名が長いため、ワークスペースの場所によってはライブラリのインクルードパスが通らない場合があります。
なるべくCドライブ直下に近い場所をワークスペースにしてください。(例 C:\Git)

### バージョン履歴
- v0.0.1
  - 初回リリース
- v0.1.0（mainタグ 未リリース）
  - ChatGPTへのリクエストを10回までループできるようにして、Function Callingで複数の関数を連携できるようにした。
