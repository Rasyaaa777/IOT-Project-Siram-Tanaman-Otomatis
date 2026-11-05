# Analisis Teknis: Sistem Penyiram Tanaman Otomatis Berbasis ESP32-C3 Super Mini

Dokumen ini berisi bedah teknis menyeluruh terhadap kode program, arsitektur perangkat keras, topologi komunikasi, serta daftar kebutuhan sistem penyiram tanaman pintar berbasis IoT.

---

## 1. Cara Kerja Sistem (System Workflow)

Sistem ini bekerja secara *real-time* dan *non-blocking* menggunakan penjadwalan berbasis `BlynkTimer` yang dieksekusi setiap $1000\text{ ms}$ ($1\text{ detik}$). Berikut adalah alur kerja sistem dari awal menyala hingga beroperasi penuh:

### A. Tahap Inisialisasi (`setup()`)
1. **Inisialisasi Serial & GPIO:** Membuka komunikasi serial pada *baudrate* $115200\text{ bps}$ untuk *debugging*, serta mengatur pin `GPIO 7` (Relay) sebagai `OUTPUT` dengan kondisi awal `HIGH` (Pompa Mati, karena modul relay bersifat *Active LOW*).
2. **Inisialisasi Bus $\text{I}^2\text{C}$ & Layar OLED:** Mengaktifkan komunikasi $\text{I}^2\text{C}$ pada pin spesifik ESP32-C3 Super Mini (`SDA = GPIO 8`, `SCL = GPIO 9`) menuju alamat heksadesimal `0x3C`. Layar menampilkan *Splash Screen* status koneksi.
3. **Koneksi Jaringan & Cloud:** Menghubungkan ESP32-C3 ke jaringan WiFi (`Redmi Note 9 Pro`) dan melakukan autentikasi ke server **Blynk Cloud**. Saat terhubung (`BLYNK_CONNECTED()`), mikrokontroler menyinkronkan status terakhir dari pin virtual `V1` (Mode) dan `V2` (Saklar Manual).
4. **Sinkronisasi Waktu Internet (NTP):** Mengambil waktu presisi dari server `pool.ntp.org` dengan kompensasi zona waktu WIB ($\text{GMT}+7$ atau $7 \times 3600\text{ detik}$).

### B. Tahap Akuisisi Data & Telemetri (`processSystem()`)
Setiap $1\text{ detik}$, fungsi `processSystem()` menjalankan langkah berikut:
1. **Pembacaan ADC Sensor Tanah:** Membaca tegangan analog pada `GPIO 2` menggunakan resolusi ADC $12\text{-bit}$ ($0\text{ s/d }4095$).
2. **Konversi Nilai Persentase:** Menghitung nilai persentase menggunakan rumus:
   $$\text{Persen} = \left(\frac{\text{Nilai ADC}}{4095.0}\right) \times 100\%$$
   > **Catatan Karakteristik Sensor:** Pada sensor kelembapan tanah analog standar, semakin **kering** tanah maka hambatan listrik semakin besar sehingga nilai tegangan/ADC mendekati $4095$ ($> 50\%$). Sebaliknya, semakin **lembap/basah** tanah maka nilai ADC menurun ($\le 50\%$). Oleh karena itu, di dalam kode nilai `persen` merepresentasikan **Persentase Kekeringan Tanah**.
3. **Pengiriman Data ke Blynk:** Mengirimkan nilai mentah ADC ke `V3` dan nilai persentase ke `V7`.
4. **Pembacaan Waktu Lokal:** Mengambil jam, menit, dan detik terkini dari RTC internal ESP32 yang telah disinkronkan dengan NTP.

### C. Logika Pengambilan Keputusan (Mode AUTO vs MANUAL)
* **Jika Mode AUTO Aktif (`V1 == 1`):**
  Sistem menggabungkan kontrol berbasis waktu (*Time-based*) dan berbasis sensor (*Sensor-based*) dengan logika **OR**:
  1. **Pemicu Jadwal Harian:** Jika waktu menunjukkan tepat pukul `07:00:00` s/d `07:00:09` (Pagi) **ATAU** `17:00:00` s/d `17:00:09` (Sore), pompa akan **MENYALA** selama $10\text{ detik}$ (`DURASI_SIRAM_DETIK = 10`) tanpa memedulikan kelembapan tanah.
  2. **Pemicu Kondisi Tanah:** Di luar jam jadwal, jika nilai pembacaan sensor $> 50.0\%$ (Tanah Kering), maka pompa akan **MENYALA** terus-menerus hingga tanah basah dan nilai sensor turun menjadi $\le 50.0\%$.
  3. **Kondisi Siaga (Idle):** Jika bukan jam jadwal dan nilai sensor $\le 50.0\%$ (Tanah Lembap), pompa **MATI**.
* **Jika Mode MANUAL Aktif (`V1 == 0`):**
  Sistem mengabaikan jadwal waktu dan sensor tanah. Status pompa sepenuhnya mengikuti instruksi tombol pada aplikasi Blynk di pin virtual `V2` (`1` = NYALA, `0` = MATI). Setiap perubahan status pompa akan memperbarui indikator `WidgetLED` pada pin virtual `V8`.

