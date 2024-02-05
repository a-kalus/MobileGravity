#include <SoftwareSerial.h>

//weight sensor libraries
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

SoftwareSerial BTserial(8, 9); // RX | TX

int inByteDir;
int inBytePumpTime;

const long baudRate = 9600;
const int numRelays = 4;

const int PUMP_RELAY_PIN_1 = 7;
const int PUMP_RELAY_PIN_2 = 6;

const int ACTUATOR_RELAY_PIN = 12;

const int PUSH_BTN_PIN = 8;

//weight sensor pins:
const int HX711_dout = 10;
const int HX711_sck = 11;

boolean relayStates[numRelays];

enum pumpStates {
  IDLEMODE,
  IN, // pumping into proxy
  OUT, // emptying proxy
};

enum pumpStates pumpState;

long int motorStartTime;
const long int maxFillTime = 5200;
const long int maxDrainTime = 6000;
long int drainDur = 5200;
long int fillDur = 3200;
const int numLevels = 10;
const long int fillDurationsMono[numLevels] = {425, 850, 1275, 1700, 2125, 2550, 2975, 3400, 3825, 4250};
const long int drainDurationsMono[numLevels] = {425, 850, 1275, 1700, 2125, 2550, 2975, 3400, 3825, 4250};


boolean ejecting = false;
long int ejectionStart;
long int ejectionDur = 2000;

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);
//HX711 additional variables
const int serialPrintInterval = 99;
const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {
  Serial.begin(9600);
  Serial.print("Sketch:   ");   Serial.println(__FILE__);
  Serial.print("Uploaded: ");   Serial.println(__DATE__);
  Serial.println(" ");

  setupPumpSystem();
  setupPushBtn();
  setupActuator();
}

void loop() {

  if (Serial.available()) {
    inByteDir = Serial.read();
    Serial.println(inByteDir);
    if (Serial.available()) {
      inBytePumpTime = Serial.read();
      Serial.println(inBytePumpTime);
    }
    handleInput();
  }
  
  readPushBtn();
  repositionActuator();

  controlWaterFlow();
  
  checkMotorRuntime();
  
  delay(50);        // delay in between reads
}
void readPushBtn(){
    if (digitalRead(PUSH_BTN_PIN)==LOW){
    Serial.flush();
    Serial.println("C1");
  } else {
    Serial.flush();
    Serial.println("C0");}
  }
  
void setupPushBtn() { 
  pinMode(PUSH_BTN_PIN, INPUT);
  digitalWrite(PUSH_BTN_PIN, HIGH);}

void setupActuator() {
  pinMode(ACTUATOR_RELAY_PIN, OUTPUT);
  digitalWrite(ACTUATOR_RELAY_PIN, HIGH);
}

void setupPumpSystem() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PUMP_RELAY_PIN_1, OUTPUT);
  pinMode(PUMP_RELAY_PIN_2, OUTPUT);


  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PUMP_RELAY_PIN_1, HIGH);
  digitalWrite(PUMP_RELAY_PIN_2, HIGH);


  for ( int i = 0; i < numRelays; i++)
  {
    relayStates[i] = false;
  }

  pumpState = IDLEMODE;

  motorStartTime = -1 * maxFillTime;
  Serial.println(motorStartTime);
}



void controlWaterFlow(){
    if ((pumpState == OUT) && (millis() > (motorStartTime + drainDur))) {
    stopMotor();
    Serial.flush();
    Serial.println("FIN");
    }

    if ((pumpState == IN) && (millis() > (motorStartTime + fillDur))) {
    stopMotor();
    Serial.flush();
    Serial.println("FIN");

    }
  
  }

void repositionActuator() {
  if ((ejecting) && (millis() > ejectionStart + ejectionDur)){
    ejecting=false;
    digitalWrite(ACTUATOR_RELAY_PIN, HIGH);
  }

}

void checkMotorRuntime(){
    if (((relayStates[0] == true)) && (millis() > (motorStartTime + maxDrainTime))) {
    stopMotor();
    }
  
    if (((relayStates[1] == true)) && (millis() > (motorStartTime + maxFillTime))) {
    stopMotor();
    }
  }

void stopMotor() {
  digitalWrite(PUMP_RELAY_PIN_1, HIGH);
  relayStates[0] = false;

  digitalWrite(PUMP_RELAY_PIN_2, HIGH);
  relayStates[1] = false;

  pumpState = IDLEMODE;
  Serial.print("Motor stopped at: ");Serial.println(millis());
  
}

void setTimeOut(pumpStates newState){
  int level = inBytePumpTime-48;
  Serial.print("Level ");Serial.println(level);
  if (0 <= level && level <=numLevels-1){
    
    if (newState == OUT){
      drainDur = drainDurationsMono[level];Serial.print("Drain for ");Serial.println(drainDur);
    }
    else if (newState == IN){
      fillDur = fillDurationsMono[level];Serial.print("Fill for ");Serial.println(fillDur);
    }

  }
}

void handleInput() {

  switch (inByteDir) {
    case (int)'R':
      stopMotor();
      setTimeOut(IN);
      startPumping(IN);
      break;
    case (int)'r':
      stopMotor();
      setTimeOut(OUT);
      startPumping(OUT);
      break;
    case (int)'L':
      stopMotor();
      setTimeOut(IN);
      startPumping(IN);
      break;
    case (int)'l':
      stopMotor();
      setTimeOut(OUT);
      startPumping(OUT);
      break;
    case (int)'h':
      stopMotor();
      break;
    case (int)'H':
      stopMotor();
      break;
    case (int)'E':
      ejectController();
    default:
      break;
  }

}

void ejectController() {
  ejecting=true;
  ejectionStart = millis();
  digitalWrite(ACTUATOR_RELAY_PIN, LOW);
}



void startPumping(pumpStates newState) {
  float motorDelay = 0;
  switch (newState) {
    case IN:
      delay(motorDelay);
      digitalWrite(PUMP_RELAY_PIN_2, LOW);
      relayStates[1] = true;
      break;
    case OUT:
      delay(motorDelay);
      digitalWrite(PUMP_RELAY_PIN_1, LOW);
      relayStates[0] = true;
      break;

    default:
      // Statement(s)
      break;
  }
  pumpState = newState;
  motorStartTime = millis();
  Serial.print("New Pump State: ");Serial.print(getStateName(pumpState));Serial.print("; Motor started at: ");Serial.println(motorStartTime);
}









const char* getStateName(pumpStates state)
{
  switch (state)
  {
    case IDLEMODE: return "Idle; Idle";
    case IN: return "Right; In";
    case OUT: return "Right; Out";
  }
}
