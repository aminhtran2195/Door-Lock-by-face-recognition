#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include "PubSubClient.h"
#include "MyMPR121.h"
#include "string.h"
#include <WiFi.h>


#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18
#define TFT_CS   15  // Chip select control pin
#define TFT_DC    2  // Data Command control pin
#define TFT_RST   4  // Reset pin (could connect to RST pin)
#define irqpin   13
#define relay_pin 12
#define mag_pin 16

const char* ssid = "HelloWorld";
const char* wifipass = "12344321";

const char* mqtt_server = "nightkoala963.cloud.shiftr.io";

//const char* mqtt_server = "minibroker.cloud.shiftr.io";
const char* username    = "nightkoala963";
const char* mqttpass    = "ijfMV2BbIpZgvPBG";

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
bool volatile getinput = false;
bool volatile needupdate_screen = false;

MPR121 Mykeypad;
hw_timer_t *timer1 = nullptr;

#define PASSWORD_MAXLENGTH 8

char Pwd_truth[] = "123456";
char Pwd_input[PASSWORD_MAXLENGTH+1] = "";
WiFiClient espClient;
PubSubClient client(espClient);

#define SCREEN_PASSINPUT 0
#define SCREEN_STANDBYSCREEN 1
#define SCREEN_YESNO 2 
#define SCREEN_FACEREG 3

#define LOCKING 0
#define WAITING 1
#define OPENING 2
#define ACCESSED 3

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

uint8_t lck_stt = LOCKING;
uint8_t scr_stt = SCREEN_STANDBYSCREEN;
volatile long Tcnt_100ms = 0; 
unsigned long anounce_time = 0;
unsigned long pingtime = 0;
bool openlock = false;

void IRAM_ATTR IRQ_Getkeychar(){
  getinput = true;
}

void IRAM_ATTR Timer1ISR(){
  portENTER_CRITICAL(&mux);
  Tcnt_100ms ++;
  portEXIT_CRITICAL(&mux);
}

/* ------------ declare Update function --------------*/
void HandleInput_Pwdscreen(char key, uint8_t res);
void HandleInput_Standbyscreen(uint8_t key);
void HandleInput_YesNoscreen(uint8_t key);
void Display_StandbyScreen();
void Display_PwdInputScreen();
void DisplayUpdate_PwdInputScreen();
void Handle_Lock();
void WifiSetup();
void Announce_Fail();
void Announce_Success();
void Display_FaceRegScreen();
/* ------------ end declare function --------------*/

void tftPrintTest() {
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 30);
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(2);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(3);
  tft.println("Hello World!");
  tft.setTextColor(ST77XX_BLUE);
  tft.setTextSize(4);
  tft.print(1234.567);
  delay(1500);
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(0);
  tft.println("Hello World!");
  
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_GREEN);
  tft.print(8675309, HEX); // print 8,675,309 out in HEX!
  tft.println(" Print HEX!");
  tft.println(" ");
  tft.setTextColor(ST77XX_WHITE);
  tft.println("Sketch has been");
  tft.println("running for: ");
  tft.setTextColor(ST77XX_MAGENTA);
  tft.print(millis() / 1000);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(" seconds.");
}

void setup(void) {
  
  Serial.begin(115200);
   // Setup WiFi
  WifiSetup();
   // Setup MQTT
  client.setServer(mqtt_server,1883);
  client.setCallback(callback);
  // OR use this initializer if using a 1.8" TFT screen with offset such as WaveShare:
  tft.initR(INITR_BLACKTAB);        // Init ST7735S chip
  tft.fillScreen(ST77XX_BLACK); 
  Wire.begin(-1,-1,400000);
  Mykeypad.InitConfig();
  Serial.println(F("Initialized"));
  tft.fillScreen(ST77XX_BLACK);

  Serial.println("done");
  pinMode(irqpin,INPUT_PULLUP);
  pinMode(relay_pin,OUTPUT);
  pinMode(mag_pin,INPUT_PULLUP);
  digitalWrite(relay_pin,LOW);
  attachInterrupt(irqpin,IRQ_Getkeychar,FALLING);
  //tftPrintTest();
  Display_StandbyScreen();
  timer1 = timerBegin(1,80,true);
  timerAttachInterrupt(timer1,&Timer1ISR,true);
  timerAlarmWrite(timer1,100000,true);
  timerAlarmEnable(timer1);
  delay(1000);
}

