//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// Smart_Remote_Controller 20250414 Ver.310 - 20260921 Ver.335                                 //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#include <Arduino.h>
#include <stdio.h>
#include <SPIFFS.h>
#include <iostream>
#include <mutex>

//(11) ------------ irReceive に必要な定義 -----------------------------------------------------//
#include "irReceive.h"
// 端子定義
#define ena_receive_PIN 33  // irReceive レベルシフト回路の動作開始、停止
// 型定義
char m_buf[1]; // 整数を文字列に変換するためのバッファ
String ProntoHEX = F(""); // ProntoHex オブジェクト初期化

// (21) -----------  irSend に必要な定義 -------------------------------------------------------//
//                                                                                             //
// #include <IRremote.hpp> は irReceive.h ですでに include されている。                          //
//                                                                                             // 
// 信号出力端子は PinDefinitionAndMore.h で定義され irReceive.h でこれを include している。        //
// PinDefinitionAndMore.h #define IR_SEND_PIN 4  // D4                                         //
//                                                                                             //
//---------------------------------------------------------------------------------------------//
// プロトタイプ宣言
void send_IR_process( String readStr, int selecter );

// (31) ----------- webserver に必要な定義 -----------------------------------------------------//
#include "webserver.h"

// (41) ----------- SPIFFS ( store, restore ) に必要な定義 -------------------------------------//
// プロトタイプ宣言
void store_Flash( String wrfile,String writeStr );
String restore_Flash( String wrfile );

// (51) ----------- そのほかの定義、グローバル変数定義 -------------------------------------------//
// プロトタイプ宣言
void bt_response(String bt_label, String bt_name); // ボタンレスポンス
//
void startTimer(uint32_t interval, String name,String label); // タイマースレッド開始
void stopTimer(); // タイマースレッド停止
//
void periodicLoop(void *parameter); // 周期実行関数
void triggerPeriodicLoop() ; // 周期実行関数をトリガーする関数
void cancelPeriodicLoop();   // 周期実行関数をキャンセルする関数
//void elapsed_time( String msg ); // 経過時間を表示する関数（動作確認用）

//グローバル変数定義
static String status_information = "情報表示";
static String wrfolder; // 書込み、読出しフォルダ名
static String wrfile;   // 書込みファイル名
static String writeStr; // 書込み文字列
static String readStr;  // 読出し文字列
static int num_reg;     // 「リモコン送信」ボタンに登録するリモコン信号の登録数カウンタ
static String bt_id = "0"; // リモコン信号を複数登録中にリモコン送信ボタンが変更されたか識別用
static int num_period = 108; // リピート周期（初期値 108ms）
static String hex_repeat = "0000 006D 0000 0002 0157 0055 0015 0E6C"; // リピート信号

//タイマー制御に使用するグローバル変数定義
TaskHandle_t worker_task = NULL; // タイマースレッドのハンドル

//フラグとして使用するグローバル変数定義
static bool ena_send = false;     // irSend 許可
static bool ena_store = false;    // Flash 書込み許可
static bool ena_restore = false;  // Flash 読出し許可
static bool ena_assign = false;   // 信号登録許可

volatile bool is_triggered = false;  // 周期実行関数をトリガーするフラグ
volatile bool is_cancelled = false;  // 周期実行関数をキャンセルするフラグ
volatile bool stop_requested = false;// 周期実行関数を停止するフラグ
volatile bool Repeat_flag = false;   // リピート信号送信フラグ

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// setup                                                                                       //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

