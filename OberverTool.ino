#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include "esp_sleep.h" 
#include <WiFi.h>
#include <esp_now.h>   
#include <esp_wifi.h>  
#include <WebServer.h>
#include <Update.h>
#include <DNSServer.h>
#include <Preferences.h> 

// ==========================================
// --- CONFIGURATION ---
// ==========================================

// Pins
const int PIN_I2C_SCL = 6;
const int PIN_I2C_SDA = 7;
const int PIN_BTN_ACTION = 3;  
const int PIN_BTN_MODE = 4;    
const int PIN_VIB_MOTOR = 10;  

// Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1 
const uint8_t OLED_ADDRESS = 0x3C;

// Brightness levels (0-255)
const uint8_t BRIGHTNESS_HIGH = 207; 
const uint8_t BRIGHTNESS_DIM = 1;    

// Power saving mode (times in milliseconds)
const unsigned long DIM_TIMEOUT = 5 * 60 * 1000;    
const unsigned long SLEEP_TIMEOUT = 15 * 60 * 1000; 

// Hardware behavior (times in milliseconds)
const int DEBOUNCE_DELAY = 400; 

// Vibration profiles (duration in milliseconds)
const unsigned long VIB_BOOT_PULSE = 200;
const unsigned long VIB_BOOT_PAUSE = 150;
const unsigned long VIB_START = 200;
const unsigned long VIB_CANCEL = 150;
const unsigned long VIB_WARNING = 400;
const unsigned long VIB_ALARM_PULSE = 400;
const unsigned long VIB_ALARM_PAUSE = 0;
const int VIB_ALARM_COUNT = 1; 

// Boot screen duration
const unsigned long BOOT_DELAY_BEFORE_VIB = 1000;
const unsigned long BOOT_DELAY_AFTER_VIB = 1500;

// OTA Service Mode settings
const char* AP_SSID = "ObserverTool";
const char* FIRMWARE_VERSION = "v2.0"; 
const byte DNS_PORT = 53;

// Program definitions { "Name", Duration (ms), Warning (ms, 0 = off) }
struct Program {
  String name;                 
  unsigned long durationMs;    
  unsigned long warningMs;     
};

const int NUM_PROGRAMS = 7;
Program programs[NUM_PROGRAMS] = {
  {"Serve", 3000, 0},             // Index 0: Standard-Startpunkt
  {"Timeout", 60000, 20000},      
  {"Break", 180000, 60000},       
  {"1.Medical", 300000, 60000},   
  {"2.Medical", 60000, 20000},    
  {"3.Medical", 15000, 0},
  {"Coin Toss", 0, 0}             
};

// ==========================================
// --- GLOBAL VARIABLES ---
// ==========================================

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DNSServer dnsServer;
WebServer server(80);

int currentMenuIndex = 0;
bool isRunning = false;
unsigned long startTime = 0;
bool warningTriggered = false; 

unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long lastActivityTime = 0;
unsigned long vibrateUntil = 0;

bool isDimmed = false;

// ==========================================
// --- ESP-NOW VARIABLES ---
// ==========================================
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
int scoreboardChannel = 1; 
bool espNowEnabled = true; 

typedef struct struct_message {
  bool start;
  unsigned long duration;
} struct_message;

// ==========================================
// --- FUNCTION DECLARATIONS ---
// ==========================================
void runServiceMode();
void runFlappyBall();
void runCoinToss();
void toggleEspNowMode(); 
void setDisplayBrightness(uint8_t contrast);
void drawHeader(String title);
void updateMenuDisplay();
void updateTimerDisplay(unsigned long remaining);
void triggerVibration(unsigned long duration);
void handleVibration(unsigned long currentMillis);
String getWebPage();
void setupESPNow();
void sendTimerCommand(bool start, unsigned long duration);

// ==========================================
// --- SETUP ---
// ==========================================

