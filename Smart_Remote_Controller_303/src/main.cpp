//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
// Smart_Remote_Controller 20241209 Ver.303                                                    //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++//
#include <Arduino.h>
#include <stdio.h>
#include <SPIFFS.h>

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
void send_IR_process( String readStr );

// (31) ----------- webserver に必要な定義 -----------------------------------------------------//
#include "webserver.h"

// (41) ----------- SPIFFS ( store, restore ) に必要な定義 -------------------------------------//
// プロトタイプ宣言
void store_FLASH( String wrfile,String writeStr );
String restore_FLASH( String wrfile );

// (51) ----------- そのほかの定義、グローバル変数定義 -------------------------------------------//
// プロトタイプ宣言
void bt_response(String bt_label, String bt_name); // ボタンレスポンス

//グローバル変数定義
static String status_information = "情報表示";
static String wrfolder; // 書込み、読出しフォルダ名
static String wrfile;   // 書込みファイル名
static String writeStr; // 書込み文字列
static String readStr;  // 読出し文字列
static int num_reg;     // 「リモコン送信」ボタンに登録するリモコン信号の登録数カウンタ
static String bt_id = "0"; // リモコン信号を複数登録中にリモコン送信ボタンが変更されたか識別用

//フラグとして使用するグローバル変数定義
static bool ena_send = false;     // irSend 許可
static bool ena_store = false;    // FLASH 書込み許可
static bool ena_restore = false;  // FLASH 読出し許可
static bool ena_assign = false;   // 信号登録許可

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
  server.on("/bt_1", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("1", "TV 電源");
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //「bt_2」ボタンに対するレスポンス
  server.on("/bt_2", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("2", "TV 入力");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_3」ボタンに対するレスポンス
  server.on("/bt_3", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("3", "TV 消音");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_4」ボタンに対するレスポンス
  server.on("/bt_4", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("4", "予備_bt_4");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_5」ボタンに対するレスポンス
  server.on("/bt_5", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("5", "TV CH+");
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //「bt_6」ボタンに対するレスポンス
  server.on("/bt_6", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("6", "TV 地上");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_7」ボタンに対するレスポンス
  server.on("/bt_7", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("7", "TV Vol+");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_8」ボタンに対するレスポンス
  server.on("/bt_8", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("8", "予備_bt_8");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_9」ボタンに対するレスポンス
  server.on("/bt_9", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("9", "TV CHｰ");
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //「bt_10」ボタンに対するレスポンス
  server.on("/bt_10", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("10", "TV BS");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_11」ボタンに対するレスポンス
  server.on("/bt_11", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("11", "TV Vol-");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
   });
  //「bt_12」ボタンに対するレスポンス
  server.on("/bt_12", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("12", "予備_bt_12");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_13」ボタンに対するレスポンス
  server.on("/bt_13", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("13", "BOSE On");
    request->send_P(200, "text/plain", getMsg_information().c_str());
  });
  //「bt_14」ボタンに対するレスポンス
  server.on("/bt_14", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("14", "BOSE Off");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_15」ボタンに対するレスポンス
  server.on("/bt_15", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("15", "BOSE Vol+");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });
  //「bt_16」ボタンに対するレスポンス
  server.on("/bt_16", HTTP_GET, [](AsyncWebServerRequest *request){
    bt_response ("16", "BOSE Vol-");
    request->send_P(200, "text/plain", getMsg_information().c_str()); 
  });

  /* リモコン送信ボタンを追加する場合は、ここに追加していく。 
   
   （例）「bt_16」ボタンに対するレスポンス
    server.on("/bt_16", HTTP_GET, [](AsyncWebServerRequest *request){
      bt_response ("16", "BOSE Vol-");
      request->send_P(200, "text/plain", getMsg_information().c_str()); 
    });
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
    ena_store = false; //FLASH 書込みを禁止
    ena_restore = false; //FLASH 読出しを禁止
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

  if ((ena_store == true) && (ena_assign == true)){ //FLASH 書込み許可、信号登録許可 の場合
    
    // rceive_IR_process(); で受信した信号 writeString を FLASH に書込む。
    // ファイル名は、登録するボタン名に対応した wrfolder と 登録数カウンタ num_reg から生成する。
    // 登録数カウンタ num_reg は「登録」ボタンを押すごとに +1 インクリメントされる。
    // (例) bt_1 の場合 wrfile = "/bt_1/bt_1_1.txt"

    wrfile = wrfolder + wrfolder + "_" + num_reg + ".txt";
    store_FLASH( wrfile, writeStr ); //受信した信号を FLASH に書込む
    
    // 当該「リモコン送信」ボタンにリモコン信号が何個登録されたかを示す登録数カウンタ num_reg を FLASH に書込む。
    // (例) bt_1 の場合 wrfile = "/bt_1/num_reg.txt"
    
    wrfile = wrfolder + "/num_reg.txt";
    String str_buf = dtostrf(num_reg,2,0,m_buf);  //整数を文字列に変換

    //上記で一度 storeFLASH(); を実行すると関数内で ena_FLASH = false 書込みを禁止されるため再度許可する。
    ena_store = true; //FLASH 書込み許可
    store_FLASH( wrfile, str_buf ); //登録数カウンタ値を FLASH に書込む
    ena_assign = false; //信号登録禁止
  }

  // (22) ----------- 「リモコン送信」ボタンが押された時の処理 ------------------------------------//

  if ((ena_restore == true) && (ena_send == true)){ //FLASH 読出し許可、irSend 許可 の場合
    
    //登録数カウンタ num.reg の値を FLAH から読み出す
    wrfile = wrfolder + "/num_reg.txt"; //ファイル名生成 (例) wrfile = "/bt_1/num_reg.txt"
    restore_FLASH(wrfile);              //FALSH から読出し
    int count = atoi(readStr.c_str());  //for ループでカウントするために文字列を整数に変換する。
    
    //「リモコン送信」ボタンに何も登録されていなければ処理を終了する。
    if ( count == 0){
      ena_restore = false; //FLASH 読出し禁止
      ena_send = false;    //irSend 禁止
      status_information = "このボタンには何も登録されていません。"; //クライアント側で情報を表示する
      Serial.println(status_information); //ターミナル出力
    }
    //「リモコン送信」ボタンに登録された登録数 num_reg 回だけ FLASH から読み出して送信する。
    for (int i =1 ; i <= count; i++){ // count は FLASH から読み出した登録数を整数化したもの
    
      //ファイル名生成 (例) wrfile = "bt_1/bt1_1.txt"
      wrfile = wrfolder + wrfolder + "_" + i + ".txt";
      restore_FLASH(wrfile); //FLAH からリモコン信号を読み出す
      delay(100);
      send_IR_process(readStr);  //リモコン信号送信
    
      //ループの最後でフラグを初期化しておく
      if ( i == count ){
        ena_restore = false; //FLASH 書込み禁止
        ena_send = false;    //irSend 禁止
      }
    }
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

        //----------- FLASH に書込むために以下を追加した ----------//
        writeStr = ProntoHEX.c_str();  //FLASH に書込むデータはグロ－バル変数 writeStr を使用する
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

void send_IR_process( String readStr ){ //FALSH から読み出したときの戻り値が readStr
  delay(200); //FLASH 書込み直後に読み出された場合を想定して遅延を設けている。
  if ( ena_send == true ){ //irSend 許可の場合
    //ターミナルにメッセージ出力
    Serial.println("\n**************************************************************");
    Serial.println("send_IR_process started");
    Serial.println("**************************************************************");
    //リモコン信号を送信する
    IrSender.sendPronto(readStr.c_str());
     //ターミナルにメッセージ出力
    Serial.println("readStr.c_str() = ");
    Serial.println(readStr.c_str());
    Serial.println("send trun on at " + String(millis()));
    Serial.println("**************************************************************");
    Serial.println("send_IR_process finished");
    Serial.println("**************************************************************");
  }
}

// (34) ----------- 初期化処理 -----------------------------------------------------------------//
// SPI 速度と端子の初期化
void doInitialize() {
  Serial.begin(SPI_SPEED);
  /*
  pinMode(ledPin, OUTPUT);           // GPIO設定：LED
  digitalWrite(ledPin, LOW);
  ledState = CST_OFF;
  */
}

// (35) ----------- Wi-Fi 接続処理　------------------------------------------------------------//
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

// (36) ｰｰｰｰｰｰｰｰｰ-- プレースホルダー処理 ---------------------------------------------------------//
String editPlaceHolder(const String& var) {
  if (var == "CURTIME") {
    return getCurTime();
  }
  else if (var == "MSG_INFORMATION"){
    return getMsg_information();
  }
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
}

// (39) ----------- Smart Remote Controller の状態を通知する ------------------------------------//
String getMsg_information(){
  return status_information;
}

// (44) ----------- store_FLASH ---------------------------------------------------------------//
//ファイル名( wrfile )と文字列( writeStr )を引数として FLASH に書込む。 ena_store = true で書込み許可。
void store_FLASH( String wrfile,String writeStr ){
  if ( ena_store == true ){
    //ターミナルにメッセージ出力
    Serial.println("\n**************************************************************");
    Serial.println("store_FLASH started");
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
    Serial.println("store_FLASH finished");
    Serial.println("**************************************************************");
    //書込みフラグを初期化して禁止する。
    ena_store = false; 
  }
}

// (45) ----------- restore_FLASH -------------------------------------------------------------//
//ファイル名( wrfile )を引数として FLASH から読出し readStr を返す。 ena_restore = true 読出し許可。
String restore_FLASH( String wrfile ){
  if ( ena_restore == true ){
    //ターミナルにメッセージ出力
    Serial.println("\n**************************************************************");
    Serial.println("restore_FLASH started");
    Serial.println("**************************************************************");

    File fr = SPIFFS.open(wrfile.c_str(),"r"); //ファイルを読込みモードで開く
    readStr = fr.readStringUntil('\n'); //改行まで1行読み出し
    fr.close(); //ファイルを閉じる

    //ターミナルにメッセージ出力
    Serial.print("readStr = \n");
    Serial.println(readStr);
    delay(200);
    Serial.print("readStr.length = "); 
    Serial.println(readStr.length()); 
    Serial.println("**************************************************************");
    Serial.println("restore_FLASH finished");
    Serial.println("**************************************************************");
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
    delay(1000);              //FLASH 書込みを待ってから次の処理へ進める。
    String str_buf = dtostrf(num_reg,2,0,m_buf);  //整数を文字列に変換
    //クライアント側で情報を表示する
    status_information = bt_name + " ボタンに"+ str_buf + " つめのリモコン信号を「登録」しました。";
    //複数登録中にリモコン送信ボタンが変更されたか識別用 bt_id に bt_label (1-16) を初期値として設定する。
    bt_id = bt_label;
  }
    //「リモコン送信」ボタンのみを押したときの処理。
  else if ((ena_store == false)){
    ena_restore = true;  //FLASH 読出しを許可
    ena_send = true;     //irSend リモコン信号送信を許可
    //クライアント側で情報を表示する
    status_information = bt_name + " リモコン信号を送信しました。";
  } 
}
