// (32) ----------- webserver に必要な定義 -----------------------------------------------------//

#include <AsyncEventSource.h>
#include <AsyncWebSocket.h>
#include <AsyncWebSynchronization.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include <StringArray.h>
#include <WebAuthentication.h>
#include <WebHandlerImpl.h>
#include <WebResponseImpl.h>
#include <HTTPUpdate.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// プロトタイプ宣言
void doInitialize();
void connectToWifi();
String editPlaceHolder(const String&);
String getMessage();
String getCurTime();
String getMsg_information();
void webpage_setup();

// ルーター接続情報
#define WIFI_SSID "XXXXXXXX"
#define WIFI_PASSWORD "YYYYYYYY"

// NTPサーバー時刻設定用
const char* ntpServer = "pool.ntp.org";
const int   gmtOffset_sec = 9 * 3600;

// 基本パラメータ定義
#define SPI_SPEED   115200
#define CST_ON      "ON"
#define CST_OFF     "OFF"

// Webサーバーオブジェクト
#define HTTP_PORT 80
AsyncWebServer server(HTTP_PORT);

// LED ピン定義
// Smart Remote Controller の機能には無関係だが動作確認用に残している。
const int ledPin = 25;              // LED出力接続ピン
String ledState = "OFF";            // 出力ピンの状態