void setup() {
  pinMode(PIN_BTN_ACTION, INPUT_PULLUP);
  pinMode(PIN_BTN_MODE, INPUT_PULLUP);
  pinMode(PIN_VIB_MOTOR, OUTPUT);
  digitalWrite(PIN_VIB_MOTOR, LOW);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    for(;;);
  }
  setDisplayBrightness(BRIGHTNESS_HIGH);

  display.clearDisplay();
  display.display();

  // Gespeicherten Sync-Status laden
  Preferences prefsMain;
  prefsMain.begin("settings", true); 
  espNowEnabled = prefsMain.getBool("espnow", true); 
  prefsMain.end();

  // --- BOOT MENU LOGIC ---
  if (digitalRead(PIN_BTN_MODE) == LOW && digitalRead(PIN_BTN_ACTION) == LOW) {
    runServiceMode(); 
  } 
  else if (digitalRead(PIN_BTN_MODE) == LOW) {
    runFlappyBall();  
  }
  else if (digitalRead(PIN_BTN_ACTION) == LOW) {
    toggleEspNowMode(); // Interaktives Einstellungs-Menü
  }

  // Setup ESP-NOW nur starten/scannen, wenn die Funktion aktiv ist!
  if (espNowEnabled) {
    setupESPNow();
  }

  gpio_wakeup_enable((gpio_num_t)PIN_BTN_ACTION, GPIO_INTR_LOW_LEVEL);
  gpio_wakeup_enable((gpio_num_t)PIN_BTN_MODE, GPIO_INTR_LOW_LEVEL);
  esp_sleep_enable_gpio_wakeup();

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(4, 1); 
  display.println("OpenCirclz");
  display.drawLine(0, 16, SCREEN_WIDTH, 16, SSD1306_WHITE);

  display.setCursor(16, 25);
  display.println("OBSERVER");
  display.setCursor(40, 45);
  display.println("TOOL");
  
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print(FIRMWARE_VERSION);
  display.display();
  
  delay(BOOT_DELAY_BEFORE_VIB); 
  digitalWrite(PIN_VIB_MOTOR, HIGH); delay(VIB_BOOT_PULSE);
  digitalWrite(PIN_VIB_MOTOR, LOW);  delay(VIB_BOOT_PAUSE);
  digitalWrite(PIN_VIB_MOTOR, HIGH); delay(VIB_BOOT_PULSE);
  digitalWrite(PIN_VIB_MOTOR, LOW);
  delay(BOOT_DELAY_AFTER_VIB); 
  
  updateMenuDisplay();
  lastActivityTime = millis(); 
}

// ==========================================
// --- MAIN LOOP ---
// ==========================================

