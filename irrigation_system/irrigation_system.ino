/*
 * Patryk Grzelewski
 * Smart automatic irrigation system with GSM module usage
 * 02.2021
 */

#include <SoftwareSerial.h>

SoftwareSerial sim(10, 11);

#define waterPump 8
#define pumpLED 2
#define pumpButton 13

#define humiditySensor A5
#define lightSensor A4

#define waterLevelEcho 6
#define waterLevelTrig 7

#define redLED 3
#define greenLED 4
#define blueLED 5

#define tankHeight 14

const unsigned long eventTime = 60000 * 60; // Czas w ms przez który zostanie wysłane maksymalnie jedno powiadomienie
unsigned long previousTime = 0; // Czas wysłania ostatniego powiadomienia

char received_SMS; 
int infoOK = -1; // -1 oznacza brak pożądanego fragmentu wiadomości SMS

void setup() {
  Serial.begin(9600);
  sim.begin(9600);

  Serial.println("Initializing..."); 
  delay(5000); // odczekanie w celu zarejestrowania się modułu GSM w sieci

  SendNotification("Your irrigation system is turned on");

  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  
  pinMode(waterPump, OUTPUT);
  pinMode(pumpLED, OUTPUT);
  pinMode(pumpButton, INPUT_PULLUP);

  pinMode(waterLevelTrig, OUTPUT); 
  pinMode(waterLevelEcho, INPUT);

  //Tryb odbierania SMS
  ReceiveMode(); 
}

void loop() {
    
  int humidity = analogRead(humiditySensor);
  humidity = map(humidity,1023,0,0,100); 

  int lightIntensity = analogRead(lightSensor);
  lightIntensity = map(lightIntensity,1023,0,100,0);

  digitalWrite(waterLevelTrig, LOW);
  delayMicroseconds(2);
  digitalWrite(waterLevelTrig, HIGH);
  delayMicroseconds(10);
  digitalWrite(waterLevelTrig, LOW);

  long duration = pulseIn(waterLevelEcho, HIGH);
  int fillLevel = tankHeight - duration*0.034/2;
  int fill = (100*fillLevel)/tankHeight;

  String buforSMS;
  
  while(sim.available()>0) {       //Jeżeli moduł GSM wysłał coś do Arduino
        received_SMS = sim.read();  //Odczytanie treści SMSa
        Serial.print(received_SMS);   //Wypisanie treści SMSa w monitorze portu szeregowego  
        buforSMS.concat(received_SMS); //Dołączenie treści SMSa do buforu
        infoOK = buforSMS.indexOf("INFO"); //Sprawdzenie czy SMS zawiera "INFO"
    }

  if(infoOK != -1) {
      String text = "Humidity: " + String(humidity) + "%\n" + "Light: " + String(lightIntensity) +
      "%\n" + "Tank fill: " + String(fill) + "%\n";
      
      SendNotification(text); //Wysłanie SMSa z informacją zwrotną
      infoOK=-1;
      
      ReceiveMode(); //Powrót do trybu odbierania SMS
  }

  unsigned long currentTime = millis(); //Aktualny czas, użyty w celu wysłania powiadomienia raz na jakiś czas

  //Sprawdzanie poziomu wody
  if(fill > 60){
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, HIGH);
  }
  else if(fill > 25){
    digitalWrite(redLED, LOW);
    digitalWrite(greenLED, HIGH);
    digitalWrite(blueLED, LOW);
  }
  else
  {
    digitalWrite(redLED, HIGH);
    digitalWrite(greenLED, LOW);
    digitalWrite(blueLED, LOW);
    if (currentTime - previousTime > eventTime) {
      SendNotification("Low water level. Your pump has stopped working.");
      previousTime = currentTime;
      currentTime = millis();
    }
  }

  //PrintInfo(humidity, lightIntensity, fill);

  //Sterowanie pompką
  if(digitalRead(pumpButton) == LOW) {
    digitalWrite(waterPump, HIGH);
    digitalWrite(pumpLED, HIGH);
  } 
  else if(humidity < 40 && fill > 25) {
    digitalWrite(waterPump, HIGH);
    digitalWrite(pumpLED, HIGH);
    delay(2000);
    digitalWrite(waterPump,LOW);
    digitalWrite(pumpLED, LOW);
    delay(10000);
  }
  else{
    digitalWrite(waterPump,LOW);
    digitalWrite(pumpLED, LOW);
  }
}

void updateSerial() //Aktualizacja portu szeregowego
{
  delay(500);
  while (Serial.available())
  {
    sim.write(Serial.read());
  }
  while(sim.available()) 
  {
    Serial.write(sim.read());
  }
}

void ReceiveMode(){       //Ustawienie modułu GSM w stan otrzymywania wiadomośći
  
  sim.println("AT"); //Jeżeleli wszystko jest w porządku wypisze "OK"
  updateSerial();
  sim.println("AT+CMGF=1"); //Ustawienie trybu tekstowego
  updateSerial();
  sim.println("AT+CNMI=2,2,0,0,0"); //Koniguracja SIM800L w jaki sposób ma obsłużyć otrzymanego SMSa
  updateSerial();
}

void SendNotification(String text){     //Wysłanie SMSa
  sim.println("AT+CMGF=1"); //Ustawienie trybu tekstowego
  updateSerial();
  sim.println("AT+CMGS=\"+xxxxxxxxxxx\""); //Numer telefonu
  updateSerial();
  sim.print(text); //text jest treścią SMSa
  updateSerial();
  sim.write(26); //Informuje moduł o możliwości SMSa
  delay(5000); //Opóźnienie w celu eliminacji zakłóceń w czujniku odległości
}

void PrintInfo(int humidity,int lightIntensity,int fill){
  Serial.print("Wilgotność gleby: ");
  Serial.print(humidity);
  Serial.println("%");
  
  Serial.print("Nasłoneczenienie: ");
  Serial.print(lightIntensity);
  Serial.println("%");

  Serial.print("Wypełnienie zbiornika: ");
  Serial.print(fill);
  Serial.print("%");
  Serial.println(" ");

  Serial.println("");
}
