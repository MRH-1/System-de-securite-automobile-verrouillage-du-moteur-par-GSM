#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// LCD I2C - adresse 0x27
LiquidCrystal_I2C lcd(0x27, 16, 2);

//GSM
SoftwareSerial mySerial(9, 5);  //SIM800L Tx & Rx is connected to Arduino #3 & #2

// Clavier 4x3
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {13, 8, 7, 6};
byte colPins[COLS] = {1, 4, 3};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Code secret
const char secretCode[] = "5585";
const byte codeLength = sizeof(secretCode) - 1;
char inputCode[10];
byte inputIndex = 0;
int attemptsLeft = 3;
bool locked = false;

const int BUTTON_PIN = 2;
bool started = false;

// IR
const int sensorPin = 12;


// moteur
// Définir les broches connectées à IN1 et IN2
const int IN1 = 10;  // PWM
const int IN2 = 11;  // PWM
int etatmoteur=0;

// Efface une ligne du LCD
void clearLine(int row) {
  lcd.setCursor(0, row);
  for (int i = 0; i < 16; i++) lcd.print(" ");
  lcd.setCursor(0, row);
}

// Fonction pour lire un SMS
void checkSMS() {
  
  if (mySerial.available()) {
    String sms = "";
    while (mySerial.available()) {
      char c = mySerial.read();
      sms += c;
      delay(10);
    }
    sms.toLowerCase();
    sms.trim();
    
 while(!(sms.indexOf("5585") != -1)){
    lcd.clear();
      lcd.print("CODE errone");
           mySerial.println("AT+CNMI=1,2,0,0,0");// Affichage automatique des nouveaux SMS
            updateSerial();
            delay(1000);
                  if (mySerial.available()) {
                      String sms = "";
                      while (mySerial.available()) {
                        char c = mySerial.read();
                        sms += c;
                        delay(10);
                      }
                  
                      sms.toLowerCase();
                      sms.trim();}}
    if (sms.indexOf("5585") != -1) {
      lcd.clear();
      lcd.print("CODE PAR SMS OK");
      analogWrite(IN1, 200);
      analogWrite(IN2, 0);
      etatmoteur = 1;
      delay(5000); // faire tourner le moteur 5 sec
    }
  }
}



void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
pinMode(sensorPin, INPUT); // définir le pin du capteur en entrée

 Serial.begin(9600); // ← Ajout pour activer le Serial Monitor
  mySerial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  
 pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

 // Configuration du SIM800L
 Serial.println("Initializing...");
  delay(1000);
  mySerial.println("AT");  //Once the handshake test is successful, it will back to OK
  updateSerial();
  mySerial.println("AT+CMGF=1");
   updateSerial();// Mode texte
  delay(1000);
 
}

  void updateSerial() {
  delay(500);
  while (Serial.available()) {
    mySerial.write(Serial.read());  //Forward what Serial received to Software Serial Port
  }
  while (mySerial.available()) {
    Serial.write(mySerial.read());  //Forward what Software Serial received to Serial Port
  }
      }

void loop() {
  int sensorValue = digitalRead(sensorPin); // lire la valeur du capteur
  
  if (!started) {
    if (digitalRead(BUTTON_PIN) == LOW && sensorValue == LOW) {
      delay(50);  // Anti-rebond
      if (digitalRead(BUTTON_PIN) == LOW && sensorValue == LOW) {
        started = true;
          // Affichage LCD
        lcd.clear();
        clearLine(0);
        lcd.print("Saisir code vous");
        clearLine(1);
        lcd.print("avez 3 tentatives");
        
        // Envoi SMS
    mySerial.println("AT+CMGS=\"+21699494256\""); // ← Ton numéro ici
     updateSerial();
    delay(1000);
    mySerial.println("Veuillez saisir le code d'accès."); // Message à envoyer
     updateSerial();
    mySerial.write(26); // Ctrl+Z pour envoyer le SMS
        delay(5000);
               mySerial.println("AT+CNMI=1,2,0,0,0");// Affichage automatique des nouveaux SMS
           updateSerial();
           delay(1000);
    }}
    return;
  }

  if (locked) {
    clearLine(0);
    lcd.print("CLAVIER BLOQUE !");
    return;
  }

  char key = keypad.getKey();
  
  if (key != NO_KEY) {
    if (key == '#') {
      inputCode[inputIndex] = '\0';
      lcd.clear();
      clearLine(0);
      lcd.print("Code: ");
      lcd.print(inputCode);
      clearLine(1);

      if (strcmp(inputCode, secretCode) == 0) {
        lcd.print("CODE VALIDE !");
         Serial.print(sensorValue);
        // Rotation avant (IN1 = PWM, IN2 = 0)
        analogWrite(IN1, 200);  // Vitesse (~80%)
          analogWrite(IN2, 0);
          etatmoteur=1;
          delay(5000);
         
        
      }
      else {
        attemptsLeft--;
        lcd.print("Incorrect:");
        lcd.print(attemptsLeft);
        lcd.print(" essai");
        inputIndex = 0;
        delay(2000);
        lcd.clear();
        if (attemptsLeft <= 0) {
          locked = true;
          clearLine(0);
          lcd.print("CLAVIER BLOQUE !");
        } else {
          clearLine(0);
          lcd.print("Reessayer code");
        }
      }
    }
    else if (key == '*') {  
      inputIndex = 0;
      lcd.clear();
      clearLine(0);
      lcd.print("Saisie effacee");
      clearLine(1);
      lcd.print("Recommencez:");
    }
    else {
      if (inputIndex < sizeof(inputCode) - 1) {
        inputCode[inputIndex++] = key;
        lcd.clear();
        clearLine(0);
        clearLine(1);
        for (byte i = 0; i < inputIndex; i++) {
          lcd.setCursor(i, 1);
          lcd.print("*");  // Masquer les chiffres
        }
      } else {
        clearLine(1);
        lcd.print("Max atteint !");
      }
    }
  }
   // Vérifie s'il y a un SMS contenant le code
  checkSMS();
  
  if ( etatmoteur==1 && sensorValue == HIGH){
              Serial.print(sensorValue);
               lcd.clear();
                clearLine(0);
                lcd.print("MOTEUR stop");
                analogWrite(IN1, 0);
                analogWrite(IN2, 0);
                etatmoteur=0;}



  }