void loop() {
  unsigned long currentMillis = millis();
  handleVibration(currentMillis); 

  bool actionPressed = (digitalRead(PIN_BTN_ACTION) == LOW);
  bool modePressed = (digitalRead(PIN_BTN_MODE) == LOW);

  if (isRunning) {
    lastActivityTime = currentMillis; 
  } else {
    unsigned long idleTime = currentMillis - lastActivityTime;
    if (idleTime >= SLEEP_TIMEOUT) {
      display.ssd1306_command(SSD1306_DISPLAYOFF); 
      esp_light_sleep_start(); 
      
      display.ssd1306_command(SSD1306_DISPLAYON); 
      setDisplayBrightness(BRIGHTNESS_HIGH);
      isDimmed = false;
      currentMillis = millis();
      lastActivityTime = currentMillis;
      lastDebounceTime1 = currentMillis + 500; 
      lastDebounceTime2 = currentMillis + 500;
      return; 
    } else if (idleTime >= DIM_TIMEOUT && !isDimmed) {
      setDisplayBrightness(BRIGHTNESS_DIM);
      isDimmed = true;
    }
  }

  if ((actionPressed || modePressed) && isDimmed) {
    setDisplayBrightness(BRIGHTNESS_HIGH);
    isDimmed = false;
    lastActivityTime = currentMillis;
    lastDebounceTime1 = currentMillis + 250;
    lastDebounceTime2 = currentMillis + 250;
  }

  // --- CANCEL BUTTON (MODE) ---
  if (modePressed && (currentMillis - lastDebounceTime2 > DEBOUNCE_DELAY) && !isDimmed) {
    lastActivityTime = currentMillis;
    if (!isRunning) {
      currentMenuIndex = (currentMenuIndex + 1) % NUM_PROGRAMS;
      updateMenuDisplay();
    } else {
      isRunning = false;
      triggerVibration(VIB_CANCEL); 

      Program p = programs[currentMenuIndex];
      if (p.name != "Serve" && p.name != "Coin Toss") {
        sendTimerCommand(false, 0);
      }
      
      currentMenuIndex = 0; 
      updateMenuDisplay();
    }
    lastDebounceTime2 = currentMillis;
  }

  // --- START BUTTON (ACTION) ---
  if (actionPressed && (currentMillis - lastDebounceTime1 > DEBOUNCE_DELAY) && !isDimmed) {
    lastActivityTime = currentMillis;
    if (!isRunning) {
      if (programs[currentMenuIndex].durationMs == 0) {
        runCoinToss();
        lastActivityTime = millis();
      } else {
        isRunning = true;
        startTime = currentMillis;
        warningTriggered = false; 
        triggerVibration(VIB_START);    
        
        Program p = programs[currentMenuIndex];
        if (p.name != "Serve" && p.name != "Coin Toss") {
          sendTimerCommand(true, p.durationMs);
        }
      }
    } else {
      if (currentMenuIndex == 0) { 
        startTime = currentMillis; 
        triggerVibration(VIB_START); 
      }
    }
    lastDebounceTime1 = currentMillis;
  }

  if (isRunning) {
    unsigned long elapsed = currentMillis - startTime;
    Program p = programs[currentMenuIndex];

    if (elapsed >= p.durationMs) {
      isRunning = false;

      if (p.name != "Serve" && p.name != "Coin Toss") {
        sendTimerCommand(false, 0);
      }

      display.clearDisplay();
      drawHeader(p.name);
      
      display.setFont(&FreeSans18pt7b);
      display.setTextSize(1);
      int16_t x1, y1;
      uint16_t w, h;
      display.getTextBounds("TIME", 0, 0, &x1, &y1, &w, &h);
      int xPos = (SCREEN_WIDTH - w) / 2 - x1;
      display.setCursor(xPos < 0 ? 0 : xPos, 50);
      display.println("TIME");
      display.setFont();
      display.display();

      for(int i = 0; i < VIB_ALARM_COUNT; i++) {
        digitalWrite(PIN_VIB_MOTOR, HIGH); delay(VIB_ALARM_PULSE); 
        digitalWrite(PIN_VIB_MOTOR, LOW);  if(i < VIB_ALARM_COUNT - 1) delay(VIB_ALARM_PAUSE); 
      }
      
      currentMenuIndex = 0; 
      updateMenuDisplay(); 
      lastActivityTime = millis(); 
    } else {
      unsigned long remaining = p.durationMs - elapsed;
      if (p.warningMs > 0 && !warningTriggered && remaining <= p.warningMs) {
        triggerVibration(VIB_WARNING); 
        warningTriggered = true;
      }
      updateTimerDisplay(remaining);
    }
  }
}

// ==========================================
// --- NEU: INTERAKTIVES SETTINGS MENU ---
// ==========================================
void toggleEspNowMode() {
  Preferences prefs;
  prefs.begin("settings", false); 
  
  // Warten, bis der Action-Button vom Einschalten losgelassen wird
  while(digitalRead(PIN_BTN_ACTION) == LOW) { delay(10); } 

  bool inMenu = true;
  
  while (inMenu) {
    display.clearDisplay();
    drawHeader("SETTINGS");
    
    display.setTextSize(1);
    String status1 = "Scoreboard Sync:";
    int x1 = (SCREEN_WIDTH - (status1.length() * 6)) / 2;
    display.setCursor(x1 < 0 ? 0 : x1, 23);
    display.print(status1);

    display.setTextSize(2);
    String status2 = espNowEnabled ? "ON" : "OFF";
    int x2 = (SCREEN_WIDTH - (status2.length() * 12)) / 2;
    display.setCursor(x2 < 0 ? 0 : x2, 38);
    display.print(status2);

    display.setTextSize(1);
    String helpTxt = "[MODE] = Save & Exit";
    int x3 = (SCREEN_WIDTH - (helpTxt.length() * 6)) / 2;
    display.setCursor(x3 < 0 ? 0 : x3, 55);
    display.print(helpTxt);
    
    display.display();

    // Toggle mit Action Button
    if (digitalRead(PIN_BTN_ACTION) == LOW) {
      espNowEnabled = !espNowEnabled; // Ändert den Status
      prefs.putBool("espnow", espNowEnabled); // Speichert sofort
      digitalWrite(PIN_VIB_MOTOR, HIGH); delay(50); digitalWrite(PIN_VIB_MOTOR, LOW); // Kurzes Feedback
      
      while(digitalRead(PIN_BTN_ACTION) == LOW) { delay(10); } // Warten bis losgelassen
    }

    // Verlassen mit Mode Button
    if (digitalRead(PIN_BTN_MODE) == LOW) {
      digitalWrite(PIN_VIB_MOTOR, HIGH); delay(200); digitalWrite(PIN_VIB_MOTOR, LOW); // Langes Feedback
      while(digitalRead(PIN_BTN_MODE) == LOW) { delay(10); } // Warten bis losgelassen
      inMenu = false; // Bricht die Schleife ab und fährt mit dem Startvorgang fort
    }
    
    delay(50);
  }
  
  prefs.end();
  display.clearDisplay();
  display.display();
}

