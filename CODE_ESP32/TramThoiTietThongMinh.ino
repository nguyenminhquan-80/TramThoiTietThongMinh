#include <Wire.h>
#include <Adafruit_AHTX0.h>
#include <BH1750.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// ========== CẤU HÌNH WiFi ==========
// ⚠️ QUAN TRỌNG: Nhập thông tin WiFi của bạn vào đây
#define WIFI_SSID "your_wifi_ssid"        // Thay bằng tên WiFi
#define WIFI_PASSWORD "your_wifi_password" // Thay bằng mật khẩu WiFi

// ========== CẤU HÌNH FIREBASE ==========
// ⚠️ QUAN TRỌNG: Nhập thông tin Firebase của bạn vào đây
// Hướng dẫn: https://console.firebase.google.com/
#define FIREBASE_HOST "your_project.firebaseio.com"  // Thay bằng URL Firebase
#define FIREBASE_AUTH ""  // Để trống nếu đang ở chế độ test mode

// ========== CẤU HÌNH CHÂN ==========
#define TFT_CS   6
#define TFT_DC   7
#define TFT_RST  15
#define TFT_MOSI 16
#define TFT_SCLK 17
#define PIN_LED  18    // Đèn (Relay NO)
#define PIN_FAN  8     // Quạt (Relay NO)
#define PIN_LED_Capnhat  10  // Nút cập nhật dữ liệu

// ========== LED RGB ==========
#define PIN_LED_R  3
#define PIN_LED_G  46
#define PIN_LED_B  9

// ========== THAM SỐ THỜI GIAN ==========
const unsigned long SENSOR_READ_INTERVAL = 2000;
const unsigned long TFT_UPDATE_INTERVAL = 2000;
const unsigned long FIREBASE_SEND_INTERVAL = 2000;
const unsigned long CONTROL_READ_INTERVAL = 500;
const unsigned long BLINK_INTERVAL = 500;

// ========== BIẾN TOÀN CỤC ==========
bool ledState = false;     // false = TẮT (Relay NO: LOW = HỞ)
bool fanState = false;     // false = TẮT (Relay NO: LOW = HỞ)
bool hasError = false;
bool isConnecting = false;
bool blinkState = false;
unsigned long lastBlinkTime = 0;
unsigned long lastControlRead = 0;
unsigned long lastSensorRead = 0;
unsigned long lastTFTUpdate = 0;
unsigned long lastFirebaseSend = 0;

// ========== BIẾN CHO NÚT CẬP NHẬT ==========
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;

// ========== BIẾN LƯU DỮ LIỆU CẢM BIẾN ==========
float nhietDoHienTai = 0;
float doAmHienTai = 0;
float anhSangHienTai = 0;
int trangThaiNhiet = 1;
int trangThaiDoAm = 1;
int trangThaiAS = 1;
int trangThaiThoiTiet = 1;

// ========== CẢM BIẾN ==========
Adafruit_AHTX0 aht;
BH1750 lightMeter;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// ========== ẢNH (giữ nguyên) ==========
#include "img_001_NhietdoCao.h"
#include "img_002_NhietdoBT.h"
#include "img_003_NhietdoThap.h"
#include "img_004_DoamCao.h"
#include "img_005_DoamBT.h"
#include "img_006_DoamThap.h"
#include "img_007_CuongdoASCao.h"
#include "img_008_CuongdoASTB.h"
#include "img_009_CuongdoASThap.h"
#include "img_010_ThoitietTot.h"
#include "img_011_ThoitietBT.h"
#include "img_012_ThoitietXau.h"
#include "img_013_Bangthoitiet.h"
#include "img_014_Muaroi.h"
#include "img_015_Nongrat.h"

// ========== Firebase ==========
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ========== NGƯỠNG ==========
const float NHIET_CAO = 27.0;
const float NHIET_THAP = 18.0;
const float DOAM_CAO = 75.0;
const float DOAM_THAP = 40.0;
const float AS_CAO = 300.0;
const float AS_THAP = 75.0;

