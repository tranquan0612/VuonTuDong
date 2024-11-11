
#include <HardwareSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
HardwareSerial SIMSerial(2);            // Khởi tạo UART2
const int soilSensorPin = 36;           // Chân cảm biến đất
const int dhtPin = 23;                  // Chân kết nối với cảm biến DHT
const String trustedNumber = "+84866604632"; 
#define DHTTYPE DHT11                   
DHT dht(dhtPin, DHTTYPE);               
int mode=1,fan=0,pump=0,light=0;
#define btn1 19
#define btn2 18
#define btn3 5
#define btn4 4
#define btn5 15
int stop=0;
void setup() {
  Serial.begin(115200);
  SIMSerial.begin(115200, SERIAL_8N1, 16, 17); // RX2=16, TX2=17 với baud rate 115200
  dht.begin();                                 // Khởi động DHT
  pinMode(soilSensorPin, INPUT);               // Thiết lập chân cảm biến đất làm đầu vào
  pinMode(25,OUTPUT);//fan
  pinMode(27,OUTPUT);//pump
  pinMode(32,OUTPUT);//light
  pinMode(btn1,INPUT_PULLUP);
  pinMode(btn2,INPUT_PULLUP);
  pinMode(btn3,INPUT_PULLUP);
  pinMode(btn4,INPUT_PULLUP);
  pinMode(btn5,INPUT_PULLUP);
  sendATCommand("AT", "OK", 1000);             // Kiểm tra kết nối với module
  sendATCommand("AT+CMGF=1", "OK", 1000);      // Chuyển sang chế độ SMS text
  sendATCommand("AT+CNMI=1,2,0,0,0", "OK", 1000); // Cấu hình tự động hiển thị tin nhắn mới
  lcd.init();                                  // Khởi động LCD
  lcd.backlight();                             // Bật đèn nền LCD
}

void loop() {
  int soilMoistureValueADC = analogRead(soilSensorPin); // Đọc giá trị cảm biến đất
  float soilMoistureValue = convertSoilMoisture(soilMoistureValueADC); // Chuyển đổi sang % 
  float humidity = dht.readHumidity();               // Đọc độ ẩm từ DHT
  float temperature = dht.readTemperature();
  checkForIncomingSMS();                       // Kiểm tra và nhận tin nhắn mới
  delay(80);
  if(digitalRead(btn5)==0){
      delay(15);
      if(digitalRead(btn5)==0){
        if(stop==1) stop=0;
        else stop=1;
      }
    }
  if(stop==1){
    digitalWrite(25,0);
    digitalWrite(27,0);
    digitalWrite(32,0);
    displayOnLCD(soilMoistureValue,temperature,humidity);
  }else{
      if(digitalRead(btn1)==0){
      delay(15);
      if(digitalRead(btn1)==0){
        if(mode==1) mode=0;
        else mode=1;
      }
    }
    if(mode==0){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("che do tay");
      manuMode();
    }else{
      if (isnan(humidity) || isnan(temperature)){
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Khong doc DHT");
      }else displayOnLCD(soilMoistureValue,temperature,humidity);
      autoMode(soilMoistureValue,temperature,humidity);
    }
  }
}
void autoMode(int soilMoistureValue, float temperature, float humidity){
  if(soilMoistureValue<10) digitalWrite(27,1);
  else digitalWrite(27,0);
  if(temperature<32) digitalWrite(25,0);
  else digitalWrite(25,1);
}
void manuMode(){
  if(digitalRead(btn2)==0){
    delay(15);
    if(digitalRead(btn2)==0){
      if(fan==1) fan=0;
      else fan=1;
    }
  }
  if(digitalRead(btn3)==0){
    delay(15);
    if(digitalRead(btn3)==0){
      if(pump==1) pump=0;
      else pump=1;
    }
  }
  if(digitalRead(btn4)==0){
    delay(15);
    if(digitalRead(btn4)==0){
      if(light==1) light=0;
      else light=1;
    }
  }
  digitalWrite(25,fan);
  digitalWrite(27,pump);
  digitalWrite(32,light);
}
void displayOnLCD(int soilMoistureValue, float temperature, float humidity) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TEMP:");
  lcd.print(temperature);          // Hiển thị nhiệt độ
  lcd.print(" C");
  lcd.setCursor(0, 1);
  lcd.print("HUMI:");
  lcd.print(humidity);             // Hiển thị độ ẩm
  lcd.print("%");
}
void sendATCommand(String cmd, String expectedResponse, int timeout) {
  SIMSerial.println(cmd);
  String response = "";
  long timeStart = millis();
  while (millis() - timeStart < timeout) {
    while (SIMSerial.available() > 0) {
      char c = SIMSerial.read();
      response += c;
    }
    if (response.indexOf(expectedResponse) != -1) {
      break;
    }
  }
  Serial.println(response); // Hiển thị phản hồi từ module
}

