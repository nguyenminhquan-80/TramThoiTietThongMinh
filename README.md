# 🌤️ TRẠM GIÁM SÁT THỜI TIẾT THÔNG MINH

## 📌 Giới thiệu
Dự án "Trạm giám sát thời tiết thông minh" là thiết bị IoT đo nhiệt độ, độ ẩm, ánh sáng và gửi dữ liệu lên điện thoại. Sản phẩm được thực hiện bởi nhóm học sinh Trường THCS Đông Hồ tham gia cuộc thi **Tin học trẻ - Bảng D2**.

## 🛠️ Linh kiện sử dụng
| Linh kiện | Số lượng |
|-----------|----------|
| ESP32-S3 N16R8 | 1 |
| Cảm biến AHT30 (nhiệt độ, độ ẩm) | 1 |
| Cảm biến BH1750 (ánh sáng) | 1 |
| Màn hình TFT ST7789 1.3 inch | 1 |
| Relay 2 kênh | 1 |

## 🔌 Sơ đồ kết nối
Xem trong thư mục `TAI_LIEU/`

## 📥 Cài đặt phần mềm
1. Tải và cài đặt [Arduino IDE](https://www.arduino.cc/en/software)
2. Thêm ESP32 vào Boards Manager
3. Cài các thư viện: `Adafruit AHTX0`, `BH1750`, `Adafruit ST7789`, `Firebase ESP Client`
4. Mở file `CODE_ESP32/TRAM_THOI_TIET.ino`
5. **QUAN TRỌNG:** Sửa thông tin WiFi và Firebase của bạn vào code
6. Kết nối ESP32 và nhấn Upload

## 📱 Ứng dụng điện thoại
- Tải file `APP_INVENTOR/TRAM_THOI_TIET.aia`
- Import vào [MIT App Inventor](https://ai2.appinventor.mit.edu/)
- Hoặc cài file .apk (nếu có)

## 👤 Tác giả
- **Nguyễn Chí Bách** - Trường THCS Đông Hồ
- **Võ Hà Bạch Yến** - Trường THCS Đông Hồ

## 📄 Giấy phép
Dự án được phân phối dưới giấy phép **MIT License**. Xem file [LICENSE](LICENSE) để biết thêm chi tiết.