// ========== HỆ SỐ HIỆU CHỈNH ==========
#define NHIET_TRU 4.6
#define CALIB_NHIET 1.0
#define CALIB_DOAM 1.0

// ========== HIỆU CHỈNH ÁNH SÁNG ==========
// Độ lệch trung bình 0.5228 lux -> cộng thêm 0.52 vào giá trị đọc được
#define CALIB_AS_OFFSET 0.52
#define CALIB_AS 1.0

// ========== HÀM ĐÁNH GIÁ ==========
int danhGiaNhietDo(float t) { 
  if (t > NHIET_CAO) return 2; 
  if (t < NHIET_THAP) return 0; 
  return 1; 
}

int danhGiaDoAm(float h) { 
  if (h > DOAM_CAO) return 2; 
  if (h < DOAM_THAP) return 0; 
  return 1; 
}

int danhGiaAnhSang(float lux) { 
  if (lux > AS_CAO) return 2; 
  if (lux < AS_THAP) return 0; 
  return 1; 
}

int danhGiaThoiTiet(int nhiet, int am, int as) {
  int demXau = 0;
  if (nhiet == 0 || nhiet == 2) demXau++;
  if (am == 0 || am == 2) demXau++;
  if (as == 0 || as == 2) demXau++;
  if (demXau >= 2) return 2;
  if (demXau == 1) return 1;
  return 0;
}

uint16_t chonMauNhietDo(int tt) { 
  if (tt == 0) return ST77XX_MAGENTA; 
  if (tt == 2) return ST77XX_RED; 
  return ST77XX_BLUE; 
}

uint16_t chonMauDoAm(int tt) { 
  if (tt == 0) return ST77XX_MAGENTA; 
  if (tt == 2) return ST77XX_RED; 
  return ST77XX_BLUE; 
}

uint16_t chonMauAnhSang(int tt) { 
  if (tt == 0) return ST77XX_MAGENTA; 
  if (tt == 2) return ST77XX_RED; 
  return ST77XX_BLUE; 
}

// ========== LED RGB ==========
void tatLED_RGB() {
  digitalWrite(PIN_LED_R, LOW);
  digitalWrite(PIN_LED_G, LOW);
  digitalWrite(PIN_LED_B, LOW);
}

void setLED_Green() { tatLED_RGB(); digitalWrite(PIN_LED_G, HIGH); }
void setLED_Red() { tatLED_RGB(); digitalWrite(PIN_LED_R, HIGH); }
void setLED_Blue() { tatLED_RGB(); digitalWrite(PIN_LED_B, HIGH); }

void blinkLED_Blue() {
  if (millis() - lastBlinkTime >= BLINK_INTERVAL) {
    lastBlinkTime = millis();
    blinkState = !blinkState;
    digitalWrite(PIN_LED_B, blinkState ? HIGH : LOW);
  }
}

void updateLED_Status() {
  if (hasError) { setLED_Red(); return; }
  if (isConnecting) { blinkLED_Blue(); return; }
  setLED_Green();
}

