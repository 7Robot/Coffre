/*
 *  Code du coffre de semaine de bar 7Robot
 *  fait par Arthur Valiente en novembre 2012
 *  Elie est passé par la ? :D
 *  
 *  Réf adapteur écran : CLCD420C
 * 
 *  Branchements Arduino :
 *  Pins   /   Utilities
 *  13     /   Led Rouge
 *  [12;5] /   Digicode ( /!\ Sens du digicode_Vue de dessous. Pin à gauche du Digicode = pin 12, pin à droite = pin 5 )
 *  4      /   Led Jaune
 *  3      /   Servo-moteur (blanc => jaune)
 *  2      /   Led Verte
 *  1[TX]  /   LCD (fil vert)
 *
 *
 *
 *  Branchements Bus : (vue de face)
 *
 *  15........................2 1 0   
 *  31...........................16       
 *  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ 
 * |_|_|_|_|_|_|_|x|_|_|_|_|_|_|_|_|      Coté Digicode dans le coffre
 * |_|_|x|_|_|_|_|_|_|_|_|_|_|_|_|_|
 *  \                            /
 *   |                          |
 *   .                          .
 *   .                          .
 *   .                          .
 *   |                          |
 *  /                            \
 *  _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ _ 
 * |_|_|_|_|_|_|_|x|_|_|_|_|_|_|_|_| <=== Bus Arduino [Aref, GND, ..., 1[Tx], 0 [Rx]]
 * |_|_|x|_|_|_|_|_|_|_|_|_|_|_|_|_|
 * 
 *  15........................2 1 0   
 *  31...........................16  
 *
 *  -[0 ; 15 ] / {8}  =  Arduino (Cf Branchements Arduino)
 *  -{31 & 28} = +5Volt Servo & LCD (Respectivement fils Rouge & Marron)
 *  -{30 & 27} = GND    Servo & LCD (Respectivement fils Noir & Blanc)
 *  -{14} = GND Arduino = GND Leds
 */
 
#include <Servo.h> 

#define max(a,b)  ((a>b)?a:b)

// pins des LEDs
#define ROUGE   13
#define JAUNE   4
#define VERTE   2

//temps de pour éteindre le rétro-éclairage (millisecondes)
#define TIME_STOP_RETRO 20000

// pin du servo
#define SERVO  3
// angle ouverture
#define SERVO_OPEN  160
#define OPEN  true
// angle fermeture
#define SERVO_CLOSE 180
#define CLOSE  false

// longueur du code
#define CODE_LEN  2

// indice des caractères spéciaux
#define STAR  10
#define SHARP  11

// vitesse de clignotement
#define SPEED  50

// temps mort lors d’un code éroné
#define SLEEP  300

// modes
#define GAME    0
#define MASTER  1

// pause rétroéclairage
#define PAUSE 0
#define OK 1


#define MK_LEN    6
int masterkey[MK_LEN] = { 5, 3, 8, 4, 2, 7 };

Servo doorServo;

int colonne[] = { 11, 9, 11, 12, 9, 11, 12, 6, 11, 12, 6, 12 };
int ligne[] = { 5, 10, 10, 10, 8, 8, 8, 7, 7, 7, 5, 5 };

int solution[CODE_LEN];
int essai[max(CODE_LEN,MK_LEN)];
int time; //temps actuel
int reftime; //temps de début d'inactivité
int essaiPos = 0; // premier caractère
int lastKey = -1; // dernier caractère
int code_len = CODE_LEN;
int mode = GAME;
int pause = OK;

// Prototypes
void createCode();
void verifyCode();
boolean codeIsCorrect();
int readState(int c);
int getKey();
void setDoorState(boolean state);
void effacerEcran();
void Eclairage();
void led(int led);

void setup() {

  pinMode(VERTE, OUTPUT); // Diode Verte = OK
  pinMode(ROUGE, OUTPUT); // Diode Rouge ≠ OK
  pinMode(JAUNE, OUTPUT); // Diode Jaune = en attente du code
  
  doorServo.attach(SERVO);
  setDoorState(CLOSE);
  led(ROUGE);
  
  createCode();

  Serial.begin(2400); // Vitesse de l'écran
  delay(1000);
  effacerEcran();
  printPrompt();
  
  reftime = millis();
  
  randomSeed(analogRead(0));
}