### D. Visualisasi Antarmuka OLED Dual-Color
Layar OLED $0.96\text{ inci}$ ($128 \times 64\text{ piksel}$) dibagi menjadi dua area fisik warna agar tidak terjadi perpotongan warna (*color bleeding*):
* **Zona Kuning ($Y = 0\text{ s/d }14$):** Menampilkan jam internet (`HH:MM:SS`) di sisi kiri dan indikator mode (`[ AUTO ]` atau `[MANUAL]`) di sisi kanan.
* **Garis Pembatas ($Y = 15$):** Garis horizontal sepanjang $128\text{ piksel}$ tepat pada perbatasan matriks piksel kuning dan biru.
* **Zona Biru ($Y = 18\text{ s/d }63$):** Menampilkan teks status tanah (`KERING` / `LEMBAP` beserta persentase), *progress bar* grafis berdimensi $124 \times 7\text{ piksel}$, status aktuator pompa (`>> NYALA <<` / `MATI`), dan informasi jadwal statis (`07:00 & 17:00`).

---

## 2. Skema Fisik & Pemetaan Pin (Physical Schema)

### A. Tabel Pemetaan Pin (Pinout Mapping)

| Komponen | Pin Komponen | Pin ESP32-C3 Super Mini | Jenis Sinyal / Antarmuka | Keterangan |
| :--- | :--- | :--- | :--- | :--- |
| **OLED 0.96" SSD1306** | VCC | 3.3V | Power | Tegangan kerja $3.3\text{ V}$ |
| | GND | GND | Ground | Ground bersama |
| | SDA | **GPIO 8** | $\text{I}^2\text{C}$ Data | Komunikasi data layar |
| | SCL | **GPIO 9** | $\text{I}^2\text{C}$ Clock | Sinyal detak layar |
| **Sensor Kelembapan Tanah** | VCC | 3.3V | Power | Agar rentang ADC sesuai ($0 - 3.3\text{ V}$) |
| | GND | GND | Ground | Ground bersama |
| | AOUT | **GPIO 2** | Analog Input (ADC1_CH2) | Pembacaan analog $12\text{-bit}$ |
| **Modul Relay 1-Channel** | VCC / DC+ | 5V | Power | Tegangan koil relay $5\text{ V}$ |
| | GND / DC- | GND | Ground | Ground bersama |
| | IN | **GPIO 7** | Digital Output | Pemicu *Active LOW* |
| **Pompa Air DC Mini** | Kabel Positif (+) | Jalur `NO` Relay | Daya Eksternal | Terhubung ke kontak *Normally Open* |
| | Kabel Negatif (-) | GND Adaptor Eksternal | Ground | Terhubung langsung ke minus catu daya |

### B. Diagram Pengawatan Fisik (Wiring Diagram)

```text
                        +-----------------------------------+
                        |       CATU DAYA / USB 5V          |
                        +-----------------+-----------------+
                                          |
                 +------------------------+------------------------+
                 | (Jalur 5V)                                      | (Jalur 5V Eksternal)
                 v                                                 v
      +-----------------------+                         +---------------------+
      | ESP32-C3 SUPER MINI   |                         |  MODUL RELAY 1-CH   |
      |                       |                         |      (Active LOW)   |
      |                  5V   |------------------------>| VCC                 |
      |                  GND  |---+-------------------->| GND                 |
      |                       |   |                     |                     |
      |                GPIO 7 |---|-------------------->| IN         COM  NO  |
      |                       |   |                     +-------------+----+--+
      |                GPIO 8 |---|----------+                        |    |
      |                GPIO 9 |---|--------+ |   +--------------------+    |
      |                       |   |        | |   | (+5V Adaptor)           |
      |                GPIO 2 |---|------+ | |   |                         v
      |                  3.3V |---+----+ | | |   |               +-------------------+
      +-----------------------+   |    | | | |   |               |   POMPA AIR DC    |
                                  |    | | | |   |               |    (3V - 5V)      |
         +------------------------+    | | | |   |               +---------+---------+
         |        +--------------------+ | | |   |                         |
         |        |                      | | |   +-------------------------+ (GND Adaptor)
         |        |   +------------------+ | |
         v        v   v                    | |
      +------------------+                 | |          +----------------------+
      |  MODUL SENSOR    |                 | |          |  OLED 0.96" SSD1306  |
      | KELEMBAPAN TANAH |                 | +--------->| SDA      (Alamat:    |
      |   (GND, VCC, A0) |                 +----------->| SCL        0x3C)     |
      +--------+---------+                 +----------->| VCC (3.3V)           |
               |                           |  +-------->| GND                  |
          [Probe Tanah]                    |  |         +----------------------+
                                           |  |
                                           +--+--> (Terhubung ke 3.3V & GND ESP32)
```

---

## 3. Topologi Sistem & Jaringan (System Topology)

Sistem menerapkan arsitektur **IoT 4-Layer** (Perception/Edge Layer, Controller Layer, Network Layer, dan Cloud/Application Layer).