// ==========================================
// --- ESP-NOW FUNCTIONS ---
// ==========================================
void setupESPNow() {
  WiFi.mode(WIFI_STA);
  
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == "Scoreboard") { 
      scoreboardChannel = WiFi.channel(i);
      break;
    }
  }
  WiFi.scanDelete();

  WiFi.mode(WIFI_OFF); 
}

void sendTimerCommand(bool start, unsigned long duration) {
  if (!espNowEnabled) return; 

  WiFi.mode(WIFI_STA);
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(scoreboardChannel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  if (esp_now_init() == ESP_OK) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = scoreboardChannel;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    struct_message msg;
    msg.start = start;
    msg.duration = duration;
    
    for(int i = 0; i < 3; i++) {
      esp_now_send(broadcastAddress, (uint8_t *) &msg, sizeof(msg));
      delay(15); 
    }
    
    esp_now_deinit();
  }
  
  WiFi.mode(WIFI_OFF); 
}

// ==========================================
// --- FEATURE: COIN TOSS ---
// ==========================================
void runCoinToss() {
  unsigned long tossStart = millis();
  int delayTime = 20;
  bool isHeads = false;

  while(millis() - tossStart < 2500) {
    isHeads = (esp_random() % 2 == 0); 
    
    display.clearDisplay();
    drawHeader("Coin Toss");
    
    display.setFont(&FreeSans18pt7b);
    display.setTextSize(1);
    String text = isHeads ? "HEADS" : "TAILS";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    int xPos = (SCREEN_WIDTH - w) / 2 - x1;
    display.setCursor(xPos < 0 ? 0 : xPos, 50);
    display.print(text);
    display.setFont();
    display.display();
    
    digitalWrite(PIN_VIB_MOTOR, HIGH); delay(15); digitalWrite(PIN_VIB_MOTOR, LOW);
    
    delay(delayTime);
    delayTime += 8; 
  }
  
  isHeads = (esp_random() % 2 == 0);
  
  display.clearDisplay();
  drawHeader("WINNER!");
  
  display.setFont(&FreeSans18pt7b);
  display.setTextSize(1);
  String text = isHeads ? "HEADS" : "TAILS";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int xPos = (SCREEN_WIDTH - w) / 2 - x1;
  display.setCursor(xPos < 0 ? 0 : xPos, 50);
  display.print(text);
  display.setFont();
  display.display();
  
  digitalWrite(PIN_VIB_MOTOR, HIGH); delay(500); digitalWrite(PIN_VIB_MOTOR, LOW);
  delay(3000); 
  
  currentMenuIndex = 0; 
  updateMenuDisplay();
}