void loop()
{
  int key = getKey();
  
  if (key == lastKey) {
    time = millis();
    if((time-reftime)>=TIME_STOP_RETRO){ // mise en veille
      pause = PAUSE;
      Eclairage();
    }
    return;
  }else{
    reftime = millis();
    pause = OK;
    Eclairage();
  }
  
  lastKey = key;
  reftime = millis();
  
  if (essaiPos == code_len) {
    if (key == -1) { // lorsque l’on relache la touche du dernier numéro
      verifyCode();
    }
  } else if (essaiPos == code_len + 1) {
    if (key == SHARP) { // Refermer l’écran
      mode = GAME;
      code_len = mode==GAME?CODE_LEN:MK_LEN;
      setDoorState(false);
      led(ROUGE);
      createCode();
      essaiPos = 0;
      effacerEcran();
      printPrompt();
    } 
  } else {
    if (key != -1) {
      if (key == SHARP) { // dièse
        mode ^= 1;
        if (mode == GAME) {
          code_len = CODE_LEN;
        } else {
          code_len = MK_LEN;
        }
        effacerEcran();
        printPrompt();
      } else if (key == STAR) { // étoile
        essaiPos = 0;
        effacerEcran();
        printPrompt();
        led(ROUGE);
      } else { // un chiffre
        led(VERTE);
        essai[essaiPos++] = key;
        if (mode == GAME) {
          Serial.write('0' + key);
        } else {
          Serial.write('*');
        }
      }
    } else if (essaiPos > 0) {
      led(JAUNE);
    }
  }
}

void printPrompt() {
  if (mode == GAME) {
    Serial.write(0x03);
    Serial.write(" \"*\" pour effacer");
    Serial.write(0x02);
    Serial.write("    Code : ");
  } else {
    Serial.write(0x03);
    Serial.write(" Appuyez sur # pour");
    Serial.write(0x04);
    Serial.write(" retourner au jeu !");
    Serial.write(0x01);
    Serial.write("  MasterKey : ");
    Serial.write(0x02);
    Serial.write("       ");
  }
}

void printFelicitation() {
  if (mode == GAME) {
    Serial.write(0x02);
    Serial.write("  Felicitation !");
    Serial.write(0x04);
    Serial.write(" Qboule pour 7Robot");
  } else {
    Serial.write(0x02);
    Serial.write("  Greeting Master !");
  }
}

void Eclairage(){
  if (pause == PAUSE){
      Serial.write(0x1B);
      Serial.write(0x62);
  }else{
      Serial.write(0x1B);
      Serial.write(0x42);
  }
}

void verifyCode() {
  if (codeIsCorrect()) {
    essaiPos++;
    effacerEcran();
    printFelicitation();
    setDoorState(OPEN);
    for (int i = 0 ; i < 10 ; i ++) {
      led(JAUNE);
      delay(SPEED);
      led(ROUGE);
      delay(SPEED);
      led(JAUNE);
      delay(SPEED);
      led(VERTE);
      delay(SPEED);
    }
  } else {
    essaiPos = 0;
    led(ROUGE);
    delay(SLEEP);
    effacerEcran();
    printPrompt();
  }
}

void createCode() {
  for (int i = 0 ; i < code_len ; i++) {
    //randomSeed(analogRead(0));
    solution[i] = random(0, 9);
  }
}

boolean codeIsCorrect() { // Teste la combinaison.
  int * code = mode==GAME?solution:masterkey;
  for (int i = 0 ; i < code_len ; i++) {
    if (code[i] != essai[i]) {
      return false; // Code incorrect
    }
  }
  return true; // Code correct
}

int readState(int c) // Test si la touche c est enfoncé
{
  // On passe toutes les pins en haute impédance
  for (int i = 5; i <= 12; i++) {
    pinMode(i, INPUT);
    digitalWrite(i, HIGH); // Pullup
  }
  
  pinMode(colonne[c], OUTPUT);
  digitalWrite(colonne[c], LOW);
  delayMicroseconds(1);
  return !digitalRead(ligne[c]);
}

int getKey() { // Donne la première touche enfoncée détectée.
  for(int i = 0; i <= 11; i++) {
    if(readState(i)) {
      return i;
    }
  }
  return -1; // aucune touche détectée
}

void setDoorState(boolean state)
{
  if (state) { // open
    doorServo.write(SERVO_OPEN);
  } else { // close
    doorServo.write(SERVO_CLOSE);
  }
}

void led(int led) {
  digitalWrite(JAUNE, led==JAUNE?HIGH:LOW);
  digitalWrite(ROUGE, led==ROUGE?HIGH:LOW);
  digitalWrite(VERTE, led==VERTE?HIGH:LOW);
}

void effacerEcran() {
  Serial.write(0x1B);
  Serial.write(0x43);
  delay(20); // La datasheet spécifie 15ms.
}