void checkForIncomingSMS() {
  if (SIMSerial.available() > 0) {
    String message = "";
    while (SIMSerial.available() > 0) {
      char c = SIMSerial.read();
      message += c;
      delay(10);
    }

    Serial.println("Received SMS:");
    Serial.println(message);

    if (message.indexOf(trustedNumber) != -1 && message.indexOf("THONG SO") != -1) {
      int soilMoistureValueADC = analogRead(soilSensorPin); // Đọc giá trị cảm biến đất
      float soilMoistureValue = convertSoilMoisture(soilMoistureValueADC); // Chuyển đổi sang % 
      float humidity = dht.readHumidity();               // Đọc độ ẩm từ DHT
      float temperature = dht.readTemperature();         // Đọc nhiệt độ từ DHT
      
      // Kiểm tra xem DHT có đọc được giá trị không
      if (isnan(humidity) || isnan(temperature)) {
        Serial.println("Failed to read from DHT sensor!");
        sendSMS(trustedNumber, "Khong doc duoc gia tri DHT");
        return;
      }

      // Tạo chuỗi tin nhắn bao gồm cả giá trị đất và DHT
      String responseMessage = "Gia tri do am dat: " + String(soilMoistureValue) + "\nNhiet do: " + String(temperature) + "C\nDo am: " + String(humidity) + "%";
      sendSMS(trustedNumber, responseMessage);           // Gửi lại giá trị qua SMS
      Serial.println("Sent soil and DHT values");
}else if (message.indexOf(trustedNumber) != -1 && message.indexOf("DIEU KHIEN TAY") != -1) {
      mode=0;
      sendSMS(trustedNumber, "DA CHUYEN CHE DO MANUAL");
    }else if (message.indexOf(trustedNumber) != -1 && message.indexOf("TU DONG") != -1) {
      mode=1;
      sendSMS(trustedNumber, "DA CHUYEN CHE DO AUTO");
    }
    if(mode==0){
      if (message.indexOf(trustedNumber) != -1 && message.indexOf("QUAT BAT") != -1) {
        fan=1;
        digitalWrite(25,1);
        sendSMS(trustedNumber, "QUAT DA BAT");
      }else if (message.indexOf(trustedNumber) != -1 && message.indexOf("QUAT TAT") != -1) {
        fan=0;
        digitalWrite(25,0);
        sendSMS(trustedNumber, "QUAT DA TAT");
      }else if (message.indexOf(trustedNumber) != -1 && message.indexOf("BOM BAT") != -1) {
        pump=1;
        digitalWrite(27,1);
        sendSMS(trustedNumber, "BOM DA BAT");
      }else if (message.indexOf(trustedNumber) != -1 && message.indexOf("BOM TAT") != -1) {
        pump=0;
        digitalWrite(27,0);
        sendSMS(trustedNumber, "BOM DA TAT");
      }else if (message.indexOf(trustedNumber) != -1 && message.indexOf("DEN BAT") != -1) {
        light=1;
        digitalWrite(32,1);
        sendSMS(trustedNumber, "DEN DA BAT");
      }else if (message.indexOf(trustedNumber) != -1 && message.indexOf("DEN TAT") != -1) {
        light=0;
        digitalWrite(32,0);
         sendSMS(trustedNumber, "DEN DA TAT");
      }
    }
  }
}

void sendSMS(String number, String message) {
  String cmd = "AT+CMGS=\"" + number + "\"\r\n";
  SIMSerial.print(cmd);
  delay(100);

  SIMSerial.print(message);     // Gửi nội dung tin nhắn
  delay(100);
  SIMSerial.write(0x1A);        // Gửi ký tự Ctrl+Z để kết thúc tin nhắn
  delay(1000);
}
float convertSoilMoisture(int adcValue) {
  // Chuyển đổi giá trị ADC sang % (giả định ADC là 10-bit)
  // ADC tối đa là 1023 (tương ứng với 0-3.3V)
  float voltage = (adcValue / 4095.0) * 3.3; // Chuyển đổi giá trị ADC sang điện áp
  float moisturePercent;

  // Giả định 0V = 100% độ ẩm, 3.3V = 0% độ ẩm (có thể điều chỉnh dựa trên cảm biến của bạn)
  moisturePercent = (1 - (voltage / 3.3)) * 100; // Chuyển đổi điện áp sang %
  
  // Đảm bảo giá trị phần trăm nằm trong khoảng 0-100
  if (moisturePercent < 0) moisturePercent = 0;
  if (moisturePercent > 100) moisturePercent = 100;

  return moisturePercent;
}