// ========== HÀM ĐỌC CẢM BIẾN (ĐÃ HIỆU CHỈNH ÁNH SÁNG) ==========
void docCamBien() {
  sensors_event_t hum, temp;
  
  // Đọc AHT30
  if (aht.getEvent(&hum, &temp)) {
    nhietDoHienTai = temp.temperature * CALIB_NHIET - NHIET_TRU;
    doAmHienTai = hum.relative_humidity * CALIB_DOAM;
    
    if (doAmHienTai > 100) doAmHienTai = 100;
    if (doAmHienTai < 0) doAmHienTai = 0;
  } else {
    Serial.println("⚠️ Lỗi đọc AHT30");
  }
  
  // Đọc BH1750 + HIỆU CHỈNH OFFSET
  float rawLight = lightMeter.readLightLevel();
  anhSangHienTai = (rawLight * CALIB_AS) + CALIB_AS_OFFSET;
  if (isnan(anhSangHienTai) || anhSangHienTai < 0) {
    anhSangHienTai = 0;
  }
  
  // Cập nhật trạng thái đánh giá
  trangThaiNhiet = danhGiaNhietDo(nhietDoHienTai);
  trangThaiDoAm = danhGiaDoAm(doAmHienTai);
  trangThaiAS = danhGiaAnhSang(anhSangHienTai);
  trangThaiThoiTiet = danhGiaThoiTiet(trangThaiNhiet, trangThaiDoAm, trangThaiAS);
  
  Serial.print("📊 Cảm biến: N=");
  Serial.print(nhietDoHienTai, 1);
  Serial.print("°C, ĐA=");
  Serial.print(doAmHienTai, 1);
  Serial.print("%, AS=");
  Serial.print(anhSangHienTai, 1);
  Serial.print(" (gốc: ");
  Serial.print(rawLight, 1);
  Serial.println(" lux)");
}

// ========== CẬP NHẬT TRẠNG THÁI THIẾT BỊ (Relay NO) ==========
void capNhatThietBi() {
  // Relay NO: HIGH = BẬT, LOW = TẮT
  digitalWrite(PIN_LED, ledState ? HIGH : LOW);
  digitalWrite(PIN_FAN, fanState ? HIGH : LOW);
  
  Serial.print("🔌 Trạng thái thiết bị: Đèn = ");
  Serial.print(ledState ? "BẬT" : "TẮT");
  Serial.print(", Quạt = ");
  Serial.println(fanState ? "BẬT" : "TẮT");
}

// ========== GỬI TRẠNG THÁI LÊN FIREBASE ==========
void capNhatTrangThaiThietBiLenFirebase() {
  if (Firebase.RTDB.setInt(&fbdo, "/device/status/led", ledState ? 1 : 0)) {
    Serial.print("📱 Đèn (FB): ");
    Serial.println(ledState ? "BẬT" : "TẮT");
  }
  
  delay(50);
  
  if (Firebase.RTDB.setInt(&fbdo, "/device/status/fan", fanState ? 1 : 0)) {
    Serial.print("📱 Quạt (FB): ");
    Serial.println(fanState ? "BẬT" : "TẮT");
  }
}

// ========== KHỞI TẠO TRẠNG THÁI MẶC ĐỊNH TRÊN FIREBASE ==========
void khoiTaoFirebaseControl() {
  // Khởi tạo với led=0 (TẮT), fan=0 (TẮT) cho Relay NO
  FirebaseJson controlInit;
  controlInit.set("led", 0);  // 0 = TẮT
  controlInit.set("fan", 0);  // 0 = TẮT
  
  FirebaseJson statusInit;
  statusInit.set("led", 0);
  statusInit.set("fan", 0);
  
  Firebase.RTDB.setJSON(&fbdo, "/device/control", &controlInit);
  Firebase.RTDB.setJSON(&fbdo, "/device/status", &statusInit);
  
  Serial.println("📱 Firebase: Đã khởi tạo control với trạng thái TẮT (0)");
}

