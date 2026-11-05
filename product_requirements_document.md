# Product Requirements Document (PRD)
## Sistem Penyiram Tanaman Otomatis Berbasis ESP32-C3 Super Mini & Blynk IoT

| Atribut Dokumen | Detail Informasi |
| :--- | :--- |
| **Nama Produk** | Monitoring & Penyiraman Tanaman Otomatis (Smart Plant Watering System) |
| **Versi Dokumen** | 1.0.0 |
| **Platform Perangkat Keras** | ESP32-C3 Super Mini + OLED 0.96" Dual-Color |
| **Platform Cloud IoT** | Blynk IoT (`TMPLn-jQSvce`) |
| **Status** | Siap Implementasi / Produksi Prototipe |

---

## 1. Ringkasan Eksekutif & Tujuan Produk (Product Overview)

### 1.1 Latar Belakang Masalah
Pemeliharaan tanaman sering terkendala oleh jadwal pemilik yang padat atau ketidaktahuan mengenai tingkat kelembapan media tanam secara akurat. Penyiraman yang hanya mengandalkan waktu terkadang kurang cukup saat cuaca sangat terik, sedangkan penyiraman manual membutuhkan kehadiran fisik di lokasi tanaman.

### 1.2 Solusi & Tujuan Produk
Membangun perangkat penyiram tanaman pintar berukuran ringkas (*compact*) berbasis **ESP32-C3 Super Mini** yang mengombinasikan **otomatisasi hibrida** (jadwal waktu internet NTP WIB dan deteksi kelembapan tanah *real-time*), dilengkapi layar informasi lokal berdesain ergonomis pada **OLED Dual-Color**, serta kemampuan pemantauan dan kendali jarak jauh melalui aplikasi **Blynk IoT**.

---

## 2. Target Pengguna & User Stories

### 2.1 Target Pengguna
* **Penghobi Tanaman Rumahan (Urban Gardener):** Membutuhkan sistem otomatis untuk menjaga tanaman tetap hidup saat ditinggal bekerja atau bepergian.
* **Pelajar / Mahasiswa / Pengembang IoT:** Membutuhkan purwarupa sistem kendali otomatis dan telemetri IoT yang terstruktur, rapi, dan mudah dipantau.

### 2.2 User Stories
1. **Sebagai pengguna**, saya ingin tanaman disiram secara otomatis setiap pukul `07:00` pagi dan `17:00` sore agar rutinitas nutrisi air tanaman terjaga.
2. **Sebagai pengguna**, saya ingin pompa menyala otomatis ketika tanah terdeteksi kering di luar jam jadwal, sehingga tanaman tidak layu saat cuaca panas ekstrem.
3. **Sebagai pengguna**, saya ingin melihat jam, mode operasi, tingkat kekeringan tanah (dalam angka dan grafik batang), serta status pompa langsung dari layar OLED pada alat tanpa harus membuka ponsel.
4. **Sebagai pengguna**, saya ingin dapat mengubah mode kerja (`AUTO` / `MANUAL`) dan menyalakan/mematikan pompa dari jarak jauh melalui aplikasi ponsel.

---

## 3. Spesifikasi Fungsional (Functional Requirements)

| ID | Nama Fitur | Deskripsi Fungsional | Prioritas |
| :--- | :--- | :--- | :--- |
| **FR-01** | **Akuisisi Sensor Tanah** | Sistem wajib membaca nilai analog sensor tanah pada `GPIO 2` setiap $1\text{ detik}$ dengan resolusi ADC $12\text{-bit}$ ($0 - 4095$) dan mengonversinya ke skala persentase ($0\% - 100\%$). | P0 (Kritis) |
| **FR-02** | **Sinkronisasi Jam NTP** | Sistem wajib menyinkronkan waktu dengan server `pool.ntp.org` pada zona waktu WIB ($\text{GMT}+7$) saat terhubung ke internet. | P0 (Kritis) |
| **FR-03** | **Penyiraman Terjadwal (Mode AUTO)** | Saat mode `AUTO` aktif, pompa wajib menyala pada pukul `07:00:00` dan `17:00:00` WIB selama tepat $10\text{ detik}$ (`DURASI_SIRAM_DETIK = 10`). | P0 (Kritis) |
| **FR-04** | **Penyiraman Berbasis Ambang Batas Sensor (Mode AUTO)** | Saat mode `AUTO` aktif dan berada di luar jadwal waktu, pompa wajib menyala jika nilai persentase sensor $> 50.0\%$ (`KERING`) dan mati jika $\le 50.0\%$ (`LEMBAP`). | P0 (Kritis) |
| **FR-05** | **Kendali Manual Jarak Jauh (Mode MANUAL)** | Saat pengguna mengubah mode ke `MANUAL` (`V1 = 0`), sistem wajib menonaktifkan pemicu otomatis dan mengikuti perintah saklar pompa dari Blynk (`V2`). | P0 (Kritis) |
| **FR-06** | **Sinkronisasi Status Cloud** | Saat perangkat baru menyala atau terhubung kembali ke Blynk (`BLYNK_CONNECTED`), perangkat wajib menarik (*sync*) status terakhir dari `V1` dan `V2` di server. | P1 (Tinggi) |
| **FR-07** | **Tampilan UI OLED Dual-Color** | Sistem wajib memperbarui layar OLED setiap $1\text{ detik}$ dengan memisahkan elemen *header* di zona kuning ($Y: 0 - 14$) dan elemen telemetri di zona biru ($Y: 18 - 63$). | P1 (Tinggi) |

