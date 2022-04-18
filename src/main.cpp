#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Adafruit_Sensor.h> 
#include <TinyGPS++.h>
#include "SoftwareSerial.h"
#include <ESP8266WebServer.h> 
#include <WiFiManager.h>
#include <MQ2.h>
#include <string.h>
#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
//-------------------------------------------------
#include <ESP_Mail_Client.h>
#include <TinyGPS++.h>
TinyGPSPlus gps;


const int rxPin = D1, txPin = D2; 
SoftwareSerial neo6m(rxPin, txPin);

//---------------- SMTP -------------------------- 
#define AUTHOR_EMAIL "lucyin744@gmail.com" //Enter the email address and password
#define AUTHOR_PASSWORD "Anonim74" //not enter your personal email here

#define RECIPIENT_EMAIL "agustuslam12@gmail.com" //Recipient's Email - Receiver's Email

#define SMTP_HOST "smtp.gmail.com"//Gmail SMTP Server Settings
#define SMTP_PORT 587 //SMTP port (TLS)

SMTPSession smtp;
WiFiClient client;

const char *server = "api.thingspeak.com";
String apikey = "P9BQWW0TYJAHQVMV"; //Write Api Key
char auth[] = "sHP3Qhl9_Hmlv-zWR5IP4oj0Eqo1ObE0"; //Enter the Auth code which was send by Blink

 
unsigned long myChannelNumber = 1570387;
const char * myWriteAPIKey = "P9BQWW0TYJAHQVMV"; 


#define DHTPIN D3        // Digital pin D3
#define Flame D6       // Digital pin D0
#define Buzzer D7       // Digital pin D0
#define DHTTYPE DHT11     // DHT 11
#define Smoke A0

DHT dht(DHTPIN, DHTTYPE);

int sensorMQ2;
int maxValueMQ2 = 450; // max asap (ppm)

// led Blynk
WidgetLED led1(V1);
WidgetLED led2(V2);
// data sensor suhu ke Blynk
WidgetLCD lcd(V4);

void smtpCallback(SMTP_Status status);

void setup()
{
  Serial.begin(115200); // See the connection status in Serial Monitor
  neo6m.begin(9600);

  WiFi.mode(WIFI_STA);
  WiFiManager WM;

  bool res;
  res = WM.autoConnect("Skripsi", "18112018");

  if(!res){
    Serial.println("Gagal Terhubung ke Internet");
  }else{
    Serial.println("Berhasil Terhubung Ke jaringan Internet");
  }


  Blynk.begin(auth, WiFi.SSID().c_str(), WiFi.psk().c_str());

  pinMode(Flame,INPUT);
  pinMode(Smoke, INPUT);
  pinMode(DHTPIN, INPUT);
  pinMode(Buzzer,OUTPUT);

  dht.begin();
  ThingSpeak.begin(client);
}

/* 
  * send_email_alert() function */