// ========== ĐỌC LỆNH TỪ FIREBASE (Relay NO) ==========
void docLenhDieuKhien() {
  bool coThayDoi = false;
  
  // Đọc lệnh đèn
  if (Firebase.RTDB.get(&fbdo, "/device/control/led")) {
    int newLedState = 0;
    if (fbdo.dataType() == "int") newLedState = fbdo.intData();
    else if (fbdo.dataType() == "string") newLedState = fbdo.stringData().toInt();
    else if (fbdo.dataType() == "float") newLedState = (int)fbdo.floatData();
    
    // newLedState từ App: 1 = BẬT, 0 = TẮT
    if (newLedState == 1 && !ledState) {
      ledState = true;
      coThayDoi = true;
      Serial.println("💡 ĐÈN ĐÃ BẬT (từ điện thoại)");
    } 
    else if (newLedState == 0 && ledState) {
      ledState = false;
      coThayDoi = true;
      Serial.println("💡 ĐÈN ĐÃ TẮT (từ điện thoại)");
    }
  }
  
  // Đọc lệnh quạt
  if (Firebase.RTDB.get(&fbdo, "/device/control/fan")) {
    int newFanState = 0;
    if (fbdo.dataType() == "int") newFanState = fbdo.intData();
    else if (fbdo.dataType() == "string") newFanState = fbdo.stringData().toInt();
    else if (fbdo.dataType() == "float") newFanState = (int)fbdo.floatData();
    
    if (newFanState == 1 && !fanState) {
      fanState = true;
      coThayDoi = true;
      Serial.println("🌀 QUẠT ĐÃ BẬT (từ điện thoại)");
    } 
    else if (newFanState == 0 && fanState) {
      fanState = false;
      coThayDoi = true;
      Serial.println("🌀 QUẠT ĐÃ TẮT (từ điện thoại)");
    }
  }
  
  if (coThayDoi) {
    capNhatThietBi();
    capNhatTrangThaiThietBiLenFirebase();
  }
}

// ========== HÀM CẬP NHẬT DỮ LIỆU NGAY TỨC THÌ ==========
void capNhatNgayLapTuc() {
  Serial.println("🔘 Nút cập nhật được nhấn! Đang cập nhật dữ liệu ngay...");
  docCamBien();
  hienThi();
  lastTFTUpdate = millis();
  
  if (WiFi.status() == WL_CONNECTED) {
    guiLenFirebase();
    lastFirebaseSend = millis();
  }
  
  Serial.println("✅ Đã cập nhật dữ liệu ngay tức thì!");
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(80, 10);
  tft.fillRect(80, 10, 100, 15, ST77XX_BLACK);
  tft.print("Da cap nhat!");
  delay(500);
  tft.fillRect(80, 10, 100, 15, ST77XX_BLACK);
}

// ========== KIỂM TRA NÚT NHẤN ==========
void kiemTraNutCapNhat() {
  if (digitalRead(PIN_LED_Capnhat) == LOW) {
    if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
      lastButtonPress = millis();
      capNhatNgayLapTuc();
    }
  }
}

// ========== GỬI DỮ LIỆU LÊN FIREBASE ==========
void guiLenFirebase() {
  isConnecting = true;
  
  FirebaseJson json;
  json.set("Nhietdo", nhietDoHienTai);
  json.set("Doam", doAmHienTai);
  json.set("Anhsang", anhSangHienTai);
  json.set("Trangthai", trangThaiThoiTiet);
  json.set("timestamp", millis());
  
  if (Firebase.RTDB.setJSON(&fbdo, "/sensor_data/latest", &json)) {
    Serial.print("🔥 Firebase đã cập nhật: N=");
    Serial.print(nhietDoHienTai, 1);
    Serial.print(", ĐA=");
    Serial.print(doAmHienTai, 1);
    Serial.print(", AS=");
    Serial.println(anhSangHienTai, 1);
    
    String historyPath = "/sensor_data/history/" + String(millis());
    Firebase.RTDB.setJSON(&fbdo, historyPath, &json);
  } else {
    Serial.print("❌ Lỗi gửi Firebase: "); 
    Serial.println(fbdo.errorReason());
  }
  
  isConnecting = false;
}

