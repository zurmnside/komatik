#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Pin sensor ultrasonic.
// TRIG digunakan untuk mengirim sinyal.
// ECHO digunakan untuk menerima pantulan sinyal.
#define PIN_TRIG 5
#define PIN_ECHO 18

// Data dikirim setiap 5 detik.
// Nilai 5000 artinya 5000 milidetik = 5 detik.
const unsigned long jedaKirim = 5000;

// Variabel untuk menyimpan waktu terakhir data dikirim.
unsigned long waktuTerakhir = 0;

// WiFiClientSecure digunakan karena Supabase memakai HTTPS.
WiFiClientSecure client;

// Fungsi untuk menghubungkan ESP32 ke WiFi.
void konekWiFi() {
  Serial.println("Menghubungkan ke WiFi...");

  // Memulai koneksi WiFi menggunakan nama dan password dari platformio.ini.
  WiFi.begin(WIFI_NAMA, WIFI_PASS);

  // ESP32 akan mencoba konek selama maksimal 20 kali.
  int percobaan = 0;

  while (WiFi.status() != WL_CONNECTED && percobaan < 20) {
    delay(500);
    Serial.print(".");
    percobaan++;
  }

  Serial.println();

  // Jika berhasil konek, tampilkan IP ESP32.
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi berhasil terhubung.");
    Serial.print("IP ESP32: ");
    Serial.println(WiFi.localIP());
  }

  // Jika gagal konek, ESP32 akan restart agar mencoba ulang dari awal.
  else {
    Serial.println("WiFi gagal terhubung. ESP32 restart.");
    delay(2000);
    ESP.restart();
  }
}

// Fungsi untuk membaca jarak dari sensor ultrasonic.
float bacaJarak() {
  // Pastikan TRIG dalam kondisi LOW terlebih dahulu.
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);

  // Kirim pulsa HIGH selama 10 mikrodetik.
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);

  // Membaca durasi pantulan pada pin ECHO.
  // Timeout 30000 mikrodetik agar program tidak macet terlalu lama.
  long durasi = pulseIn(PIN_ECHO, HIGH, 30000);

  // Jika durasi 0, berarti sensor gagal membaca pantulan.
  if (durasi == 0) {
    return -1;
  }

  // Rumus jarak:
  // kecepatan suara sekitar 0.0343 cm per mikrodetik.
  // Dibagi 2 karena sinyal pergi dan kembali.
  float jarak = durasi * 0.0343 / 2;

  // Jika hasil tidak masuk akal, anggap gagal.
  if (jarak < 0 || jarak > 400) {
    return -1;
  }

  return jarak;
}

// Fungsi untuk mengirim data jarak ke Supabase.
void kirimData(float jarak) {
  // Jika WiFi terputus, konek ulang.
  if (WiFi.status() != WL_CONNECTED) {
    konekWiFi();
  }

  // Mengizinkan koneksi HTTPS tanpa validasi sertifikat.
  // Ini praktis untuk prototype.
  // Untuk produksi, sebaiknya pakai root certificate.
  client.setInsecure();

  HTTPClient http;

  // Endpoint REST API Supabase.
  // Nama tabelnya adalah sensor.
  String url = String(SUPA_URL) + "/rest/v1/sensor";

  // Membuka koneksi HTTP ke URL Supabase.
  http.begin(client, url);

  // Header wajib untuk mengirim JSON ke Supabase.
  http.addHeader("Content-Type", "application/json");

  // Header apikey wajib untuk Supabase REST API.
  http.addHeader("apikey", SUPA_KEY);

  // Authorization menggunakan Bearer token dari anon key.
  http.addHeader("Authorization", String("Bearer ") + SUPA_KEY);

  // Supabase tidak perlu mengembalikan isi data setelah insert.
  // Ini membuat response lebih ringan.
  http.addHeader("Prefer", "return=minimal");

  // Data yang dikirim dalam format JSON.
  // Nama kolom harus sama dengan tabel Supabase.
  String data = "{";
  data += "\"alat\":\"esp32\",";
  data += "\"jarak_cm\":" + String(jarak, 2);
  data += "}";

  // Mengirim data dengan metode POST.
  int kode = http.POST(data);

  // Menampilkan informasi ke Serial Monitor.
  Serial.print("Data dikirim: ");
  Serial.println(data);

  Serial.print("Kode HTTP: ");
  Serial.println(kode);

  // Jika kode 200 sampai 299, artinya berhasil.
  if (kode >= 200 && kode < 300) {
    Serial.println("Status: berhasil kirim ke Supabase.");
  } else {
    Serial.println("Status: gagal kirim ke Supabase.");
    Serial.println(http.getString());
  }

  // Menutup koneksi HTTP.
  http.end();
}

void setup() {
  // Mengaktifkan Serial Monitor.
  Serial.begin(115200);

  // Menentukan mode pin sensor.
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  // Menghubungkan ESP32 ke WiFi.
  konekWiFi();

  Serial.println("Sistem siap membaca sensor.");
}

void loop() {
  // millis() digunakan agar program tidak berhenti total seperti delay panjang.
  unsigned long sekarang = millis();

  // Jika sudah lewat 5 detik, baca dan kirim data.
  if (sekarang - waktuTerakhir >= jedaKirim) {
    waktuTerakhir = sekarang;

    // Membaca jarak dari sensor.
    float jarak = bacaJarak();

    // Jika hasil valid, kirim ke Supabase.
    if (jarak > 0) {
      Serial.print("Jarak terbaca: ");
      Serial.print(jarak);
      Serial.println(" cm");

      kirimData(jarak);
    }

    // Jika hasil tidak valid, jangan dikirim.
    else {
      Serial.println("Sensor gagal membaca jarak.");
    }

    Serial.println("--------------------");
  }
}

