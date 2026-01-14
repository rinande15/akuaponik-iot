#include <WiFi.h>
//#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WebServer.h>
#include <ESP32Servo.h>

#include "DFRobot_ESP_PH.h"
#include "EEPROM.h"

DFRobot_ESP_PH ph;
#define ESPADC 4096.0   //the esp Analog Digital Convertion value
#define ESPVOLTAGE 3300 //the esp voltage supply value
#define PH_PIN 35    //the esp gpio data pin number
float voltage, phValue, temperature = 25;

// Library untuk LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Konfigurasi WiFi
const char* ssid = "ISK TEAM";
const char* password = "labISK2023ok";
WiFiServer server(80); 

// Konfigurasi waktu sahur
const int jamPakan = 8; // Jam pakan
const int menitPakan = 0; // Menit menit

//TDS
#include "GravityTDS.h"
#define TdsSensorPin 34
#define EEPROM_SIZE 512

GravityTDS gravityTds;
float temperaturetds = 25, tdsValue;

//Ultrasonik
#define triger 5
#define echo 18
static const int servoPin = 13;
Servo servo1;

void sevorun() {
    for(int posDegrees = 0; posDegrees <= 150; posDegrees++) {
        servo1.write(posDegrees);
        //Serial.println(posDegrees);
        delay(2);
    }

    for(int posDegrees = 180; posDegrees >= 0; posDegrees--) {
        servo1.write(posDegrees);
        //Serial.println(posDegrees);
        delay(2);
    }
}

void phread() {
  static unsigned long timepoint = millis();
  if (millis() - timepoint > 1000U) //time interval: 1s
  {
    timepoint = millis();
    //voltage = rawPinValue / esp32ADC * esp32Vin
    voltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE; // read the voltage
    phValue = ph.readPH(voltage, temperature); // convert voltage to pH with temperature compensation
  }
  ph.calibration(voltage, temperature); // calibration process by Serail CMD
}

void tdsread() {
    //temperature = readTemperature();  //add your temperature sensor and read it
    gravityTds.setTemperature(temperaturetds);  // set the temperature and execute temperature compensation
    gravityTds.update();  //sample and calculate
    tdsValue = gravityTds.getTdsValue();  // then get the value
    //Serial.print(tdsValue,0);
    //Serial.println("ppm");
    delay(1000);

}

#define kran1 26

void setup() {
  servo1.attach(servoPin);
  pinMode (triger, OUTPUT); //trigger sebagai output
  pinMode (echo, INPUT); //echo sebagai input
  pinMode (kran1, OUTPUT);
  digitalWrite(kran1, HIGH);

  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight();


  // Inisialisasi WiFi dan sinkron waktu
  Serial.begin(115200);
  WiFi.begin(ssid, password);
 // configTime(0, 0, "pool.ntp.org"); // sinkron waktu dengan server ntp
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)  // Waiting for the response of wifi network
  {
    delay (500);
    Serial.print (".");
  }
  Serial.println("");
  Serial.println("Connection Successful");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());           // Getting the IP address
  Serial.println("Type the above IP address into browser search bar"); 
  server.begin();                           // Starting the server

  EEPROM.begin(32);//needed to permit storage of calibration value in eeprom
  ph.begin();

  //TDS
    EEPROM.begin(EEPROM_SIZE);  //Initialize EEPROM
    
    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(3.3);  //reference voltage on ADC, default 5.0V on Arduino UNO
    gravityTds.setAdcRange(4096);  //1024 for 10bit ADC;4096 for 12bit ADC
    gravityTds.begin();  //initialization

}

