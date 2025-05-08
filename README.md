# ChieModel - Sistem Avatar Virtual 2D

## Deskripsi
ChieModel adalah aplikasi avatar virtual 2D yang dapat digunakan sebagai virtual camera atau hanya tampilan jendela. Aplikasi ini memungkinkan pengguna untuk mengubah pose dan ekspresi avatar melalui kontrol keyboard, dengan animasi transisi yang mulus saat berpindah pose.

## Fitur
- Berbagai pose dan ekspresi dengan kontrol keyboard
- Animasi lompatan yang halus saat berpindah pose
- Mode kamera virtual untuk aplikasi konferensi video
- Mode jendela untuk penggunaan tanpa kamera virtual
- Toggle ekspresi dengan menekan tombol yang sama dua kali
- Gambar model tertanam dalam biner aplikasi

## Dependensi
Untuk membangun dan menjalankan ChieModel, Anda memerlukan dependensi berikut:

```
SDL2
SDL2_image
SDL2_ttf
python3 (untuk menghasilkan embedded_models.cpp)
v4l2loopback (opsional, untuk mode kamera virtual)
```

### Instalasi Dependensi (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-ttf-dev python3

# Opsional, untuk mode kamera virtual
sudo apt install v4l2loopback-dkms
```

## Cara Membangun

1. Kloning repositori ini:
```bash
git clone https://github.com/username/ChieModel.git
cd ChieModel
```

2. Bangun aplikasi:
```bash
make
```

3. (Opsional) Instal aplikasi secara sistem-wide:
```bash
sudo make install
```

## Cara Menggunakan

### Mode Kamera Virtual
Untuk menggunakan ChieModel sebagai kamera virtual:

1. Muat modul v4l2loopback terlebih dahulu:
```bash
sudo modprobe v4l2loopback devices=1 video_nr=20 card_label="ChieModel Virtual Camera" exclusive_caps=1
```

2. Jalankan aplikasi:
```bash
./ChieModel
```

3. Kemudian, pilih "ChieModel Virtual Camera" dalam aplikasi konferensi video Anda.

### Mode Jendela
Untuk menjalankan ChieModel hanya dalam mode jendela (tanpa kamera virtual):
```bash
./ChieModel --window
```

### Kontrol Keyboard
- `1-9, 0`: Mengubah pose avatar (posisi tubuh)
- `Q, W, E, R, dll`: Mengubah ekspresi wajah
- Tekan tombol yang sama dua kali untuk beralih antara ekspresi 1 dan ekspresi yang dipetakan
- `ESC`: Keluar dari aplikasi

## Kustomisasi
Untuk mengubah model avatar, ganti gambar di folder `model/` dan recompile aplikasi.

## Uninstall
Jika Anda telah menginstal aplikasi secara sistem-wide:
```bash
sudo make uninstall
```

## Lisensi
Proyek ini dilisensikan di bawah [Masukkan Lisensi Anda di Sini].

## Kredit
Dikembangkan oleh [Nama Anda]. Avatar Chie dibuat oleh [Artis/Pemilik Karakter].