// ========== HIỂN THỊ MÀN HÌNH TFT ==========
void hienThi() {
  if (trangThaiThoiTiet == 2 && trangThaiNhiet == 0) 
    tft.drawRGBBitmap(0, 0, (uint16_t*)Nen_Muaroi, 240, 320);
  else if (trangThaiNhiet == 2 && trangThaiAS == 2) 
    tft.drawRGBBitmap(0, 0, (uint16_t*)Nen_Nongrat, 240, 320);
  else 
    tft.drawRGBBitmap(0, 0, (uint16_t*)Bangthoitiet, 239, 320);
  
  if (trangThaiThoiTiet == 0) 
    tft.drawRGBBitmap(0, 290, (uint16_t*)ThoitietTot, 240, 30);
  else if (trangThaiThoiTiet == 1) 
    tft.drawRGBBitmap(0, 290, (uint16_t*)ThoitietBT, 240, 30);
  else 
    tft.drawRGBBitmap(0, 290, (uint16_t*)ThoitietXau, 240, 30);
  
  if (trangThaiNhiet == 0) 
    tft.drawRGBBitmap(20, 60, (uint16_t*)NhietdoThap, 64, 64);
  else if (trangThaiNhiet == 1) 
    tft.drawRGBBitmap(20, 60, (uint16_t*)NhietdoBT, 64, 64);
  else 
    tft.drawRGBBitmap(20, 60, (uint16_t*)NhietdoCao, 64, 64);
  
  if (trangThaiDoAm == 0) 
    tft.drawRGBBitmap(20, 140, (uint16_t*)DoamThap, 64, 64);
  else if (trangThaiDoAm == 1) 
    tft.drawRGBBitmap(20, 140, (uint16_t*)DoamBT, 64, 64);
  else 
    tft.drawRGBBitmap(20, 140, (uint16_t*)DoamCao, 64, 64);
  
  if (trangThaiAS == 0) 
    tft.drawRGBBitmap(20, 220, (uint16_t*)CuongdoASThap, 64, 64);
  else if (trangThaiAS == 1) 
    tft.drawRGBBitmap(20, 220, (uint16_t*)CuongdoASBT, 64, 64);
  else 
    tft.drawRGBBitmap(20, 220, (uint16_t*)CuongdoASCao, 64, 64);
  
  tft.setTextSize(4);
  tft.setTextColor(chonMauNhietDo(trangThaiNhiet)); 
  tft.setCursor(95, 85); 
  tft.print(nhietDoHienTai, 1);
  
  tft.setTextColor(chonMauDoAm(trangThaiDoAm)); 
  tft.setCursor(95, 165); 
  tft.print(doAmHienTai, 1);
  
  tft.setTextColor(chonMauAnhSang(trangThaiAS)); 
  tft.setCursor(95, 245);
  if (anhSangHienTai >= 1000) 
    tft.print(anhSangHienTai, 0); 
  else 
    tft.print(anhSangHienTai, 1);
}

// ========== KIỂM TRA LỖI ==========
void kiemTraLoiHeThong() {
  if (WiFi.status() != WL_CONNECTED) {
    if (!hasError) { 
      hasError = true; 
      Serial.println("🔴 LỖI: Mất WiFi!"); 
    }
    return;
  }
  
  if (nhietDoHienTai == 0 && doAmHienTai == 0 && anhSangHienTai == 0) {
    if (!hasError) { 
      hasError = true; 
      Serial.println("🔴 LỖI: Cảm biến không hoạt động!"); 
    }
    return;
  }
  
  if (hasError) { 
    hasError = false; 
    Serial.println("✅ Hệ thống hoạt động trở lại!"); 
  }
}

