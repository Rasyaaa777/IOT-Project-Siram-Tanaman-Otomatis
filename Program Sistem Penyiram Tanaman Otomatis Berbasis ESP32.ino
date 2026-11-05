// Sistem Penyiram Tanaman Otomatis Berbasis ESP32-C3 Super Mini
// Khusus Layar OLED 0.96" Dual Color (Zona Kuning Atas 16px, Zona Biru Bawah 48px)
// Fitur:
// 1. Zona Kuning (Y: 0-15): Header Jam Internet (WIB) & Status Mode [AUTO/MANUAL]
// 2. Garis Pemisah (Y: 15): Batas fisik antara warna kuning & biru
// 3. Zona Biru (Y: 18-63): Nilai Kelembapan, Indikator Bar Grafis, Status Tanah & Pompa
// 4. Kontrol Blynk: V1 (Mode), V2 (Manual Pompa), V3 (ADC), V7 (Persen), V8 (LED Indikator)
// 5. Jadwal Otomatis Internet: 07:00 Pagi & 17:00 Sore

#define BLYNK_TEMPLATE_ID "TMPL6BgWagWjB"
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "7yYsPh7ayIHAAdw6PCBOwXwMRQyvv711"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>

// ================= KONFIGURASI OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ================= KREDENSIAL WIFI =================
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "punya saya";
char pass[] = "12345678";

// ================= KONFIGURASI WAKTU NTP (WIB = GMT+7) =================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;     // UTC +7 (WIB)
const int   daylightOffset_sec = 0;

// ================= KONFIGURASI PIN =================
const int relay = 7;         // Relay Pompa di GPIO7
const int soilmoisture = 2;  // AOut Sensor Tanah di GPIO2

// ================= VARIABEL SISTEM =================
BlynkTimer timer;
float persen = 0.0;
bool isAutoMode = true;       // true = AUTO, false = MANUAL (dikontrol dari Blynk V1)
bool manualPumpState = false; // Status pompa saat mode MANUAL (dikontrol dari Blynk V2)
bool isPumping = false;       // Status relay fisik saat ini

// Jadwal Siram Otomatis (Format 24 jam)
const int JADWAL_PAGI_JAM = 7;      // 07:00 Pagi
const int JADWAL_SORE_JAM = 17;     // 17:00 Sore
const int DURASI_SIRAM_DETIK = 10;   // Durasi menyiram sesuai jadwal (detik)

// ================= KONTROL RELAY =================
// Catatan: Modul relay umumnya Active LOW (LOW = ON, HIGH = OFF)
void setPump(bool state) {
  isPumping = state;
  if (state) {
    digitalWrite(relay, LOW);  // Pompa NYALA
  } else {
    digitalWrite(relay, HIGH); // Pompa MATI
  }
  
  // Update indikator LED di aplikasi Blynk
  WidgetLED relayLed(V8);
  if (state) relayLed.on(); else relayLed.off();
}

// ================= HANDLER BLYNK =================
// V1: Switch Mode (1 = AUTO, 0 = MANUAL)
BLYNK_WRITE(V1) {
  isAutoMode = (param.asInt() == 1);
  Serial.print("Mode diubah ke: ");
  Serial.println(isAutoMode ? "AUTO" : "MANUAL");
  
  if (!isAutoMode && !manualPumpState) {
    setPump(false);
  }
}

// V2: Tombol Manual Pompa (1 = ON, 0 = OFF)
BLYNK_WRITE(V2) {
  manualPumpState = (param.asInt() == 1);
  if (!isAutoMode) {
    setPump(manualPumpState);
    Serial.print("Manual Pompa: ");
    Serial.println(manualPumpState ? "ON" : "OFF");
  }
}

// Sync status saat Blynk terhubung
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1);
  Blynk.syncVirtual(V2);
}

