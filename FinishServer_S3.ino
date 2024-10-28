#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <esp_adc_cal.h>

#define PIN_BAT_VOLT 4

esp_now_peer_info_t peerInfo;
uint8_t address_Pilot_1[] = {0x08, 0x3A, 0x8D, 0x96, 0x40, 0x58}; //08:3A:8D:96:40:58
uint8_t address_Pilot_2[] = {0xB0, 0xB2, 0x1C, 0xF8, 0xB5, 0xC4}; //B0:B2:1C:F8:B5:C4
uint8_t address_Pilot_3[] = {0xB0, 0xB2, 0x1C, 0xF8, 0xBC, 0x48}; //B0:B2:1C:F8:BC:48
uint8_t address_Pilot_4[] = {0xB0, 0xB2, 0x1C, 0xF8, 0xAE, 0x88}; //B0:B2:1C:F8:AE:88
uint8_t address_Pilot_5[] = {0x40, 0x22, 0xD8, 0x07, 0x9C, 0x4C}; //40:22:D8:07:9C:4C TEST ID
uint8_t address_Pilot_6[] = {0xFC, 0xB4, 0x67, 0x4E, 0x7A, 0xF0}; //FC:B4:67:4E:7A:F0 SPARE 1

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h

#define BUTTON_RESET_PIN  0
#define BUTTON_RESET_PIN_2  14

typedef struct {
    uint8_t modeNum;
    volatile bool isReset;
} out_struct;

out_struct outMessage;

void IRAM_ATTR OnResetMode_0() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 250)
  {
    outMessage.modeNum = 0;
    outMessage.isReset = true;
  }
  last_interrupt_time = interrupt_time;
}

void IRAM_ATTR OnResetMode_1() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 250)
  {
    outMessage.modeNum = 1;
    outMessage.isReset = true;
  }
  last_interrupt_time = interrupt_time;
}

void setup(void) {
  Serial.begin(115200);
  pinMode(15, OUTPUT); // to boot with battery...
  digitalWrite(15, 1);
  tft.init();
  tft.setRotation(0);
  DrawDefaultScreen();
  pinMode(BUTTON_RESET_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RESET_PIN_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RESET_PIN), OnResetMode_0, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RESET_PIN_2), OnResetMode_1, FALLING);

  outMessage.modeNum = 0;
  outMessage.isReset = false;
  
  WiFi.mode(WIFI_STA);
  Serial.println("Tiny Drones - Race Finisher Server");
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    tft.setTextFont(2);
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_BLACK);
    tft.setCursor(3,0,2);
    tft.println("Error init ESP-NOW");
    return;
  }
  else{
    esp_now_register_send_cb(OnDataSent);

    // register peer
    AddMacToServer(address_Pilot_1);
    AddMacToServer(address_Pilot_2);
    AddMacToServer(address_Pilot_3);
    AddMacToServer(address_Pilot_4);
    AddMacToServer(address_Pilot_5);
    AddMacToServer(address_Pilot_6);
    
    esp_now_register_recv_cb(OnDataRecv);
  }  
}

void loop() {
  if(outMessage.isReset){
    //esp_err_t result = esp_now_send(0, (uint8_t *) &outMessage, sizeof(outMessage));
    esp_err_t result;
    result = esp_now_send(address_Pilot_1, (uint8_t *) &outMessage, sizeof(outMessage));
    delay(20);
    result = esp_now_send(address_Pilot_2, (uint8_t *) &outMessage, sizeof(outMessage));
    delay(20);
    result = esp_now_send(address_Pilot_3, (uint8_t *) &outMessage, sizeof(outMessage));
    delay(20);
    result = esp_now_send(address_Pilot_4, (uint8_t *) &outMessage, sizeof(outMessage));
    delay(20);
    result = esp_now_send(address_Pilot_5, (uint8_t *) &outMessage, sizeof(outMessage));
    delay(20);
    result = esp_now_send(address_Pilot_6, (uint8_t *) &outMessage, sizeof(outMessage));
    outMessage.isReset = false;
    DrawDefaultScreen();
  }
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  Serial.print("Packet from: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
  Serial.print(" send status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  String pilot = "";
  memcpy(&pilot, incomingData, sizeof(pilot));
  if(pilot != ""){
    tft.println(String("     " + pilot));
  }
}

void AddMacToServer(uint8_t mac[]){
  // register peer  
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Serial.print("Failed to add peer: ");
    Serial.println(macStr);
    return;
  }
}

void DrawDefaultScreen(){
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 5, 4);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextFont(4);
  tft.println("   Tiny Drones ");
  tft.setTextFont(2);
  tft.println("     Finish Line Server"); 
  BatteryValue();
  DrawMode();
  tft.fillRect(0, 66, 170, 2, TFT_GREEN);
  tft.setCursor(0, 80, 4);
  tft.setTextColor(TFT_YELLOW);
  tft.setTextFont(4);
}

void DrawMode(){
  if(outMessage.modeNum == 0){
    tft.fillRect(75, 310, 20, 6, TFT_YELLOW); //2
  }
  else{
    tft.fillRect(45, 310, 20, 6, TFT_YELLOW); //1
    tft.fillRect(75, 310, 20, 6, TFT_YELLOW); //2
    tft.fillRect(105, 310, 20, 6, TFT_YELLOW); //3
  }
}
void BatteryValue(){
  esp_adc_cal_characteristics_t adc_chars;

  // Get the internal calibration value of the chip
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  uint32_t raw = analogRead(PIN_BAT_VOLT);
  uint32_t v1 = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2; //The partial pressure is one-half

  // If the battery is not connected, the ADC detects the charging voltage of TP4056, which is inaccurate.
  // Can judge whether it is greater than 4300mV. If it is less than this value, it means the battery exists.
  if (v1 > 4300) {
      tft.print("          USB mode");
    //tft.print("   No battery connected!");
  } else {
      tft.print("       Battery: ");
      float batt = (v1 * 1.0) /1000;
      tft.print(String(batt, 2));
      tft.print("V");
      Serial.println(String(batt,2));
      Serial.println(v1);
  }
}