// ==========================================
// --- EASTEREGG: FLAPPY DOT ---
// ==========================================
void runFlappyBall() {
  Preferences prefs;
  prefs.begin("flappydot", false);
  int highscore = prefs.getInt("highscore", 0);

  while(digitalRead(PIN_BTN_ACTION) == LOW || digitalRead(PIN_BTN_MODE) == LOW) { delay(10); }

  display.clearDisplay();
  drawHeader("Flappy Dot");
  
  display.setTextSize(2);
  String hsStr = "RECORD: " + String(highscore);
  int hsX = (SCREEN_WIDTH - (hsStr.length() * 12)) / 2;
  display.setCursor(hsX < 0 ? 0 : hsX, 25);
  display.print(hsStr);

  display.setTextSize(1);
  String psStr = "- PRESS START -";
  int psX = (SCREEN_WIDTH - (psStr.length() * 6)) / 2;
  display.setCursor(psX < 0 ? 0 : psX, 50);
  display.print(psStr);
  
  display.display();

  while(true) {
    if(digitalRead(PIN_BTN_MODE) == LOW) { 
      prefs.end(); 
      return; 
    }
    if(digitalRead(PIN_BTN_ACTION) == LOW) break; 
    delay(10);
  }
  
  while(digitalRead(PIN_BTN_ACTION) == LOW) { delay(10); }

  float birdY = 32, birdVy = 0;
  int obsX = 128, obsW = 12, obsGap = 26, obsY = 32, score = 0;
  bool gameOver = false, lastBtnState = false;
  unsigned long lastFrame = 0;
  
  while(true) {
    if(millis() - lastFrame > 30) { 
      lastFrame = millis();
      
      if(!gameOver) {
        bool btnState = (digitalRead(PIN_BTN_ACTION) == LOW);
        if(btnState && !lastBtnState) {
          birdVy = -4.0; 
          digitalWrite(PIN_VIB_MOTOR, HIGH); delay(5); digitalWrite(PIN_VIB_MOTOR, LOW);
        }
        lastBtnState = btnState;
        
        birdVy += 0.5; 
        birdY += birdVy;
        obsX -= 3;     
        
        if(obsX < -obsW) {
          obsX = 128;
          obsY = (esp_random() % 34) + 15; 
          score++;
        }
        
        if(birdY + 3 >= 64 || birdY - 3 <= 0) gameOver = true;
        if(30 + 3 > obsX && 30 - 3 < obsX + obsW) { 
          if(birdY - 3 < obsY - obsGap/2 || birdY + 3 > obsY + obsGap/2) gameOver = true;
        }
        
        if(gameOver) {
          digitalWrite(PIN_VIB_MOTOR, HIGH); delay(400); digitalWrite(PIN_VIB_MOTOR, LOW);
          
          if (score > highscore) {
            highscore = score;
            prefs.putInt("highscore", highscore);
          }
        }
      } else {
        display.clearDisplay();
        drawHeader("GAME OVER");
        
        display.setTextSize(2);
        String scoreStr = "Score: " + String(score);
        int scX = (SCREEN_WIDTH - (scoreStr.length() * 12)) / 2;
        display.setCursor(scX < 0 ? 0 : scX, 25);
        display.print(scoreStr);
        
        display.setTextSize(1);
        String restStr = "- PRESS RESTART -";
        int restX = (SCREEN_WIDTH - (restStr.length() * 6)) / 2;
        display.setCursor(restX < 0 ? 0 : restX, 50);
        display.print(restStr);
        
        display.display();
        
        if(digitalRead(PIN_BTN_ACTION) == LOW) { 
          birdY = 32; birdVy = 0; obsX = 128; score = 0; gameOver = false;
          while(digitalRead(PIN_BTN_ACTION) == LOW) delay(10); 
        }
        if(digitalRead(PIN_BTN_MODE) == LOW) { 
          while(digitalRead(PIN_BTN_MODE) == LOW) delay(10);
          prefs.end();
          return; 
        }
        continue;
      }
      
      display.clearDisplay();
      display.setTextSize(1); display.setCursor(2, 2); display.print(score); 
      display.fillCircle(30, (int)birdY, 3, SSD1306_WHITE); 
      display.fillRect(obsX, 0, obsW, obsY - obsGap/2, SSD1306_WHITE); 
      display.fillRect(obsX, obsY + obsGap/2, obsW, 64 - (obsY + obsGap/2), SSD1306_WHITE); 
      display.display();
    }
  }
}

// ==========================================
// --- OTA SERVICE MODE ---
// ==========================================