---

## 4. Spesifikasi Non-Fungsional (Non-Functional Requirements)

* **NFR-01 (Kinerja & Responsivitas):** Siklus pembacaan sensor, evaluasi logika, pengiriman data Blynk, dan *rendering* layar OLED dilakukan secara *non-blocking* dengan interval tetap $1000\text{ ms}$ ($1\text{ Hz}$).
* **NFR-02 (Keamanan Aktuator / Fail-Safe):** Saat perangkat pertama kali dinyalakan (*booting*), pin relay (`GPIO 7`) harus langsung dikondisikan `HIGH` (Pompa Mati) sebelum koneksi WiFi selesai agar pompa tidak menyala tanpa kendali. Saat beralih dari mode `AUTO` ke `MANUAL` dengan kondisi tombol manual mati, pompa harus segera dimatikan.
* **NFR-03 (Ergonomi Visual Layar):** Tidak boleh ada elemen teks atau grafik yang melintasi baris piksel $Y = 16\text{ s/d }17$ untuk menghindari distorsi warna antara zona kuning dan biru pada modul OLED SSD1306.

---

## 5. Spesifikasi Antarmuka Pengguna (UI/UX Design)

### 5.1 Tata Letak Layar Lokal (OLED 128x64 Dual-Color)

```text
+--------------------------------------------+  --- Y = 0
| 07:00:05                        [ AUTO ]   |   ZONA KUNING (Tinggi: 16px)
+--------------------------------------------+  --- Y = 15 (Garis Horizontal)
| Tanah: KERING (65%)                        |   Y = 20
| [||||||||||||||||||||||||||||||          ] |   Y = 31..37 (Progress Bar 124x7px)
| Pompa: >> NYALA <<                         |   Y = 43
| Jadwal: 07:00 & 17:00                      |   Y = 54
+--------------------------------------------+  --- Y = 63 (ZONA BIRU: 48px)
```

### 5.2 Konfigurasi Datastreams Aplikasi Blynk IoT

| Virtual Pin | Nama Datastream | Tipe Data | Rentang Nilai | Widget Aplikasi Blynk | Arah Komunikasi |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **V1** | Mode Operasi | Integer | `0` (Manual), `1` (Auto) | Switch / Segmented Switch | Cloud $\rightarrow$ Device |
| **V2** | Kontrol Pompa Manual | Integer | `0` (OFF), `1` (ON) | Button / Switch (Mode Push/Switch) | Cloud $\rightarrow$ Device |
| **V3** | Nilai ADC Tanah | Integer / Float | $0 - 4095$ | Value Display / SuperChart | Device $\rightarrow$ Cloud |
| **V7** | Persentase Kekeringan | Float | $0.0 - 100.0$ | Gauge / Level Bar ($\%$) | Device $\rightarrow$ Cloud |
| **V8** | Status LED Pompa | Integer | `0` (Mati), `255` (Nyala) | LED Indicator | Device $\rightarrow$ Cloud |

---

## 6. Arsitektur Perangkat Keras & Parameter Konfigurasi

### 6.1 Alokasi Pin Mikrokontroler (ESP32-C3 Super Mini)
* **Sensor Kelembapan Tanah (AOUT):** `GPIO 2` (Input Analog ADC1)
* **Modul Relay Pompa (IN):** `GPIO 7` (Output Digital, *Active LOW*)
* **Layar OLED ($\text{I}^2\text{C}$ SDA):** `GPIO 8`
* **Layar OLED ($\text{I}^2\text{C}$ SCL):** `GPIO 9`