void loop() {
  if(millis() - pingtime > 1000){
    MQTTConnect();
    client.loop();
    pingtime = millis();
  }
  HandleInput();
  HandleDisplay();
  Handle_Lock();  
}
/*---- SETUP ------*/
void WifiSetup(){
  Serial.println("Start connecting to " + String(ssid));
  WiFi.begin(ssid,wifipass);
  while(WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println("Wifi connected");
}

void MQTTConnect(){
  while (!client.connected()) {
      Serial.println("Trying connect to MQTT Broker..");    
      if (client.connect("ESP32",username,mqttpass,"lastwill",0,false,"Thiet bi offline")) {
        Serial.println("Connected.");
        // subscribe to topic
        client.subscribe("control");
        client.subscribe("anouncetoESP");
        client.subscribe("result");
       
        break;
      }
      delay(100);
  }
}

/*----- help function -----*/
void ClearPassword(){
  for(uint8_t i=0;i<=PASSWORD_MAXLENGTH;i++){
    Pwd_input[i] = '\0';
  }
}

bool Action_AddNumber(char num){
  uint8_t len = strlen(Pwd_input);
  if (len<PASSWORD_MAXLENGTH-1) {
    Pwd_input[strlen(Pwd_input)] = num;
    return false;
  } 
  else return true;
}

void ActionComplete(){
  if(!strcmp(Pwd_input,Pwd_truth)){
    Serial.println("Mat khau dung ");
    openlock = true;
    Announce_Success();
  }
  else {
    Serial.println("Mat khau sai ");
    Announce_Fail();
    openlock = false;
  }
  ClearPassword();
}

char GetInputChar(uint8_t *res){
  if(getinput == true){
    //keychar = Mykeypad.getKey();
    *res = Mykeypad.GetState(MULTICLK_OFF);
    getinput = false;
    return Mykeypad.getKey();
  }
  return '\0';
}

void HandleInput(){
  uint8_t res;
  char key = GetInputChar(&res);
  if(key != '\0'){
    //Serial.print("Phim vua nhap: " + String(key) + "\n");
    switch (scr_stt)
    {
    case SCREEN_STANDBYSCREEN:
      HandleInput_Standbyscreen(key);
      break;
      
    case SCREEN_PASSINPUT:
      /* code */
      HandleInput_Pwdscreen(key,res);
      break;

    case SCREEN_YESNO:
      HandleInput_YesNoscreen(key);
      break;
    default:
      break;
    }
  }
  else {
    // HandleTimeout();
  }
}

void HandleDisplay(){
  if(needupdate_screen){
    switch(scr_stt){
      case SCREEN_STANDBYSCREEN:
      break;
      case SCREEN_PASSINPUT:
        DisplayUpdate_PwdInputScreen();
      break;
    }
  }
  else // xu ly time out
  {}
}

void HandleInput_Pwdscreen(char key, uint8_t res){
  if((key != '*') && (key != '#')) {
    //Serial.println("add number");
    if (Action_AddNumber(key)) {
      ActionComplete();
    }
  }
  else if(key == '*') {
    do{
      res = Mykeypad.GetState(MULTICLK_OFF);
      delay(2);
    }while(res == NONE);
    if(res == PRESSED) {
      uint8_t len = strlen(Pwd_input);
      if (len != 0)  Pwd_input[len-1] = '\0';
      //Serial.println("backspace");
    }
    else {
      ClearPassword();
      //Serial.println("delete");
    }
  }
  else if (key == '#') {
    if(strlen(Pwd_input) == 0) { 
      ClearPassword();
      // quay lai man hinh chinh
    }
    else ActionComplete();
  }
  needupdate_screen = true;
  Serial.print("\nMat khau vua nhap: ");
  Serial.println(Pwd_input);
}

void HandleInput_Standbyscreen(uint8_t key){
  if(key != '#')
    Display_PwdInputScreen();
  else{
    client.publish("active","1");
    Display_FaceRegScreen();
  }
}

void HandleInput_YesNoscreen(uint8_t key){
  if(key == '*') Display_StandbyScreen();
  else if (key == '#') Display_PwdInputScreen();
}

void Display_FaceRegScreen(){
  scr_stt = SCREEN_FACEREG;
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(10,10);
  tft.println("    VAO GIAO DIEN \n NHAN DIEN KHUON MAT");
  tft.print("\n HAY DUA MAT VAO GAN CAMERA");
}

void Display_PwdInputScreen(){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(15,5);
  tft.print("NHAP MAT\n   KHAU");
  tft.drawRect(tft.width()/2 - 50,tft.height()/2 -12,96,24,ST77XX_RED);
  scr_stt = SCREEN_PASSINPUT;
}

void Display_StandbyScreen(){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(15,5);
  tft.print(" This is\nStandby screen");
  scr_stt = SCREEN_STANDBYSCREEN;
}

void Display_WaitingScreen(){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(10,30);
  tft.print("Server dang xu ly ......");  

}

void DisplayUpdate_PwdInputScreen(){
  tft.setCursor(tft.width()/2 - 35,tft.height()/2 - 8);
  tft.setTextSize(1);
  tft.fillRect(tft.width()/2 - 49,tft.height()/2 -10,94,20,ST77XX_BLACK);
  for(uint8_t i=0;i<8;i++){
    if(Pwd_input[i] != '\0') tft.print("*");
    else tft.print(" ");
  }
  needupdate_screen = false;
}

void Handle_Lock(){
  switch (lck_stt){
  case LOCKING:
    if(openlock) {
      digitalWrite(relay_pin,HIGH);
      client.publish("door/status", "open");
      lck_stt = WAITING;
      portENTER_CRITICAL(&mux);
      Tcnt_100ms =0;
      portEXIT_CRITICAL(&mux);
      openlock = false;
    }
    break;
  case WAITING:
    //Serial.println("tcnt ex = "+(String)(Tcnt_100ms_ex));
    if(digitalRead(mag_pin)){
      lck_stt = OPENING;
      portENTER_CRITICAL(&mux);
      Tcnt_100ms =0;
      portEXIT_CRITICAL(&mux);
    }
    else if(Tcnt_100ms > 50) {
      lck_stt = LOCKING;
      digitalWrite(relay_pin,LOW);
      portENTER_CRITICAL(&mux);
      Tcnt_100ms =0;
      portEXIT_CRITICAL(&mux);
    }
    break;
  case OPENING:
    if(!digitalRead(mag_pin) && (Tcnt_100ms > 20)){
      lck_stt = ACCESSED;
      portENTER_CRITICAL(&mux);
      Tcnt_100ms =0;
      portEXIT_CRITICAL(&mux);
    }
    break;
  case ACCESSED:
    if(!digitalRead(mag_pin) && (Tcnt_100ms > 20)){
      lck_stt = LOCKING;
      digitalWrite(relay_pin,LOW);
      portENTER_CRITICAL(&mux);
      Tcnt_100ms =0;
      portEXIT_CRITICAL(&mux);
      openlock = false;
    }
    break;
  default:
    break;
  }
}

void Announce_Fail(){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(20,10);
  tft.print(" Sai mat khau");
  tft.setCursor(22,25);
  tft.print(" Nhap lai?");
  openlock = false;
  scr_stt = SCREEN_YESNO;
}

void Announce_Success(){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(15,5);
  tft.print(" SUCCESS");
  openlock = true;
  client.publish("thongbao","accessed");
  client.publish("InputPwd","1");
  delay(1500);
  Display_StandbyScreen();
}

void Announce_FaceRegFail(){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(1);
  tft.setCursor(10,30);
  tft.print("- KHUON MAT KHONG XAC DINH -");
  delay(3000);
  Display_StandbyScreen();
}

void Announce_FaceRegSuccess(String tempmsg){
  tft.fillScreen(ST77XX_BLACK); 
  tft.setTextColor(ST77XX_RED);
  tft.setTextSize(2);
  tft.setCursor(10,30);
  tft.println("Wellcomed");
  tft.print(tempmsg);
  delay(2000);
  openlock = true;
  Display_StandbyScreen();
}


/* ------ MQTT HANDLE ---------*/

void callback(char* topic, byte* payload, unsigned int length) {
  String msg = "";
  for (uint8_t i=0;i<length;i++) {
    msg += String((char)payload[i]);
  }
  //Serial.println(msg);
  if(String(topic) == "control") {
    if(msg == "1")  {
      openlock = true;
      client.publish("thongbao","opened");
    }
    else {
      client.publish("thongbao","closed");
      digitalWrite(relay_pin,LOW);
    }
  }
  else if((String)topic == "announcetoESP"){
    Display_WaitingScreen();
  }
  else if ((String)topic == "result") {
    if(msg == "F") Announce_FaceRegFail();
    else Announce_FaceRegSuccess(msg); 
  }

}