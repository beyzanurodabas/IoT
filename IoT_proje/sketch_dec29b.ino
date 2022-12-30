/**********************************************************
 *      Nesnelerin İnterneti ve Uygulamaları Dersi
 *  Proje Uygulaması (Akıllı Çöp Kutusu)
 *           Yazar : Beyzanur Odabaş
 *********************************************************/
/* Kütüphane Dosyaları */
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include <ESP_Mail_Client.h>
#include <BlynkSimpleEsp8266.h>
//#include <SimpleTimer.h>
BlynkTimer timer;
#define BLYNK_PRINT Serial

/* Kablosuz Bağlantı Bilgileri */ 
char auth [] = "qBiRJU_LRFrhfNLNYFEV0UGNUuF8sQk3";  // Blynk key  
const char* WLAN_SSID = "Eseny"; // "Kablosuz Ağ Adı" 
const char* WLAN_PASSWORD = "Yasin1234"; // "Kablosuz Ağ Şifresi"


/* Email Göndermek İçin SMTP sunucu ayarları */
#define SMTP_server "smtp.gmail.com"
#define SMTP_PORT 465  
#define AUTHOR_EMAIL "beyyzaodabass@gmail.com" //göndericinin emaili
#define AUTHOR_PASSWORD "bbwsvuxtnunwwydv" //göndericinin şifresi : bbwsvuxtnunwwydv
#define RECIPIENT_EMAIL "beyzanurodabas6@gmail.com" //alıcı mail adresi
 
SMTPSession smtp; 

/* ThingSpeak Kurulumu */ 
unsigned long channelID =1995320;             // Thingspeak channel ID 
unsigned int field_no=1; // percentage(yüzde) yazacağımız field numarası 
const char* writeAPIKey = "70ORUOWEJU30XLYQ";   // Thingspeak write API Key 
const char* thingSpeakHost = "api.thingspeak.com";


// ---------- Çöp Kutusu Değişkenleri ve Pin Tanımları ------------//
#define MQ2pin (0) //AO pinine bağlanmalı. Gaz sensoru
float gasSensorValue;
const int total_height = 30; //Çöp kutusunun CM cinsinden uzunluğu
const int hold_height = 25;// CM cinsinden çöpün max kapasitesi

const int trigPin = 16; // D0
const int echoPin = 5; // D1
const int led1=D5; //kırmızı led
const int led2=D6; //sarı led
const int led3=D7; //yeşil led
int distance;  //mesafe cm
int result;    //sonuc cm
long Time;  //duration
int capacity = 0;
int garbageLevel = 0;   //çöp seviyesi
int x;

WiFiClient client; 
/*** ESP8266 WiFi Kurulum Fonksiyonu ***/
void wifiSetup()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Kablosuz Agina Baglaniyor");
    WiFi.begin(WLAN_SSID, WLAN_PASSWORD);
    // WiFi durum kontrolü
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
    }
    Serial.println();
    Serial.print(WLAN_SSID);
    Serial.println("Kablosuz Aga Baglandi");
  }
}

void mesafeSensor (){
  /* hc-sr04 sensöründen okuma işlemi */
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    Time = pulseIn(echoPin, HIGH);
    distance = Time * 0.034;  //mıkrosanıye basına cm cinsinden hareket miktarını 
    result = distance / 2;  //gidiş gelişten ötürü
    garbageLevel = map(result, capacity, total_height, 100, 0);
}
 /* mq-2 gaz sensöründen okuma işlemi */
void gasSensor(){ //blynk'e aktaracak
  gasSensorValue = analogRead(MQ2pin); // read analog input pin 0
  //Blynk.virtualWrite(V2,gasSensorValue);
  Serial.print("Gaz sensörü degeri : ");
  Serial.println(gasSensorValue);
  if(gasSensorValue > 800)
  {
    Serial.println("Yanıcı gaz algılandı! Çöp Yanabilir");
    Blynk.notify("Yanıcı gaz algılandı! Çöp Yanabilir");
    sendEmail();
  }
  else{
    Serial.println("Yanıcı gaz seviyesi normal! Çöp güvenli.");
    Blynk.notify("Yanıcı gaz seviyesi normal! Çöp güvenli.");
  }
  
  delay(200); 
}


/* ThingSpeak Field Yazma İşlemi **/
void upload()
{
   x= ThingSpeak.writeField(channelID,field_no, garbageLevel,writeAPIKey); //değeri gönder 
  //if (x == 200)Serial.println("Veri güncellendi..");
}

void sendEmail(){
    smtp.debug(1);
    ESP_Mail_Session session;
   
    session.server.host_name = SMTP_server ;
    session.server.port = SMTP_PORT;
    session.login.email = AUTHOR_EMAIL;
    session.login.password = AUTHOR_PASSWORD;
    session.login.user_domain = "";
 
    SMTP_Message message;
    message.sender.name = "ESP8266";
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "DİKKAT ! Çöp kutusu yanıyor!"; //konu
    message.addRecipient("Beyzanur Odabas",RECIPIENT_EMAIL);
    if (!smtp.connect(&session))
        return;
    if (!MailClient.sendMail(&smtp, &message))
        Serial.println("Error sending Email, " + smtp.errorReason());
}

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(led1, OUTPUT); //kırmızı d5
  pinMode(led2, OUTPUT); //sarı d6
  pinMode(led3, OUTPUT); //yesil d7
  
  WiFi.mode(WIFI_STA);
  wifiSetup();
  ThingSpeak.begin(client);                 //  ThingSpeak client sınıfı  başlatılıyor
  Blynk.begin(auth, WLAN_SSID, WLAN_PASSWORD,"blynk.cloud",80);  //Arduino Blynk'e bağlanıyor
  timer.setInterval(1000L,gasSensor);
  
  capacity = total_height - hold_height;
  delay(100);
}

void loop() {

  delay(100);
  mesafeSensor();

  if (garbageLevel > 100)
  {
    garbageLevel = 100;
    Serial.println("Uyarı: Çöp doldu, 24 saat içerisinde boşaltılmalı. ");
    
    digitalWrite(led1, HIGH); //kırmızı ledi yaktık
    delay(4000);
    digitalWrite(led1, LOW);
    
    Serial.println("— — — — — — — — — — — — -");
    Serial.println("Çöpü boşaltınız….");
    
  }
  else if (garbageLevel > 20 && garbageLevel <100)
  {
    Serial.println("Uyarı: Çöp daha dolmadı. ");
    
    digitalWrite(led2, HIGH); //sarı ledi yaktık
    delay(4000);
    digitalWrite(led2, LOW);
  }
  else if(garbageLevel <= 20){
    if(garbageLevel < 0){
        garbageLevel = 0;
    }
    Serial.println("Uyarı: Çöp boş. ");
    
    digitalWrite(led3, HIGH); //yesil ledi yaktık
    delay(4000);
    digitalWrite(led3, LOW);
  }
  Serial.print("Çöp Seviyesi: ");
  Serial.print(garbageLevel);
  Serial.println("%");
  
  upload();

  
  Serial.println("!!!!!!!!!!! Çöpte yanıcı madde var mı tespit ediliyor !!!!!!!!!!!!!");
  timer.run();
  Blynk.run();
  //gasSensor();
  Serial.println("-------------------------------------------------------------");
 
}