### 6.2 Parameter Konstanta Sistem
* **Alamat $\text{I}^2\text{C}$ OLED:** `0x3C`
* **Server NTP:** `pool.ntp.org` (Offset: $+25200\text{ detik}$ / $\text{GMT}+7$)
* **Jadwal Pagi (`JADWAL_PAGI_JAM`):** `7` (`07:00 WIB`)
* **Jadwal Sore (`JADWAL_SORE_JAM`):** `17` (`17:00 WIB`)
* **Durasi Siram Terjadwal (`DURASI_SIRAM_DETIK`):** $10\text{ detik}$
* **Ambang Batas Kekeringan Tanah:** $> 50.0\%$

---

## 7. Kriteria Penerimaan & Skenario Pengujian (Acceptance Criteria & QA)

| ID Uji | Skenario Pengujian | Kondisi Awal | Aksi / Input | Hasil yang Diharapkan (Expected Result) |
| :--- | :--- | :--- | :--- | :--- |
| **TC-01** | **Inisialisasi & Booting** | Perangkat mati | Hubungkan catu daya $5\text{ V}$ | Relay tidak menyala saat *booting*, layar menampilkan *Splash Screen*, terhubung ke WiFi & Blynk, jam NTP tampil sesuai WIB. |
| **TC-02** | **Pemicu Sensor Tanah (Mode AUTO)** | Mode `AUTO`, di luar jam 07:00/17:00 | Angkat probe sensor dari tanah basah ke udara/tanah kering (nilai $> 50\%$) | Status OLED berubah menjadi `KERING`, *progress bar* naik $> 50\%$, teks pompa menjadi `>> NYALA <<`, relay aktif (`LOW`), dan LED Blynk `V8` menyala. |
| **TC-03** | **Penghentian Otomatis Sensor (Mode AUTO)** | Mode `AUTO`, pompa sedang menyala karena kering | Celupkan probe sensor ke tanah basah/air (nilai $\le 50\%$) | Status OLED berubah menjadi `LEMBAP`, relay mati (`HIGH`), teks pompa menjadi `MATI`, dan LED Blynk `V8` padam. |
| **TC-04** | **Pemicu Jadwal Harian (Mode AUTO)** | Mode `AUTO`, tanah dalam kondisi `LEMBAP` ($\le 50\%$) | Waktu sistem mencapai pukul `07:00:00` atau `17:00:00` WIB | Pompa menyala tepat selama $10\text{ detik}$ (`07:00:00` s/d `07:00:09`) lalu mati kembali pada detik ke-$10$ (`07:00:10`). |
| **TC-05** | **Override ke Mode MANUAL** | Mode `AUTO`, tanah `KERING` (pompa menyala) | Ubah switch `V1` di aplikasi Blynk ke `MANUAL` (`0`) dengan kondisi `V2 = OFF` | Header OLED berubah menjadi `[MANUAL]`, pompa langsung mati meskipun tanah masih kering. |
| **TC-06** | **Eksekusi Pompa Manual** | Mode `MANUAL` (`V1 = 0`) | Tekan tombol `V2` di aplikasi Blynk ke posisi `ON` (`1`) | Pompa langsung menyala, OLED menampilkan `>> NYALA <<`, LED `V8` menyala, dan mati kembali saat `V2` diubah ke `OFF` (`0`). |

---

## 8. Risiko Teknis & Rencana Pengembangan Lanjutan (Future Roadmap)

### 8.1 Mitigasi Risiko Saat Ini
1. **Interpretasi Terbalik Nilai Persentase:** Saat ini rumus `persen = (soil / 4095.0) * 100.0` menghasilkan angka tinggi saat kering dan rendah saat basah. Jika pengguna menginginkan angka $100\%$ berarti "Sangat Basah/Lembap", rumusnya pada iterasi berikutnya dapat dibalik menggunakan fungsi `map()` atau $\text{Persen Lembap} = 100.0 - \left(\frac{\text{soil}}{4095.0} \times 100.0\right)$.
2. **Osilasi Relay (Chattering) di Sekitar Ambang $50\%$:** Jika nilai sensor berfluktuasi cepat di sekitar $49.9\% - 50.1\%$, relay dapat berbunyi cetak-cetek dengan cepat.

### 8.2 Roadmap Versi 2.0
* **Penambahan Histeresis (Hysteresis Control):** Menetapkan batas atas dan batas bawah (misalnya pompa menyala saat kekeringan $> 60\%$ dan baru berhenti menyiram setelah nilai turun ke $< 35\%$) untuk melindungi usia pakai relay dan pompa.
* **Kalibrasi Batas Basah & Kering (Dry/Wet Calibration):** Menambahkan konstanta nilai ADC saat sensor di udara kering dan di dalam air murni agar rentang $0\% - 100\%$ lebih akurat.
* **WiFi Provisioning (Blynk.Edgent / WiFiManager):** Menghapus *hardcode* SSID dan Password di dalam kode agar pengguna dapat mengganti koneksi WiFi langsung dari ponsel.