// ========== HIỂN THỊ KHỞI ĐỘNG ==========
void hienThiThongBaoKhoiDong() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  int yPos = 30, lineHeight = 25;
  
  tft.setCursor(10, yPos); tft.println("Khoi tao he thong..."); yPos += lineHeight; delay(500);
  
  tft.setCursor(10, yPos); tft.print("AHT30 ");
  if (aht.begin()) { 
    tft.setTextColor(ST77XX_GREEN); tft.println("OK"); 
  } else { 
    tft.setTextColor(ST77XX_RED); tft.println("FAIL"); 
    while (1) delay(10);
  }
  yPos += lineHeight; delay(500);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, yPos); tft.print("BH1750 ");
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) { 
    tft.setTextColor(ST77XX_GREEN); tft.println("OK"); 
    delay(200);
  } else { 
    tft.setTextColor(ST77XX_RED); tft.println("FAIL"); 
    while (1) delay(10);
  }
  yPos += lineHeight; delay(500);
  
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, yPos); tft.println("Dang ket noi WiFi...."); yPos += lineHeight;
  
  isConnecting = true;
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(250); attempts++;
    tft.setCursor(10, yPos); tft.print(".");
    if (attempts % 20 == 0) {
      tft.fillRect(10, yPos, 200, 20, ST77XX_BLACK);
      tft.setCursor(10, yPos); tft.print("Dang ket noi WiFi....");
    }
    blinkLED_Blue();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(10, yPos); tft.println("WiFi da ket noi!"); yPos += lineHeight;
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(10, yPos); tft.print("IP: "); tft.println(WiFi.localIP()); yPos += lineHeight;
  } else {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(10, yPos); tft.println("WiFi ket noi that bai!"); yPos += lineHeight;
    hasError = true;
  }
  delay(1000);
  isConnecting = false;
  
  tft.setTextColor(ST77XX_GREEN);
  tft.setCursor(10, yPos); tft.println("He thong da san sang!");
  delay(2000);
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=========================================");
  Serial.println("Khởi tạo hệ thống giám sát thông minh...");
  Serial.println("=========================================");
  
  // Cấu hình chân
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  pinMode(PIN_LED_Capnhat, INPUT_PULLUP);
  
  // ========== QUAN TRỌNG: Khởi tạo trạng thái TẮT cho Relay NO ==========
  ledState = false;   // ĐÈN TẮT khi khởi động
  fanState = false;   // QUẠT TẮT khi khởi động
  
  digitalWrite(PIN_LED, LOW);   // LOW = TẮT (Relay NO)
  digitalWrite(PIN_FAN, LOW);   // LOW = TẮT (Relay NO)
  tatLED_RGB();
  
  Serial.println("🔌 Khởi động: ĐÈN và QUẠT ở trạng thái TẮT (Relay NO)");
  
  // Khởi tạo màn hình
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(0);
  
  // Khởi tạo I2C
  Wire.begin(4, 5);
  Wire.setClock(400000);
  
  hienThiThongBaoKhoiDong();
  docCamBien();
  
  // Cấu hình Firebase
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Firebase đã khởi tạo!");
  delay(1000);
  
  // Khởi tạo control trên Firebase với led=0, fan=0 (TẮT)
  khoiTaoFirebaseControl();
  
  delay(500);
  
  guiLenFirebase();
  hienThi();
  
  Serial.println("=========================================");
  Serial.println("✅ HỆ THỐNG ĐÃ SẴN SÀNG!");
  Serial.println("🔘 Nhấn nút chân 10 để cập nhật dữ liệu ngay lập tức");
  Serial.println("=========================================");
}

// ========== LOOP ==========
void loop() {
  unsigned long currentMillis = millis();
  
  kiemTraNutCapNhat();
  
  if (currentMillis - lastSensorRead >= SENSOR_READ_INTERVAL) {
    docCamBien();
    lastSensorRead = currentMillis;
  }
  
  if (currentMillis - lastTFTUpdate >= TFT_UPDATE_INTERVAL) {
    hienThi();
    lastTFTUpdate = currentMillis;
  }
  
  if (currentMillis - lastFirebaseSend >= FIREBASE_SEND_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
      guiLenFirebase();
    }
    lastFirebaseSend = currentMillis;
  }
  
  if (currentMillis - lastControlRead >= CONTROL_READ_INTERVAL) {
    if (WiFi.status() == WL_CONNECTED) {
      docLenhDieuKhien();
    }
    lastControlRead = currentMillis;
  }
  
  kiemTraLoiHeThong();
  updateLED_Status();
  
  delay(10);
}
