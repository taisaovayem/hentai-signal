#include <WiFi.h>
#include <esp_now.h>

#define BUTTON_PIN 0
#define LED_PIN 15

typedef struct struct_message {
  bool ledState;
} struct_message;

struct_message incomingData;
bool lastButtonState = HIGH;

// Broadcast address to all ESP-NOW peers
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// LED blink control
bool blinking = false;
unsigned long blinkStartTime = 0;
const unsigned long blinkDuration = 5000; // 5 seconds
unsigned long lastToggleTime = 0;
const unsigned long toggleInterval = 100; // blink every 500ms

// Callback when data is received
void OnDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *incomingDataBytes, int len) {
  memcpy(&incomingData, incomingDataBytes, sizeof(incomingData));
  if (incomingData.ledState) {
    blinking = true;
    blinkStartTime = millis();
    lastToggleTime = millis();
    digitalWrite(LED_PIN, HIGH); // Start with LED on
  }
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_is_peer_exist(broadcastAddress)) {
    esp_now_add_peer(&peerInfo);
  }
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW && lastButtonState == HIGH) {
    // Button just pressed

    struct_message outgoingData;
    outgoingData.ledState = true;

    esp_now_send(broadcastAddress, (uint8_t *) &outgoingData, sizeof(outgoingData));
    delay(500); // debounce
  }

  lastButtonState = buttonState;

  // Handle blinking
  if (blinking) {
    unsigned long currentTime = millis();

    if (currentTime - blinkStartTime >= blinkDuration) {
      blinking = false;
      digitalWrite(LED_PIN, LOW); // Stop blinking
    } else if (currentTime - lastToggleTime >= toggleInterval) {
      lastToggleTime = currentTime;
      digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
    }
  }
}