void send_email_alert(){
  //------------------------------------------------------------------
  boolean newData = false; 
  for (unsigned long start = millis(); millis() - start < 2000;){
    while (neo6m.available()){
      if (gps.encode(neo6m.read()))
      {newData = true;break;}} 
  }
  //------------------------------------------------------------------
  if(newData != true or gps.location.isValid() != 1) 
  {Serial.println("No Valid GPS Data is Found.");return;}
  //------------------------------------------------------------------
  newData = false; // false
  String latitude = String(gps.location.lat(), 6);
  String longitude = String(gps.location.lng(), 6);
  Serial.println("Latitude: "+ latitude);
  Serial.println("Longitude: "+ longitude);
  //return;

  //Enable the debug via Serial port | No debug = 0 | Start debug = 1
  smtp.debug(1); // 1
  smtp.callback(smtpCallback);
  //-------------------------------------------------
  ESP_Mail_Session session; //Declare the session config data
  //-------------------------------------------------
  //Set the session config
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;
  session.login.user_domain = "";
  //-------------------------------------------------
  SMTP_Message message;  //Declare the message class
  //-------------------------------------------------
  //Set the message headers
  message.sender.name = "Rumah Lucy";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "PERINGATAN KEBAKARAN";
  message.addRecipient("agustuslam", RECIPIENT_EMAIL);
  //-------------------------------------------------
  //Send HTML message
  String htmlMsg = "<div style=\"color:#2f4468;\">";
  htmlMsg += "<h1>PERINGATAN!!! </h1>";
  htmlMsg += "<h1>TERDETEKSI ADANYA API </h1>";
  htmlMsg += "<h1>Rumah: Rumah Lucy</h1>";
  htmlMsg += "<h1>HP : 0823 ++++ ++++</h1>";
  htmlMsg += "<h1>Latitude: "+latitude+"</h1>";
  htmlMsg += "<h1>Longitude: "+longitude+"!</h1>";
  htmlMsg += "<h1><a href=\"http://maps.google.com/maps?q=loc:"+latitude+","+longitude+"\">Check Location in Google Maps</a></h1>";
  htmlMsg += "<p>* Sent from RUMAH LUCY</p></div>";
  message.html.content = htmlMsg.c_str();
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  if (!smtp.connect(&session)) /* Connect to server with the session config */
    return;

  if (!MailClient.sendMail(&smtp, &message)) /* Start sending Email and close the session */
    Serial.println("Error sending Email, " + smtp.errorReason());
}

/* *
 * smtpCallback() function
 * */
/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status){
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success()){
    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failled: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++){
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
    }
    Serial.println("----------------\n");
  }
}


void loop()
{
  Blynk.run(); // Initiates Blynk

  float h = dht.readHumidity();
  float t = dht.readTemperature(); 
  int readFlame = digitalRead(Flame);
  sensorMQ2 = analogRead(Smoke);
  Blynk.virtualWrite(V5, sensorMQ2);
  
  lcd.clear();
  Blynk.virtualWrite(V4, t);  //V5 is for Humidity
  lcd.print(0,0 ,"Temp :");
  lcd.print(6,0,t);
  lcd.print(0,1, "Humadity :");

  Blynk.virtualWrite(V4, h);  //V6 is for Temperature
  lcd.print(10,1,h);


   // mengirim Data ke By8link dan nontifikasi Email
  if(readFlame == LOW){
    led2.on();
    led1.off();
    digitalWrite(Buzzer, HIGH);
    Blynk.notify("Alert : Terdeteksi Adanya Api"); 
    Serial.println("Terdeteksi Adanya API");
    send_email_alert();
    Serial.println("<---------------------->");
    Serial.print("\n\n");
  }else if(sensorMQ2 >= maxValueMQ2){
    led2.on();
    led1.off();
    digitalWrite(Buzzer, HIGH);
    Blynk.notify("Alert : Terdeteksi Asap"); 
    Serial.print("Sensor Asap : ");
    Serial.print(sensorMQ2);
    Serial.println("Terdeteksi API");
    send_email_alert();
    Serial.print("<---------------------->");
    Serial.print("\n\n");
  }else if(t >= 45 ){
    led2.on();
    led1.off();
    Blynk.notify("Alert : Terdeteksi suhu rungan tinggi");
    Serial.println("Temperature : ");
    Serial.print(t);
    Serial.println("Terdeteksi suhu rungan tinggi");
    Serial.println("<---------------------->");
    Serial.print("\n\n");
  }
  else{
    digitalWrite(Buzzer, LOW);
    led1.on(); // lampu on (Hijau)
    led2.off(); // lampu off (Merah)
  } 

 
  // Mengirimkan data ke ThinkSpeak
  if (client.connect(server, 80)) {
    String postStr = apikey;
    postStr += "&field1=";
    postStr += String(t);
    postStr += "&field2=";
    postStr += String(h);
    postStr += "&field3=";
    postStr += String(sensorMQ2);
    postStr += "&field4=";
    postStr += String(readFlame);
    postStr += "\r\n\r\n\r\n";

    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + apikey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
  }
  client.stop();
  delay(10000);
  
}