const char web_page_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body{font-family:sans-serif;background:#222;color:#fff;text-align:center;padding:50px 20px;}
    input[type=file]{margin-bottom:20px;padding:10px;background:#333;border-radius:5px;}
    input[type=submit]{background:#f3ca20;color:#000;border:none;padding:15px 30px;font-size:18px;border-radius:5px;font-weight:bold;cursor:pointer;}
    .version{font-size:14px; color:#aaa; margin-bottom:30px;}
  </style>
</head>
<body>
  <h2>ObserverTool Update</h2>
  <div class='version'>Current Version: <b>%VERSION%</b></div>
  <p>Choose new Firmware (.bin):</p>
  <form method='POST' action='/update' enctype='multipart/form-data'>
    <input type='file' name='update' accept='.bin'><br><br>
    <input type='submit' value='Flash Update'>
  </form>
  <br><br>
  <h3>&copy; OpenCirclz - Moritz Knaebel</h3>
</body>
</html>
)rawliteral";

String getWebPage() {
  String html = String(web_page_html);
  html.replace("%VERSION%", FIRMWARE_VERSION); 
  return html;
}

void runServiceMode() {
  digitalWrite(PIN_VIB_MOTOR, HIGH); delay(600); digitalWrite(PIN_VIB_MOTOR, LOW);
  WiFi.softAP(AP_SSID);
  IPAddress IP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", IP);

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", getWebPage());
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "Update failed!" : "Update succesful! Device is restarting...");
    delay(1000);
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      display.fillRect(0, 18, SCREEN_WIDTH, 46, SSD1306_BLACK);
      display.setTextSize(1); display.setCursor(0, 30); display.println("Flashing Firmware..."); display.display();
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) { Update.printError(Serial); }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { 
        display.fillRect(0, 18, SCREEN_WIDTH, 46, SSD1306_BLACK);
        display.setTextSize(1); display.setCursor(0, 30); display.println("DONE! Restarting..."); display.display();
      }
    }
  });

  server.onNotFound([]() {
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  display.clearDisplay(); drawHeader("OTA UPDATE");
  display.setTextSize(1);
  display.setCursor(0, 22); display.print("WIFI: "); display.print(AP_SSID);
  display.setCursor(0, 36); display.print("IP:   "); display.print(IP);
  display.setCursor(0, 50); display.print("FW:   "); display.print(FIRMWARE_VERSION);
  display.display();

  while (true) {
    dnsServer.processNextRequest();
    server.handleClient();
    delay(1);
  }
}

// ==========================================
// --- HELPER FUNCTIONS ---
// ==========================================

void setDisplayBrightness(uint8_t contrast) {
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(contrast);
}

void triggerVibration(unsigned long duration) {
  vibrateUntil = millis() + duration;
  digitalWrite(PIN_VIB_MOTOR, HIGH);
}

void handleVibration(unsigned long currentMillis) {
  if (currentMillis >= vibrateUntil) {
    digitalWrite(PIN_VIB_MOTOR, LOW);
  }
}

void drawHeader(String title) {
  display.setTextSize(2); 
  display.setTextColor(SSD1306_WHITE);
  
  int xPos = (SCREEN_WIDTH - (title.length() * 12)) / 2;
  if (xPos < 0) xPos = 0;
  display.setCursor(xPos, 1);
  display.println(title);
  
  display.drawLine(0, 16, SCREEN_WIDTH, 16, SSD1306_WHITE);
}

void updateMenuDisplay() {
  display.clearDisplay();
  Program p = programs[currentMenuIndex];
  
  drawHeader(p.name);
  
  display.setFont(&FreeSans18pt7b);
  display.setTextSize(1);

  String timeString = "";
  if (p.durationMs == 0) {
    timeString = "START"; 
  } else if (p.durationMs >= 60000) {
    timeString = String(p.durationMs / 60000) + " m";
  } else {
    timeString = String(p.durationMs / 1000) + " s";
  }

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeString, 0, 0, &x1, &y1, &w, &h);
  int xPos = (SCREEN_WIDTH - w) / 2 - x1;
  if (xPos < 0) xPos = 0;

  display.setCursor(xPos, 50);
  display.print(timeString);
  
  display.setFont(); 
  display.display();
}

void updateTimerDisplay(unsigned long remaining) {
  display.clearDisplay();
  Program p = programs[currentMenuIndex];
  drawHeader(p.name);

  display.setFont(&FreeSans24pt7b);
  display.setTextSize(1);

  String timerString = "";
  String templateString = ""; 

  if (p.durationMs == 3000) {
    float secRemaining = remaining / 1000.0;
    timerString = String(secRemaining, 1);
    templateString = "8.8";
  } else {
    int mins = remaining / 60000;
    int secs = (remaining % 60000) / 1000;
    timerString = String(mins) + ":";
    if (secs < 10) timerString += "0";
    timerString += String(secs);
    templateString = (mins > 9) ? "88:88" : "8:88";
  }

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(templateString, 0, 0, &x1, &y1, &w, &h);
  
  int xPos = (SCREEN_WIDTH - w) / 2 - x1;

  display.setCursor(xPos < 0 ? 0 : xPos, 56);
  display.print(timerString);
  
  display.setFont(); 
  display.display();
}