void loop() {
  digitalWrite (triger, HIGH); //mengirim suara
  delayMicroseconds(10);  //selama 10 mikro detik
  digitalWrite (triger, LOW); //berhenti mengirim suara
 
  
  
  float jarak = pulseIn(echo, HIGH); //membaca data dan di masukkan ke variabel jarak
  jarak=jarak/1000000; //konversi mikro detik ke detik
  jarak=jarak*330/2; //data mentah di ubah ke dalam meter
  jarak=jarak*100; //mengubah data ke dalam centi meter
  float kap=(jarak/35)*100;
  float kapasitas= 100-kap;
  //Serial.println(jarak);
  // Ambil waktu sekarang
  time_t now = time(nullptr);

  // Konversi waktu sekarang ke waktu lokal
  struct tm* localTime = localtime(&now);

    phread();
   tdsread();
   //Serial.print("TDS Value:");
   //Serial.print(tdsValue,0);
   //Serial.println("ppm");

  // Tampilkan waktu sekarang pada LCD
  lcd.setCursor(0, 0);
  lcd.print("WAKTU: ");
  lcd.print(localTime->tm_hour);
  lcd.print(":");
  if (localTime->tm_min < 10) {
    lcd.print("0");
  }
  lcd.print(localTime->tm_min);
  lcd.print(":");
  if (localTime->tm_sec < 10) {
    lcd.print("0");
  }
  lcd.print(localTime->tm_sec);

  // Tampilkan waktu sahur pada LCD
  lcd.setCursor(0, 1);
  lcd.print("JADWAL: ");
  lcd.print(sahurHour);
  lcd.print(":");
  if (sahurMinute < 10) {
    lcd.print("0");
  }
  lcd.print(sahurMinute);
  lcd.setCursor(0, 2);
   lcd.print("PH: ");
   lcd.print(phValue,1);
   lcd.print(" TDS: ");
   lcd.print(tdsValue,0);
   lcd.print(" ppm");
   lcd.setCursor(0,3);
   lcd.print("K:");
   lcd.print(kapasitas,0);
   lcd.print("% ");
   lcd.print(WiFi.localIP()); 

  // Periksa apakah sudah waktunya sahur
  if (localTime->tm_hour == sahurHour && localTime->tm_min == sahurMinute && localTime->tm_sec == 0) {
    lcd.setCursor(0, 1);
    lcd.print("Waktunya Makan!");
    sevorun();
  }

  delay(1000);
  WiFiClient client = server.available();

  if (client)
  {
    boolean currentLineIsBlank = true;
    String buffer = "";  
    while (client.connected())
    {
      if (client.available())                    // if there is some client data available
      {
        char c = client.read(); 
        buffer+=c;                              // read a byte
        if (c == '\n' && currentLineIsBlank)    // check for newline character, 
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();    
          client.print("<HTML><title>ESP32</title>");
          client.print("<body><center><h2>INOVASI AKUAPONIK 4.0: INTEGRASI IOT DALAM REVITALISASI BUDIDAYA IKAN LELE & PAKCOY PADA MEDIA AKUAPONIK </h2></center>");
          client.print("<tr><h3><td>PH Air: </td><td><span class=\"sensor\">");
          client.print(phValue,2);
          client.println(" </span></td></h3></tr>");
          client.print("<tr><h3><td>TDS: </td><td><span class=\"sensor\">");
          client.print(tdsValue,0);
          client.println(" ppm</span></td></h3></tr>");
          client.print("<tr><h3><td>Ketersedian Pakan: </td><td><span class=\"sensor\">");
          client.print(kapasitas,2);
          client.println(" %</span></td></h3></tr>"); 
          //client.print("<p>Kontrol Pakan</p>");
          client.print("<a href=\"/?relayon\"\"><center><button><h2>BERI PAKAN</h2></button></center></a>");
          client.print("</body></HTML>");
          break;        // break out of the while loop:
        }
        if (c == '\n') { 
          currentLineIsBlank = true;
          buffer="";       
        } 
        else 
          if (c == '\r') {     
          if(buffer.indexOf("GET /?relayon")>=0)
            sevorun();
   
        }
        else {
          currentLineIsBlank = false;
        }  
      }
    }
    client.stop();
  }
  if (phValue < 6 || tdsValue > 256) {
    digitalWrite(kran1, LOW);
    delay(500);
  }
  else {
    digitalWrite(kran1, HIGH);
    delay(500);
  }
}
