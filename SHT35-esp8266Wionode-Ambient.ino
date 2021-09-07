#include <ESP8266WiFi.h>
#include <Wire.h>
#include <ClosedCube_SHT31D.h>
#include <Ambient.h>

unsigned long prev;
int wifiFailCount = 0;

//wionode config
const uint8_t PORT0A = 1;
const uint8_t PORT0B = 3;
const uint8_t PORT1A = 4;
const uint8_t PORT1B = 5;
const uint8_t PORT_POWER = 15;
const uint8_t FUNC_BTN = 0;
const uint8_t BLUE_LED = 2;
const uint8_t RED_LED = PORT_POWER;
const uint8_t UART_TX = PORT0A;
const uint8_t UART_RX = PORT0B;
const uint8_t I2C_SDA = PORT1A;
const uint8_t I2C_SCL = PORT1B;

WiFiClient client;
Ambient ambient;
ClosedCube_SHT31D sht3xd;

//////////////////////////////////////////////
//  この間の値が個人で設定する値です。
const char* ssid = "ssid";//SSID
const char* password = "password";//パスワード

unsigned int channelId = ; // AmbientのチャネルID ex)44924
const char* writeKey = ""; // ライトキー ex)"5a5c92406b4bb53d"

unsigned long sendInterval = 5000;//送信間隔
/////////////////////////////////////////////

//セットアップ関数 起動時に一度だけ実行される
void setup() {
  //Wionode setup
  setupWionode();

  //sensor setup
  setupSensor();

  //WiFi setup
  setupWiFi();

  //ambient setup
  ambient.begin(channelId, writeKey, &client); // チャネルIDとライトキーを指定してAmbientの初期化
  
  delay(5);
  prev = millis();
}

//wionodeのセットアップ。
void setupWionode() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println("*** Wake up WioNode...");
  pinMode(PORT1B, INPUT);
  pinMode(FUNC_BTN, INPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(PORT_POWER, OUTPUT);
  digitalWrite(PORT_POWER, HIGH);
  digitalWrite(BLUE_LED, HIGH);
}

//sensorのセットアップ
void setupSensor() {
  Serial.println("Setting up sensor");
  Wire.begin(I2C_SDA, I2C_SCL);
  sht3xd.begin(0x45);
  SHT31D_ErrorCode resultSoft = sht3xd.softReset();
  if (resultSoft != SHT3XD_NO_ERROR) {
    Serial.print("[SHT3X]Error code: ");
    Serial.println(resultSoft);
    while (1);
  }
  Serial.println("Sensor ready");
}

//wifiのセットアップ
void setupWiFi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print('.');
    addWifiFailCount();
  }
  wifiFailCount = 0;
  Serial.print("WiFi connected\r\nIP address: ");
  Serial.println(WiFi.localIP());
}

//wifiへの接続失敗が50回を超えたら、wionodeをリセットする
void addWifiFailCount() {
  if (50 <= wifiFailCount++) {
    ESP.restart();
  }
}

//ループ関数　繰り返し実行される
void loop() {
  //wifi接続確認、接続が途切れていれば、再接続
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Not connected to the Internet:");
    Serial.println(WiFi.status());
    setupWiFi();
    prev = millis();
  }

  //既定の送信間隔を超えていたら、実行する
  if ((millis() - prev) >= sendInterval) {
    digitalWrite(BLUE_LED, LOW);

    SHT31D result = sht3xd.readTempAndHumidity(SHT3XD_REPEATABILITY_LOW, SHT3XD_MODE_CLOCK_STRETCH, 50);
    if (result.error != SHT3XD_NO_ERROR) {
      Serial.print("[SHT3X]Error code: ");
      Serial.println(result.error);
      return;
    }

    Serial.print("Temp: ");
    Serial.print(result.t, 4);
    Serial.println("*C");
    Serial.print("Humi: ");
    Serial.print(result.rh, 4);
    Serial.println("%");

    //温度、湿度をAmbientに送信する
    ambient.set(1, String(result.t).c_str());
    ambient.set(2, String(result.rh).c_str());
    ambient.send();

    digitalWrite(BLUE_LED, HIGH);
    prev += sendInterval;
  }
}
