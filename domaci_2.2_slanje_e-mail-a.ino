#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Base64.h>
#include <DHT11.h>

char ssid[] = "AndroidAP514A";  //ime WiFi mreze
char password[] = "drug1234";   //WiFi lozinka

const int LED_PIN = 21;  //LED pin
const int THM_PIN = 2;   //pin TH modula
const int BTN_PIN = 17;  // Pin na kojem je taster za slanje zahteva

unsigned int SMTP_PORT = 2525;            //port smtp servera
char smtpServer[] = "smtp.sendgrid.net";  //URL smtp servera
//username i password za prijavi na smtp server (u kodu base64):
char smtpLogin[] = "YXBpa2V5";
char smtpPassword[] = "U0cubDV1ZlJkWDVSWGVlNkxJRko2T0hyZy5Ub1llbTh0MFpiY3lRVUJOZzJ1cDNENGJCcDRWa0hDNmpsRGlXalpkc1JJ";
//domen posiljaoca mejla
char smtpFrom[] = "<esp32.bmu@gmail.com>";
//e-mail adresa primaoca mejla
char smtpTo[] = "<andjela.vaskovic1@gmail.com>";
//Podaci za zaglavlje mejla:
char mailTo[] = "To: Andjela Vaskovic <andjela.vaskovic1@gmail.com>";
char mailFrom[] = "From: esp32.bmu@gmail.com";
char mailSubject[] = "Subject: ESP32 email test";

float last_Email_Time;
float email_Interval=864000;

const unsigned int HTTP_PORT = 80;  //port veb servera
//FSM:
enum FsmStateType { DISCONNECTED,
                    CONNECTED,
                    DISCONNECTING };
FsmStateType fsmState = DISCONNECTED;
//objekat klase DHT11 i posle nje izmerena temperatura i vlaznost
DHT11 dht11(THM_PIN);
float temperature;
float humidity;
//TCP server i klijent:
WiFiServer server(HTTP_PORT);
WiFiClient client;

//Prijemni bafer i indeks u prijemnom baferu
char buffer[256];
int k = 0;
//dva poslenje primljena karaktera:
char rxChar;
char lastRxChar;

void connectToWiFi() {
  Serial.println("Povezivanje na WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Povezivanje...");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Povezan na WiFi");
  } else {
    Serial.println("Neuspešno povezivanje na WiFi");
  }


  // Wait for client to be ready

  while (!client.connect(smtpServer, SMTP_PORT)) {
    delay(500);
    Serial.println("Waiting for client to be ready...");
  }

  Serial.println("Client ready");
}

void sendEmail(float temperature, float humidity) {
  // Spajanje na SMTP server
  // Wait for client to be ready
  /*
  while (! client.connect(smtpServer, SMTP_PORT)) {
    delay(500);
    Serial.println("Waiting for client to be ready...");
  }
  Serial.println("Client ready");
  */
  // Slanje komandi za autentifikaciju
  if (!client.connect(smtpServer, SMTP_PORT)) {
    Serial.println("Neuspešno povezivanje na SMTP server");
    return;
  } else if (client.connect(smtpServer, SMTP_PORT)) {  //Pokusava da se poveze na SMTP server
    Serial.println("Povezano!");
    // EHLO komanda
    client.println("EHLO ");  //Salje EHLO komandu SMTP serveru
    delay(1000);
    // Autorizacija
    client.println("AUTH LOGIN");  // Salje komandu za autorizaciju na SMTP serveru
    delay(500);
    client.println(smtpLogin);  //Salje korisnicko ime u Base64 formatu na SMTP server
    delay(500);
    client.println(smtpPassword);  //Salje lozinku u Base64 formatu na SMTP server
    delay(500);


    // Pošiljalac
    client.println("MAIL FROM:" + String(smtpFrom));  //Salje komandu za primaoca na SMTP server
    delay(500);
    // Primalac
    client.println("RCPT TO:" + String(smtpTo));  //Salje komandu za primaoca na SMTP server
    delay(500);
    // Inicijalizacija prenosa poruke
    client.println("DATA");  //Salje komandu za pocetak slanja e-mail poruke na SMTP server
    delay(500);

    // Zaglavlje e-mail poruke
    client.println(mailTo);       //Salje zaglavlje e-mail poruke sa informacijama o primaocu
    client.println(mailFrom);     //Salje zaglavlje e-mail poruke sa informacijama o poisiljaocu
    client.println(mailSubject);  //Salje zaglavlje e-mail poruke sa temom
    client.println();

    // Telo e-mail poruke
    client.println("Parametri: temperatura i vlaznost vazduha iznose:");
    client.print("Temperatura = ");
    client.print(temperature);
    client.println(" °C");
    client.print("Vlaznost vazduha = ");
    client.print(humidity);
    client.println(" %");


    client.println(".");
    delay(100);
    client.println();
    delay(100);
    client.println("QUIT");
    delay(100);
    client.stop();  //Zavrsava komunikaciju sa SMTP serverom 
    Serial.println("E-mail poslat");

  } else {
    Serial.println("Greška prilikom povezivanja na SMTP server!");
  }


  // Prekid veze sa SMTP serverom
  client.println("QUIT");
  delay(100);

  // Zatvaranje veze
  client.stop();
 
}


void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  Serial.begin(115200);
 

  //display.setBrightness(0x0f);
  connectToWiFi();
  //udp.begin(localPort);
  sendResponse();
}

//slanje HTTP odgovora:
void sendResponse() {
  float temperature = dht11.readTemperature();
  float humidity = dht11.readHumidity();

  sendEmail(temperature, humidity);
}

void loop() {

  if (digitalRead(BTN_PIN) == LOW) {  
    sendResponse();                  
  }

  // Provera vremena za slanje e-maila
  if (millis() - last_Email_Time >= email_Interval) {  
    last_Email_Time = millis();                        
    sendResponse();                                    
  }
}