void setup() {

 // (13) ---------- irReceive setup -----------------------------------------------------------//
  //void irReceive_setup();
  pinMode(LED_BUILTIN, OUTPUT); 
  pinMode(ena_receive_PIN, OUTPUT); //irReceive レベルシフト回路にバイアス電圧を加えるにより動作、停止を制御する
  digitalWrite(ena_receive_PIN, LOW); //初期状態を LOW 停止に設定する
  num_reg = 0;

  Serial.begin(115200);   // Status message will be sent to PC at 9600 baud
  while (!Serial)     ; // Wait for Serial to become available. Is optimized away for some cores.

  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_PORT_USBVIRTUAL) || defined(SERIAL_USB) /*stm32duino*/|| defined(USBCON) /*STM32_stm32*/ \
   || defined(SERIALUSB_PID)  || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
  #endif
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));

  // infos for receive
  Serial.print(RECORD_GAP_MICROS);
  Serial.println(F(" us is the (minimum) gap, after which the start of a new IR packet is assumed"));
  Serial.print(MARK_EXCESS_MICROS);
  Serial.println();
  Serial.println(F("Because of the verbose output (>200 ms at 115200 baud), repeats are not dumped correctly!"));
  Serial.println();
  Serial.println(F("If you receive protocol NEC, Samsung or LG, run also ReceiveDemo to check if your actual protocol is eventually NEC2 or SamsungLG, which is determined by the repeats"));
  Serial.println();

 // (22) ----------- irSend setup -------------------------------------------------------------//
  Serial.begin(115200);
  IrSender.begin();

 // (33) ----------- webserver setup ----------------------------------------------------------//
  doInitialize();             // 初期化処理をして
  connectToWifi();            // Wi-Fiルーターに接続する
  configTime(gmtOffset_sec, 0, ntpServer);  // まず時刻を合わせる

 // (42) ----------- SPIFFS setup -------------------------------------------------------------//
  //SPIFFSのセットアップ
  if (!SPIFFS.begin(true)) {
   Serial.println("An Error has occurred while mounting SPIFFS");
   return;
  }

 // (34) ----------- ブラウザ表示 setup --------------------------------------------------------//
  //index.htmlにアクセスされた時のレスポンス
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", String(), false, editPlaceHolder);
  });
  //style.cssにアクセスされた時のレスポンス
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/style.css", "text/css");
  });
  //js.jsにアクセスされた時のレスポンス
  server.on("/js.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/js.js", "js/js");
  });
  //curtime リクエストに対するレスポンス
  server.on("/curtime", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getCurTime().c_str());
  });
  /*
  //message リクエストに対するレスポンス
  server.on("/message", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getMessage().c_str());
  });
  */
  //msg_information リクエストに対するレスポンス
  server.on("/msg_information", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });

 // (52) ------- リモコン送信ボタンレスポンス setup （ bt_1 ～ bt_16 ）---------------------------//
  //「bt_1」ボタンに対するレスポンス
  server.on("/bt_1_on", HTTP_GET, [](AsyncWebServerRequest *request){
    //タイマー開始、ボタンの番号とラベルを渡す。
    startTimer(num_period, "1", "TV 電源"); 
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_1_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_2」ボタンに対するレスポンス
  server.on("/bt_2_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "2", "TV 入力"); 
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_2_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_3」ボタンに対するレスポンス
  server.on("/bt_3_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "3", "TV 消音");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_3_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_4」ボタンに対するレスポンス
  server.on("/bt_4_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "4", "予備_bt_4");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_4_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_5」ボタンに対するレスポンス
  server.on("/bt_5_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "5", "TV CH+");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_5_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_6」ボタンに対するレスポンス
  server.on("/bt_6_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "6", "TV 地上");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_6_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_7」ボタンに対するレスポンス
  server.on("/bt_7_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "7", "TV Vol+");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_7_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_8」ボタンに対するレスポンス
  server.on("/bt_8_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "8", "予備_bt_8");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_8_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_9」ボタンに対するレスポンス
  server.on("/bt_9_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "9", "TV CH-");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_9_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_10」ボタンに対するレスポンス
  server.on("/bt_10_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "10", "TV BS");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_10_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_11」ボタンに対するレスポンス
  server.on("/bt_11_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "11", "TV VOL-");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_11_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_12」ボタンに対するレスポンス
  server.on("/bt_12_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "12", "予備_bt_12");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_12_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_13」ボタンに対するレスポンス
  server.on("/bt_13_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "13", "BOSE On");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_13_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_14」ボタンに対するレスポンス
  server.on("/bt_14_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "14", "BOSE Off");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_14_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_15」ボタンに対するレスポンス
  server.on("/bt_15_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "15", "BOSE Vol+");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_15_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  //「bt_16」ボタンに対するレスポンス
  server.on("/bt_16_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "16", "BOSE Vol-");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_16_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });

  /* リモコン送信ボタンを追加する場合は、ここに追加していく。 
   
   （例）「bt_16」ボタンに対するレスポンス
   server.on("/bt_16_on", HTTP_GET, [](AsyncWebServerRequest *request){
    startTimer(num_period, "16", "function");
  request->send(200, "text/plain", getMsg_information().c_str()); });
  server.on("/bt_16_off", HTTP_GET, [](AsyncWebServerRequest *request){
    stopTimer(); //タイマー停止
  request->send(200, "text/plain", "OFF"); });
  */

 // (53)------- モード切換、制御ボタンレスポンス setup ( bt_mode, bt_c1 ～ bt_c4 )----------------//
  //モード切換ボタンに対するレスポンス
  server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("モード切換ボタンが押されました。"); //ターミナルにメッセージ出力
    status_information = "モード切換ボタンが押されました。"; //クライアント側で情報を表示する
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //capture 「取込」ボタン bt_c1 に対するレスポンス
  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("\n「取込」ボタンが押されました。"); //ターミナルにメッセージ出力
    digitalWrite(ena_receive_PIN, HIGH); //irReceive レベルシフト回路の動作を開始
    status_information = "「取込」を開始します。リモコン送信機のボタンを押してください。"; //クライアント側で情報を表示する
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //memory 「登録」ボタン bt_c2 に対するレスポンス
  server.on("/memory", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("\n「登録」ボタンが押されました。"); //ターミナルにメッセージ出力
    digitalWrite(ena_receive_PIN, LOW); //irReceive レベルシフト回路の動作を停止
    ena_store = true;
    num_reg = num_reg + 1; //「リモコン送信」ボタンに登録する登録数を加算する
    status_information = "登録を開始します。\n登録する「任意」のボタンを押してください。"; //クライアント側で情報を表示する
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //finish 「終了」ボタン bt_c3 に対するレスポンス
  server.on("/finish", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("\n「終了」ボタンが押されました。"); //ターミナルにメッセージ出力
    digitalWrite(ena_receive_PIN, LOW); //irReceive レベルシフト回路の動作を停止。
    ena_send = false; //irsend を禁止
    ena_store = false; //Flash 書込みを禁止
    ena_restore = false; //Flash 読出しを禁止
    ena_assign = false; //信号登録禁止
    status_information = "取込、登録を終了しました。"; //クライアント側で情報を表示する
    num_reg = 0; //登録数カウンタをリセットする
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //予備ボタン bt_c4 に対するレスポンス
  server.on("/bt_c4", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.print("Button bt_c4 pushed : "); //ターミナルにメッセージ出力
    status_information = "Button bt_c4 pushed : "; //クライアント側で情報を表示する
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });

 // (35)-------- webserver setup --------------------------------------------------------------//
  //サーバーを開始する
  server.begin();
 
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// Loop                                                                                        //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

void loop() {

  // (14) ------------- receive_IR_process ----------------------------------------------------//
  // 「取込」ボタンを押すと、digitalWrite(ena_receive_PIN, HIGH); //irReceive によりレベルシフト回路
  //  が動作してリモコン信号受信回路から「取込」を開始できるようになる。

  receive_IR_process( ); //この関数を実行すると戻り値 writeStr が得られる。

  // (43) -----------「登録」ボタンに続けて「リモコン送信」ボタンが押された時の処理 -----------------//

  if ((ena_store == true) && (ena_assign == true)){ //Flash 書込み許可、信号登録許可 の場合
    
    // rceive_IR_process(); で受信した信号 writeString を Flash に書込む。
    // ファイル名は、登録するボタン名に対応した wrfolder と 登録数カウンタ num_reg から生成する。
    // 登録数カウンタ num_reg は「登録」ボタンを押すごとに +1 インクリメントされる。
    // (例) bt_1 の場合 wrfile = "/bt_1/bt_1_1.txt"

    wrfile = wrfolder + wrfolder + "_" + num_reg + ".txt";
    store_Flash( wrfile, writeStr ); //受信した信号を Flash に書込む
    
    // 当該「リモコン送信」ボタンにリモコン信号が何個登録されたかを示す登録数カウンタ num_reg を Flash に書込む。
    // (例) bt_1 の場合 wrfile = "/bt_1/num_reg.txt"
    
    wrfile = wrfolder + "/num_reg.txt";
    String str_buf = dtostrf(num_reg,2,0,m_buf);  //整数を文字列に変換

    //上記で一度 storeFlash(); を実行すると関数内で ena_Flash = false 書込みを禁止されるため再度許可する。
    ena_store = true; //Flash 書込み許可
    store_Flash( wrfile, str_buf ); //登録数カウンタ値を Flash に書込む
    ena_assign = false; //信号登録禁止
  }

  // (22) ----------- 「リモコン送信」ボタンが押された時の処理 ------------------------------------//

  if ((ena_restore == true) && (ena_send == true)){ //Flash 読出し許可、irSend 許可 の場合
   
    //登録数カウンタ num.reg の値を Flash から読出す
    wrfile = wrfolder + "/num_reg.txt"; //ファイル名生成 (例) wrfile = "/bt_1/num_reg.txt"
    restore_Flash(wrfile);              //Flash から読出し
    int count = atoi(readStr.c_str());  //for ループでカウントするために文字列を整数に変換する。
    
    //「リモコン送信」ボタンに何も登録されていなければ処理を終了する。
    if ( count == 0){
      ena_restore = false; //Flash 読出し禁止
      ena_send = false;    //irSend 禁止
      status_information = "このボタンには何も登録されていません。"; //クライアント側で情報を表示する
      cancelPeriodicLoop() ; //タイマー停止
      Serial.println(status_information); //ターミナル出力
    }
    //「リモコン送信」ボタンに登録された登録数 num_reg 回だけ Flash からて送信する。
    for (int i =1 ; i <= count; i++){ // count は Flash からた登録数を整数化したもの
    
      //ファイル名生成 (例) wrfile = "bt_1/bt1_1.txt"
      wrfile = wrfolder + wrfolder + "_" + i + ".txt";
      restore_Flash(wrfile); //Flash からリモコン信号を読出す
      //delay(100);
      send_IR_process(readStr, 0);  //リモコン信号送信

      //ループの最後でフラグを初期化しておく
      if ( i == count ){
        ena_restore = false;    //Flash 書込み禁止
        ena_send = false;       //irSend 禁止
        triggerPeriodicLoop();  //待機中のタイマースレッド起動
      }
    }
 }

// (56) ----------- 「リピート送信」処理 ------------------------------------------------------//

  if ( Repeat_flag == true){  //リピート信号開始フラグが true の場合
    ena_send = true;                 //irSend 許可
    send_IR_process(hex_repeat, 1);  //リピート信号送信
    //send_IR_process(readStr, 1);   //リモコン信号送信を繰り返すことも可能
    Repeat_flag = false;
  }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// 関数定義                                                                                     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//

// (15) ----------- receive_IR_process --------------------------------------------------------//

// PIN 33 が HIGH 出力になると、irReceive は待ち受け状態になり PIN 15 に入力があれば動作を開始する。
// リモコン信号が入力されると戻り値 writeStr を返す。 

String receive_IR_process() {

  if (IrReceiver.decode()) {  // Grab an IR code
    Serial.println("\n**************************************************************");
    Serial.println("receive_IR_process started");
    Serial.println("**************************************************************");
    // At 115200 baud, printing takes 200 ms for NEC protocol and 70 ms for NEC repeat
    Serial.println(); // blank line between entries
    Serial.println(); // 2 blank lines between entries
    IrReceiver.printIRResultShort(&Serial);
    // Check if the buffer overflowed
    if (IrReceiver.decodedIRData.flags & IRDATA_FLAGS_WAS_OVERFLOW) {
      Serial.println(F("Try to increase the \"RAW_BUFFER_LENGTH\" value of " STR(RAW_BUFFER_LENGTH) " in " __FILE__));
      // see also https://github.com/Arduino-IRremote/Arduino-IRremote#compile-options--macros-for-this-library
    } else {      
      if (IrReceiver.decodedIRData.protocol == UNKNOWN) {
        Serial.println(F("Received noise or an unknown (or not yet enabled) protocol"));
      }
      Serial.println();
      IrReceiver.printIRSendUsage(&Serial);
      Serial.println();
      Serial.println(F("Raw result in internal ticks (50 us) - with leading gap"));
      IrReceiver.printIRResultRawFormatted(&Serial, false); // Output the results in RAW format
      Serial.println(F("Raw result in microseconds - with leading gap"));
      IrReceiver.printIRResultRawFormatted(&Serial, true);  // Output the results in RAW format
      Serial.println();                               // blank line between entries
      Serial.print(F("Result as internal 8bit ticks (50 us) array - compensated with MARK_EXCESS_MICROS="));
      Serial.println(MARK_EXCESS_MICROS);
      IrReceiver.compensateAndPrintIRResultAsCArray(&Serial, false); // Output the results as uint8_t source code array of ticks
      Serial.print(F("Result as microseconds array - compensated with MARK_EXCESS_MICROS="));
      Serial.println(MARK_EXCESS_MICROS);
      IrReceiver.compensateAndPrintIRResultAsCArray(&Serial, true); // Output the results as uint16_t source code array of micros
      IrReceiver.printIRResultAsCVariables(&Serial);  // Output address and data as source code variables
      Serial.println();                               // blank line between entries

      IrReceiver.compensateAndPrintIRResultAsPronto(&Serial);

      /*
        * Example for using the compensateAndStorePronto() function.
        * Creating this String requires 2210 bytes program memory and 10 bytes RAM for the String class.
        * The String object itself requires additional 440 bytes RAM from the heap.
        * This values are for an Arduino Uno.
      */
      Serial.println();                                     // blank line between entries
      String ProntoHEX = F("");                             // Assign string to ProtoHex string object
//        String ProntoHEX = F("Pronto HEX contains: ");        // Assign string to ProtoHex string object
      if (int size = IrReceiver.compensateAndStorePronto(&ProntoHEX)) {   // Dump the content of the IReceiver Pronto HEX to the String object
//            // Append compensateAndStorePronto() size information to the String object (requires 50 bytes heap)
//            ProntoHEX += F("\r\nProntoHEX is ");              // Add codes size information to the String object
//            ProntoHEX += size;
//            ProntoHEX += F(" characters long and contains "); // Add codes count information to the String object
//            ProntoHEX += size / 5;
//            ProntoHEX += F(" codes");
//            Serial.println(ProntoHEX.c_str());                // Print to the serial console the whole String object

        //----------- Flash に書込むために以下を追加した ----------//
        writeStr = ProntoHEX.c_str();  //Flash に書込むデータはグロ－バル変数 writeStr を使用する
        Serial.print("writeStr.length = ");
        Serial.println(writeStr.length()); //文字列の長さを出力する
        //ターミナルにメッセージ出力
        Serial.println("**************************************************************");
        Serial.println("receive_IR_process finishd");
        Serial.println("**************************************************************");
      
        Serial.println();                                 // blank line between entries
      }
   }
    IrReceiver.resume();                            // Prepare for the next IR frame
    delay(200);
    String str_buf = dtostrf(writeStr.length(), 5, 0, m_buf); //文字列 writeStr の長さを抽出して文字列に変換する 
    //クライアント側で情報を表示する
    status_information = "取込んだリモコン信号の長さは、" + str_buf + " バイトです。\nこの内容でよければ、「登録」ボタンを押してください。\nやり直すには、リモコン送信機のボタンをもう一度押してください。";
  }
  return writeStr;
} 

// (23) ----------- send_IR_process -----------------------------------------------------------//

void send_IR_process( String readStr, int selecter ){ //FALSH からたときの戻り値が readStr
  //delay(200); //Flash 書込み直後に読出された場合を想定して遅延を設けている。
  //elapsed_time( "send_IR_process started " );
  portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED; //鍵（ミューテックス）
  if ( ena_send == true ){ //irSend 許可の場合
    //ターミナルにメッセージ出力
    Serial.println("\n**************************************************************");
    Serial.println("send_IR_process started");
    Serial.println("**************************************************************");
    //リモコン信号を送信する
    taskENTER_CRITICAL(&myMutex); //クリティカルセクションの開始
    IrSender.sendPronto(readStr.c_str(), selecter);
    taskEXIT_CRITICAL(&myMutex); //クリティカルセクションの終了
     //ターミナルにメッセージ出力
    Serial.println("readStr.c_str() = ");
    Serial.println(readStr.c_str());
    Serial.println("send trun on at " + String(millis()));
    Serial.println("**************************************************************");
    Serial.println("send_IR_process finished");
    Serial.println("**************************************************************");
  }
  //elapsed_time( "send_IR_process finished" );
}

// (34) ----------- 初期化処理 -----------------------------------------------------------------//
// SPI 速度と端子の初期化
void doInitialize() {
  Serial.begin(SPI_SPEED);
}

// (36) ----------- Wi-Fi 接続処理　------------------------------------------------------------//
// Wi-Fiルーターに接続する
void connectToWifi() {
  Serial.print("Connecting to Wi-Fi ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  //モニターにローカル IPアドレスを表示する
  Serial.println("WiFi connected.");
  Serial.print("  *IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

// (37) ｰｰｰｰｰｰｰｰｰ-- プレースホルダー処理 ---------------------------------------------------------//
String editPlaceHolder(const String& var) {
  if (var == "CURTIME") {
    return getCurTime();
  }
  else if (var == "MSG_INFORMATION"){
    return getMsg_information();
  }
  return "";
}

// (38) ----------- 現在の日時を取得する --------------------------------------------------------//
String getCurTime() {
  struct tm timeinfo;
  char buf[64];
  if(getLocalTime(&timeinfo)) {
    sprintf(buf,"%04d/%02d/%02d %02d:%02d:%02d",
    timeinfo.tm_year+1900, timeinfo.tm_mon+1, timeinfo.tm_mday,
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    return buf;
  }
  return "";
}

// (39) ----------- Smart Remote Controller の状態を通知する ------------------------------------//
String getMsg_information(){
  return status_information;
}

// (44) ----------- store_Flash ---------------------------------------------------------------//
// ファイル名( wrfile )と文字列( writeStr )を引数として Flash に書込む。 ena_store = true で書込み許可。
void store_Flash( String wrfile,String writeStr ){
  if ( ena_store == true ){
    //ターミナルにメッセージ出力
    Serial.println("\n**************************************************************");
    Serial.println("store_Flash started");
    Serial.println("**************************************************************");

    File fw = SPIFFS.open(wrfile.c_str(), "w");  //ファイルを書込みモードで開く
    fw.println(writeStr);  //ファイルに書込み
    fw.close();  //ファイルを閉じる
    
    //ターミナルにメッセージ出力
    Serial.print("wrfile = ");
    Serial.println(wrfile);
    Serial.print("writeStr= ");
    Serial.println(writeStr);
    delay (1000);
    Serial.println("**************************************************************");
    Serial.println("store_Flash finished");
    Serial.println("**************************************************************");
    //書込みフラグを初期化して禁止する。
    ena_store = false; 
  }
}

// (45) ----------- restore_Flash -------------------------------------------------------------//
//ファイル名( wrfile )を引数として Flash から読出し readStr を返す。 ena_restore = true 読出し許可。
String restore_Flash( String wrfile ){
  if ( ena_restore == true ){
    //ターミナルにメッセージ出力
    //elapsed_time("restore_Flash started");
    Serial.println("\n**************************************************************");
    Serial.println("restore_Flash started");
    Serial.println("**************************************************************");

    File fr = SPIFFS.open(wrfile.c_str(),"r"); //ファイルを読出しモードで開く
    readStr = fr.readStringUntil('\n'); //改行まで1行
    fr.close(); //ファイルを閉じる

    //ターミナルにメッセージ出力
    Serial.print("readStr = \n");
    Serial.println(readStr);
    delay(200);
    Serial.print("readStr.length = "); 
    Serial.println(readStr.length()); 
    Serial.println("**************************************************************");
    Serial.println("restore_Flash finished");
    Serial.println("**************************************************************");
    //elapsed_time("restore_Flash finished");
  }
  return readStr;
}

// (54) ----------- bt_response ボタンレスポンス ------------------------------------------------//
// リモコン送信ボタンに表示するラベル( bt_label ) とボタン名 ( bt_name ) を引数とするボタン動作
void bt_response(String bt_label, String bt_name){
  wrfolder = "/bt_" + bt_label; //フォルダ名生成（bt_label の値は 1-16）
  wrfile = "/bt_" + bt_label;   //ファイル名生成
  
  //ボタンに登録するときの処理
  if (ena_store == true){     //「取込」、「登録」に続けて「リモコン送信」ボタンを押した場合、
    if ( bt_id != bt_label){ //複数「登録」で異なる「リモコン送信」ボタンを押した場合、
      num_reg = 1;            //リモコン送信ボタンに登録するリモコン信号の数を 1 に戻しておく。
    }   
    ena_assign = true;        //信号登録許可
    delay(1000);              //Flash 書込みを待ってから次の処理へ進める。
    String str_buf = dtostrf(num_reg,2,0,m_buf);  //整数を文字列に変換
    //クライアント側で情報を表示する
    status_information = bt_name + " ボタンに"+ str_buf + " つめのリモコン信号を「登録」しました。";
    //複数登録中にリモコン送信ボタンが変更されたか識別用 bt_id に bt_label (1-16) を初期値として設定する。
    bt_id = bt_label;
  }
    //「リモコン送信」ボタンのみを押したときの処理。
  else if ((ena_store == false)){
    ena_restore = true;  //Flash 読出しを許可
    ena_send = true;     //irSend リモコン信号送信を許可
    //クライアント側で情報を表示する
    status_information = bt_name + " リモコン信号を送信しました。";
  } 
}


// (61) ----------- バックグラウンドで周期実行される関数 ------------------------------------------//

void periodicLoop(void *parameter){
  uint32_t interval = (uint32_t)parameter;

  // is_triggered または is_cancelled になるまで待つ
  while (!is_triggered && !is_cancelled && !stop_requested) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // キャンセル
  if (is_cancelled) {
    worker_task = NULL;
    vTaskDelete(NULL);
    return;
  }

  // 周期処理開始
  TickType_t lastWakeTime = xTaskGetTickCount();
  while (!stop_requested && !is_cancelled) {
    // ★ 周期的に実行する処理
    Repeat_flag = true; // リピート信号送信フラグを true にする
    vTaskDelayUntil( &lastWakeTime, pdMS_TO_TICKS( interval ));
  }

  // 終了
  worker_task = NULL;
  vTaskDelete(NULL);
}

// (62) ----------- タイマー開始 ----------------------------------------------------------------//

void startTimer(uint32_t interval, String name,String label){
  if (worker_task != NULL) {
    return;
  }

  is_triggered = false;
  is_cancelled = false;
  stop_requested = false;
  bt_response (name.c_str(), label.c_str());

  xTaskCreate( periodicLoop, "periodicLoop", 4096, (void *)interval, 1, &worker_task );
}

// (63) ----------- タイマー終了 ---------------------------------------------------------------//

void stopTimer(){
  stop_requested = true;
}

// (64) ----------- 待機中のタイマースレッドの起動トリガ -----------------------------------------//

void triggerPeriodicLoop() {
  is_triggered = true; // 起動トリガセット
}

// (65) ----------- タイマースレッドのキャンセルトリガ ------------------------------------------//

void cancelPeriodicLoop() {
  is_cancelled = true; // キャンセルフラグセット
}