#include <AccelStepper.h>

// --- PİN TANIMLAMALARI (Arduino Nano) ---
#define IN1 4
#define IN2 5
#define IN3 6
#define IN4 7

#define ENCODER_CLK 2 // Mutlaka D2 olmalı (Interrupt 0)
#define ENCODER_DT 3
#define ENCODER_SW 8

// 1-3-2-4 bobin tetikleme sırası (Yazılımsal çaprazlama)
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

// --- HIZ DEĞİŞKENLERİ ---
volatile int setSpeed = 500;      // Başlangıç hızı
const int maxSpeed = 1000;        // 28BYJ-48 limiti
const int speedStep = 100;        // Encoder'ın her tıkında artacak hız

volatile bool directionForward = true;
volatile unsigned long lastButtonPress = 0;

// --- İVME (RAMPING) DEĞİŞKENLERİ ---
float currentActualSpeed = 0.0;   
int targetSpeed = 0;              
const float acceleration = 1.5;   // İvme katsayısı
unsigned long lastAccelTime = 0;
const int accelInterval = 5;      // Güncelleme periyodu (ms)

// --- INTERRUPT FONKSİYONU (Nano için IRAM_ATTR kaldırıldı) ---
void readEncoder() {
  int dtValue = digitalRead(ENCODER_DT);
  if (dtValue == HIGH) {
    setSpeed += speedStep; // Sağa çevrildi
  } else {
    setSpeed -= speedStep; // Sola çevrildi
  }
  
  if (setSpeed > maxSpeed) setSpeed = maxSpeed;
  if (setSpeed < 0) setSpeed = 0; 
}

void setup() {
  Serial.begin(115200);

  // Pin Modları (Dahili Pull-up dirençlerini açıyoruz)
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  // Interrupt Ayarı (Sadece düşen kenarda tetiklenir)
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), readEncoder, FALLING);

  // Motor Başlangıç Ayarları
  stepper.setMaxSpeed(maxSpeed);
  stepper.setSpeed(0); 
}

void loop() {
  // 1. Buton Okuma ve Yön Değiştirme (Debounce ile)
  if (digitalRead(ENCODER_SW) == LOW) {
    if (millis() - lastButtonPress > 250) { 
      directionForward = !directionForward;
      lastButtonPress = millis();
    }
  }

  // 2. Hedef Hızı Hesapla
  targetSpeed = directionForward ? setSpeed : -setSpeed;

  // 3. Yazılımsal İvmelenme (Ramping) Algoritması
  if (millis() - lastAccelTime > accelInterval) {
    if (currentActualSpeed < targetSpeed) {
      currentActualSpeed += acceleration;
      if (currentActualSpeed > targetSpeed) currentActualSpeed = targetSpeed; 
    } 
    else if (currentActualSpeed > targetSpeed) {
      currentActualSpeed -= acceleration;
      if (currentActualSpeed < targetSpeed) currentActualSpeed = targetSpeed;
    }
    
    stepper.setSpeed(currentActualSpeed);
    lastAccelTime = millis();
  }

  // 4. Motoru Döndür
  stepper.runSpeed();
}