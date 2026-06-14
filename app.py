import streamlit as st
import urllib.request
import urllib.parse
import json
import time


# =========================
# BAGIAN 1: KONFIGURASI
# =========================

SUPA_URL = "https://udyjdfmrwcfbzykuaxqp.supabase.co"
SUPA_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InVkeWpkZm1yd2NmYnp5a3VheHFwIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODEzMzk1NjEsImV4cCI6MjA5NjkxNTU2MX0.vVOlA5HnX9Vb9L2rh61Jex5SEIU2jp3T7LgT2i480SY"

NAMA_TABEL = "sensor"


# =========================
# BAGIAN 2: SETUP HALAMAN
# =========================

st.set_page_config(
    page_title="Dashboard Sensor",
    layout="wide"
)

st.title("Dashboard Sensor Ultrasonic ESP32")
st.caption("Data diambil dari Supabase dan diperbarui otomatis setiap 30 detik.")


# =========================
# BAGIAN 3: FUNGSI STATUS
# =========================

def cek_status(jarak):
    if jarak < 10:
        return "Terlalu dekat"
    elif jarak < 30:
        return "Dekat"
    elif jarak < 100:
        return "Sedang"
    else:
        return "Jauh"


# =========================
# BAGIAN 4: AMBIL DATA SUPABASE
# =========================

def ambil_data():
    endpoint = f"{SUPA_URL}/rest/v1/{NAMA_TABEL}"

    parameter = {
        "select": "id,alat,jarak_cm,waktu",
        "alat": "eq.esp32",
        "order": "waktu.desc",
        "limit": "30"
    }

    url_lengkap = endpoint + "?" + urllib.parse.urlencode(parameter)

    request = urllib.request.Request(
        url_lengkap,
        headers={
            "apikey": SUPA_KEY,
            "Authorization": f"Bearer {SUPA_KEY}"
        }
    )

    try:
        response = urllib.request.urlopen(request)
        hasil = response.read().decode("utf-8")
        data = json.loads(hasil)
        return data

    except Exception as error:
        st.error("Gagal mengambil data dari Supabase.")
        st.text(error)
        return []


# =========================
# BAGIAN 5: TAMPILKAN DASHBOARD
# =========================

data = ambil_data()

if len(data) == 0:
    st.warning("Belum ada data sensor.")
else:
    data_terbaru = data[0]

    jarak_terbaru = float(data_terbaru["jarak_cm"])
    status_terbaru = cek_status(jarak_terbaru)
    waktu_terbaru = data_terbaru["waktu"]

    kolom1, kolom2, kolom3 = st.columns(3)

    with kolom1:
        st.metric("Jarak Terbaru", f"{jarak_terbaru:.2f} cm")

    with kolom2:
        st.metric("Status", status_terbaru)

    with kolom3:
        st.metric("Update Terakhir", waktu_terbaru)

    st.subheader("Grafik Jarak")

    data_grafik = []

    for baris in reversed(data):
        data_grafik.append(float(baris["jarak_cm"]))

    st.line_chart(data_grafik)

    st.subheader("Tabel Data Sensor")

    for baris in data:
        baris["status"] = cek_status(float(baris["jarak_cm"]))

    st.dataframe(data, use_container_width=True)


# =========================
# BAGIAN 6: AUTO UPDATE 30 DETIK
# =========================

st.caption("Dashboard akan mengambil ulang data setiap 30 detik.")

time.sleep(30)
st.rerun()