// ================= LOOP KONTROL & UPDATE TAMPILAN OLED =================
void processSystem() {
  // 1. Baca Sensor Kelembapan Tanah (ADC 12-bit: 0 - 4095)
  float soil = analogRead(soilmoisture);
  persen = (soil / 4095.0) * 100.0;

  // Kirim data ke Blynk
  Blynk.virtualWrite(V3, soil);
  Blynk.virtualWrite(V7, persen);

  // 2. Baca Jam Saat Ini dari NTP Internet
  struct tm timeinfo;
  char timeStr[9] = "--:--:--";
  int currentHour = -1;
  int currentMin = -1;
  int currentSec = -1;

  if (getLocalTime(&timeinfo)) {
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    currentHour = timeinfo.tm_hour;
    currentMin  = timeinfo.tm_min;
    currentSec  = timeinfo.tm_sec;
  }

  // 3. Logika Penyiraman Otomatis / Manual
  if (isAutoMode) {
    // A. Cek Jadwal Waktu (Pagi 07:00 atau Sore 17:00 di menit ke-0)
    bool isScheduleTime = (currentHour == JADWAL_PAGI_JAM || currentHour == JADWAL_SORE_JAM) && (currentMin == 0);
    
    if (isScheduleTime && currentSec < DURASI_SIRAM_DETIK) {
      setPump(true);
    } 
    // B. Cek Sensor Kelembapan Tanah (Jika tanah kering > 50%)
    else if (persen > 50.0) {
      setPump(true);
    } 
    // C. Jika tanah cukup lembap dan bukan waktu jadwal
    else {
      setPump(false);
    }
  } else {
    // Mode MANUAL: Ikuti tombol Blynk V2
    setPump(manualPumpState);
  }

  // 4. Update Tampilan Layar OLED (Khusus Dual-Color Kuning-Biru)
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);

  // ---------------------------------------------------------
  // [ZONA KUNING - Y: 0 s/d 14]
  // Baris status atas: Jam WIB & Mode
  // ---------------------------------------------------------
  oled.setTextSize(1);
  oled.setCursor(2, 4);
  oled.print(timeStr);

  oled.setCursor(76, 4);
  if (isAutoMode) {
    oled.print("[ AUTO ]");
  } else {
    oled.print("[MANUAL]");
  }

  // Garis pemisah tepat di batas fisik warna kuning & biru (Y = 15)
  oled.drawFastHLine(0, 15, 128, SSD1306_WHITE);

  // ---------------------------------------------------------
  // [ZONA BIRU - Y: 18 s/d 63]
  // Bagian utama monitoring tanaman & pompa
  // ---------------------------------------------------------
  
  // Baris 1 Zona Biru: Status Tanah & Persentase
  oled.setCursor(2, 20);
  oled.print("Tanah: ");
  if (persen > 50.0) {
    oled.print("KERING (");
  } else {
    oled.print("LEMBAP (");
  }
  oled.print(persen, 0);
  oled.print("%)");

  // Baris 2 Zona Biru: Progress Bar Indikator Kelembapan Tanah
  // Frame kotak bar: X=2, Y=31, Lebar=124, Tinggi=7
  oled.drawRect(2, 31, 124, 7, SSD1306_WHITE);
  int barWidth = (int)((persen / 100.0) * 120);
  if (barWidth > 120) barWidth = 120;
  if (barWidth < 0) barWidth = 0;
  oled.fillRect(4, 33, barWidth, 3, SSD1306_WHITE);

  // Baris 3 Zona Biru: Status Pompa & Jadwal
  oled.setCursor(2, 43);
  oled.print("Pompa: ");
  if (isPumping) {
    oled.print(">> NYALA <<");
  } else {
    oled.print("MATI");
  }

  // Baris 4 Zona Biru: Info Jadwal Siram
  oled.setCursor(2, 54);
  oled.print("Jadwal: 07:00 & 17:00");

  oled.display();
}

void setup() {
  Serial.begin(115200);

  // Setup Relay
  pinMode(relay, OUTPUT);
  setPump(false); // Pastikan pompa mati saat awal menyala

  // Inisialisasi I2C ESP32-C3 Super Mini (SDA=8, SCL=9)
  Wire.begin(8, 9);

  // Inisialisasi Layar OLED
  if (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("Gagal mendeteksi OLED SSD1306"));
  } else {
    oled.clearDisplay();
    oled.setTextColor(SSD1306_WHITE);
    
    // Header Kuning pada splash screen
    oled.setTextSize(1);
    oled.setCursor(8, 4);
    oled.println("SMART PLANT WATERING");
    oled.drawFastHLine(0, 15, 128, SSD1306_WHITE);
    
    // Isi Biru pada splash screen
    oled.setCursor(15, 26);
    oled.println("ESP32-C3 Supermini");
    oled.setCursor(15, 40);
    oled.println("Koneksi WiFi...");
    oled.setCursor(15, 52);
    oled.println("Sinkronisasi Jam...");
    oled.display();
  }

  // Koneksi WiFi & Blynk
  Blynk.begin(auth, ssid, pass);

  // Konfigurasi Jam Internet (NTP) WIB
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Timer: Eksekusi pemrosesan sensor & layar tiap 1 detik
  timer.setInterval(1000L, processSystem);
}

void loop() {
  Blynk.run();
  timer.run();
}
