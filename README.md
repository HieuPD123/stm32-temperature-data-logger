# Bộ Giám Sát Nhiệt Độ & Ghi Log Thẻ Nhớ (STM32)

Hệ thống sử dụng vi điều khiển STM32 để đọc nhiệt độ từ cảm biến DS18B20, hiển thị lên màn hình OLED 0.96", tự động quản lý ghi log vào thẻ nhớ SD và tương tác từ xa thông qua giao tiếp **UART1 (Baudrate: 115200)**.

---

## 💾 Cơ Chế Ghi Dữ Liệu Tối Ưu (RAM Buffer)

Để tăng tuổi thọ cho thẻ nhớ SD và tránh làm nghẽn hệ thống, vi điều khiển sử dụng cơ chế bộ đệm chuỗi:

* Cứ mỗi **1 giây**, hệ thống đọc nhiệt độ và lấy thời gian một lần, sau đó gom dữ liệu vào bộ đệm RAM (`sd_buffer`).
* Khi bộ đệm tích lũy đủ **10 dòng (tương ứng 10 giây)**, vi điều khiển mới mở thẻ SD, xả toàn bộ dữ liệu xuống file một lần duy nhất rồi đóng file lại ngay.
* Cứ mỗi **5 phút**, hệ thống tự động cập nhật và hiển thị phần trăm dung lượng sử dụng của thẻ lên màn hình OLED một cách độc lập.
* **Lưu ý khởi động:** Mỗi khi vi điều khiển bị Reset hoặc cấp nguồn lại, hệ thống sẽ tự động xóa file cũ để ghi log từ đầu.

---

## 📑 Giao Thức Điều Khiển Qua UART

Hệ thống hỗ trợ 2 tập lệnh cấu hình và truy xuất dữ liệu từ xa thông qua cổng UART1. Các lệnh đều bắt buộc phải kết thúc bằng ký tự xuống dòng `\n` (LF) hoặc `\r\n` (CRLF).

### 1. Lệnh Đọc Dữ Liệu Log (Phân Trang)

Dùng để đọc lại các dòng lịch sử nhiệt độ đã lưu trong file trên thẻ nhớ. Hệ thống trả về **10 dòng dữ liệu** cho mỗi trang.

**Cú pháp:**

```text
READ page_number
```

**Tham số:**

* `page_number` là số nguyên (bắt đầu từ `0`)
* `READ 0`: Đọc từ dòng 0 đến dòng 9
* `READ 1`: Đọc từ dòng 10 đến dòng 19
* `READ 2`: Đọc từ dòng 20 đến dòng 29

**Cơ chế hoạt động:**

Để tiết kiệm RAM và tài nguyên MCU, hệ thống sẽ lướt nhanh qua các dòng trước đó và chỉ gọi hàm truyền UART cho đúng 10 dòng thuộc trang được yêu cầu, sau đó đóng file sớm.

**Ví dụ tương tác:**

```text
Gửi:
READ 0

Phản hồi từ STM32:
--- START READING LINES 0 TO 9 ---
2026-06-17 13:20:01, 28.31
2026-06-17 13:20:02, 28.31
...
--- DONE ---
```

### 2. Lệnh Đồng Bộ Thời Gian Thực (RTC)

Dùng để cài đặt lại thời gian cho RTC của STM32. Thời gian này sẽ được sử dụng trực tiếp khi ghi dữ liệu vào file log.

**Cú pháp:**

```text
RTC:YYYY-MM-DD HH:MM:SS
```

**Ý nghĩa tham số:**

* `YYYY`: Năm (ví dụ: 2026)
* `MM`: Tháng (01–12)
* `DD`: Ngày (01–31)
* `HH`: Giờ (00–23)
* `MM`: Phút (00–59)
* `SS`: Giây (00–59)

**Lưu ý:**

* Không được có khoảng trắng giữa `RTC:` và phần năm.
* Sau khi cập nhật thành công, thời gian mới sẽ được sử dụng cho các bản ghi tiếp theo.

**Ví dụ tương tác:**

```text
Gửi:
RTC:2026-06-17 11:15:00

Phản hồi từ STM32:
System RTC Updated to: 2026-06-17 11:15:00
```

---

## 🛠 Định Dạng File Log Trên Thẻ Nhớ

Dữ liệu nhiệt độ được lưu trong file `log.csv` theo chuẩn CSV, giúp dễ dàng mở bằng Excel hoặc các công cụ phân tích dữ liệu.

**Ví dụ nội dung file:**

```text
2026-06-17 11:12:10, 25.50
2026-06-17 11:12:21, 25.52
2026-06-17 11:12:32, 25.48
```