```text
========================================================================================
                            TOPOLOGI SISTEM & JARINGAN IoT
========================================================================================

 [ LAYER 1: EDGE / FISIK ]        [ LAYER 2: CONTROLLER ]       [ LAYER 3: NETWORK ]
 
 +-----------------------+
 | Sensor Tanah (Analog) |---(ADC GPIO 2)---+
 +-----------------------+                  |
                                            v
 +-----------------------+        +-------------------+         +--------------------+
 | Layar OLED 0.96"      |<--(I2C GPIO 8,9)-|                   |         | WiFi Hotspot / AP  |
 | (UI Lokal Dual-Color) |        |  ESP32-C3 Super   |<=WiFi==>| "Redmi Note 9 Pro" |
 +-----------------------+        |       Mini        | 2.4 GHz |  (Gateway Internet)|
                                  |                   |         +---------+----------+
 +-----------------------+        +-------------------+                   |
 | Modul Relay + Pompa   |<--(DO GPIO 7)----+                             | (Internet / WAN)
 +-----------------------+                                                |
                                                                          v
========================================================================================
                           [ LAYER 4: CLOUD & APPLICATION ]
========================================================================================
                 |                                                |
        (UDP Port 123 / NTP)                             (TCP/IP Blynk Protocol)
                 |                                                |
                 v                                                v
      +---------------------+                          +---------------------+
      |     NTP Server      |                          |    Blynk IoT Cloud  |
      |   "pool.ntp.org"    |                          |   (Server Regional) |
      | (Sinkronisasi WIB)  |                          +----------+----------+
      +---------------------+                                     |
                                                   +--------------+--------------+
                                                   | Datastreams:                |
                                                   | - V1: Mode (Auto/Manual)    |
                                                   | - V2: Saklar Pompa Manual   |
                                                   | - V3: Nilai Mentah ADC      |
                                                   | - V7: Persentase Sensor (%) |
                                                   | - V8: Indikator LED Pompa   |
                                                   +--------------+--------------+
                                                                  |
                                                                  v
                                                       +---------------------+
                                                       | Smartphone Pengguna |
                                                       |  (Aplikasi Blynk)   |
                                                       +---------------------+
```

---

## 4. Kebutuhan Perangkat Keras & Perangkat Lunak

### A. Kebutuhan Perangkat Keras (Hardware Bill of Materials)
1. **Mikrokontroler:** ESP32-C3 Super Mini Development Board (Prosesor RISC-V 32-bit Single-Core @ $160\text{ MHz}$, WiFi $2.4\text{ GHz}$ IEEE 802.11 b/g/n, USB Type-C).
2. **Layar Tampilan:** Modul OLED $0.96\text{ inci}$ $\text{I}^2\text{C}$ $128 \times 64\text{ piksel}$ chip driver **SSD1306** varian *Dual-Color* (Kuning-Biru), 4-pin (`GND`, `VCC`, `SCL`, `SDA`).
3. **Sensor:** Modul Sensor Kelembapan Tanah (*Soil Moisture Sensor*) dengan keluaran analog (`AOUT`), bisa menggunakan tipe kapasitif v1.2 (tahan korosi) atau tipe resistif YL-69 + komparator LM393.
4. **Penggerak (Driver):** Modul Relay $1\text{-Channel}$ $5\text{ V}$ DC dengan pemicu *Active LOW* dan *optocoupler isolation*.
5. **Aktuator:** Pompa Air Celup Mini (*Mini Submersible Water Pump*) DC $3\text{ V} - 5\text{ V}$ beserta selang silikon diameter $6\text{ mm}$.
6. **Catu Daya & Aksesoris:**
   * Adaptor Daya USB $5\text{ V} / 2\text{ A}$ (Disarankan memisahkan jalur daya pompa dari regulator internal ESP32 untuk mencegah *voltage drop/reset*).
   * *Breadboard* atau PCB matriks, serta kabel *jumper* (Male-to-Male / Male-to-Female).

### B. Kebutuhan Perangkat Lunak (Software Requirements)
1. **Lingkungan Pengembangan (IDE):**
   * Arduino IDE (versi $1.8.19$ atau $2.x$) atau VS Code dengan ekstensi PlatformIO.
   * **Board Core:** `esp32` by Espressif Systems (diinstal melalui Arduino Boards Manager, memilih target board `ESP32C3 Dev Module` dengan opsi `USB CDC On Boot: Enabled` untuk Serial Monitor).
2. **Pustaka (Libraries):**
   * `WiFi.h`, `WiFiClient.h`, `Wire.h`, `time.h` (Pustaka bawaan paket ESP32 Core).
   * `Blynk` by Volodymyr Shymanskyy (`BlynkSimpleEsp32.h`).
   * `Adafruit GFX Library` by Adafruit (`Adafruit_GFX.h`).
   * `Adafruit SSD1306` by Adafruit (`Adafruit_SSD1306.h`).
3. **Layanan Cloud & Mobile:**
   * Akun dan konsol **Blynk IoT** (Template ID: `TMPLn-jQSvce`).
   * Aplikasi **Blynk IoT** pada perangkat Android atau iOS.