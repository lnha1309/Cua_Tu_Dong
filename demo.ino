#include <ESP32Servo.h>

#define PIN_TRIG 12
#define PIN_ECHO 13
#define PIN_SERVO 14

// Tham số cấu hình
const float DIST_THRESHOLD_CM = 60.0;    // Ngưỡng phát hiện người để mở
const float SAFE_EDGE_CM      = 15.0;    // Mép cửa an toàn (anti-pinch)
const int   DOOR_CLOSED       = 0;       // Góc đóng
const int   DOOR_OPEN         = 90;      // Góc mở
// giữ cửa mở 5s rồi mới đóng khi ko có người
const unsigned long HOLD_OPEN_MS = 5000; // Giữ mở sau khi không còn ai

// Lọc nhiễu: lấy trung vị N mẫu
const int SAMPLES = 5;
const int SAMPLE_INTERVAL_MS = 40;

Servo door;
bool isOpen = false;
unsigned long lastSeenMs = 0;

float readDistanceCm() {
  float vals[SAMPLES];
  for (int i = 0; i < SAMPLES; i++) {
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(PIN_TRIG, LOW);

    unsigned long duration = pulseIn(PIN_ECHO, HIGH, 25000UL); // ~400cm
    //tốc độ âm thanh trong kk 343m/s
    float dist = (duration > 0) ? (duration * 0.0343f / 2.0f) : 9999.0f;
    vals[i] = dist;
    delay(SAMPLE_INTERVAL_MS);
  }
  // sort nhỏ gọn
  for (int i = 1; i < SAMPLES; i++) {
    float key = vals[i];
    int j = i - 1;
    while (j >= 0 && vals[j] > key) { vals[j + 1] = vals[j]; j--; }
    vals[j + 1] = key;
  }
  return vals[SAMPLES / 2]; // trung vị
}

void openDoor() {
  if (!isOpen) {
    door.write(DOOR_OPEN);
    isOpen = true;
    Serial.println("Door: OPEN");
  }
}

void closeDoor() {
  if (isOpen) {
    door.write(DOOR_CLOSED);
    isOpen = false;
    Serial.println("Door: CLOSED");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);

  door.attach(PIN_SERVO, 500, 2400);

  closeDoor();
  Serial.println("AutoDoor ready.");
}

void loop() {
  float d = readDistanceCm();
  Serial.print("Distance(cm): ");
  Serial.println(d, 1);

  unsigned long now = millis();

  // Anti-pinch: đang đóng mà có vật rất gần -> mở lại
  if (!isOpen && d < SAFE_EDGE_CM) {
    openDoor();
    lastSeenMs = now;
  }

  if (d <= DIST_THRESHOLD_CM) {
    lastSeenMs = now;
    openDoor();
  } else {
    if (isOpen && (now - lastSeenMs >= HOLD_OPEN_MS)) {
      closeDoor();
    }
  }
}
