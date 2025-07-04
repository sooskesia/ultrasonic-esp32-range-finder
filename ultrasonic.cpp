#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// TFT Display pins
#define TFT_CS     5    
#define TFT_RST    4
#define TFT_DC     2
#define TFT_MOSI   23   
#define TFT_SCLK   18   

// HC-SR04 pins
#define TRIG_PIN   15   
#define ECHO_PIN   19   

// Initialize display with specific pins
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

// Radar settings
const int centerX = 80;
const int centerY = 120;
const int maxRadius = 60;
const int maxDistance = 200; // cm

// Sweep variables
int sweepAngle = 0;
int sweepDirection = 1;
int lastSweepAngle = 0;

// Detection zones
struct Detection {
  float distance;
  unsigned long timestamp;
  bool active;
  bool drawn;
};

Detection detections[10];
float lastDistance = -1;
bool gridDrawn = false;

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("=== ULTRASONIC RADAR STARTING ===");
  
  // Initialize HC-SR04
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  Serial.println("HC-SR04 initialized");
  
  // Manual reset sequence for display
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  digitalWrite(TFT_RST, LOW);
  delay(100);
  digitalWrite(TFT_RST, HIGH);
  delay(100);
  
  // Initialize TFT
  tft.initR(INITR_BLACKTAB);  
  Serial.println("Display initialized");
  
  tft.setRotation(1); // Landscape
  tft.fillScreen(ST77XX_BLACK);
  
  // Initialize detection array
  for(int i = 0; i < 10; i++) {
    detections[i].active = false;
    detections[i].drawn = false;
  }
  
  // Draw static elements once
  drawRadarGrid();
  gridDrawn = true;
  
  Serial.println("Radar ready!");
  delay(500);
}

void loop() {
  // Measure distance
  float distance = measureDistance();
  
  // Process detection
  if(distance > 0 && distance < maxDistance) {
    addDetection(distance);
  }
  
  // Only erase old sweep line
  eraseSweepLine(lastSweepAngle);
  
  // Update detections (fade old ones)
  updateDetections();
  
  // Draw new sweep line
  drawSweepLine(sweepAngle);
  
  // Update info only if distance changed significantly
  if(abs(distance - lastDistance) > 2 || lastDistance == -1) {
    displayInfo(distance);
    lastDistance = distance;
  }
  
  // Update sweep angle
  lastSweepAngle = sweepAngle;
  sweepAngle += sweepDirection * 4; // Faster sweep
  if(sweepAngle >= 60) {
    sweepAngle = 60;
    sweepDirection = -1;
  } else if(sweepAngle <= -60) {
    sweepAngle = -60;
    sweepDirection = 1;
  }
  
  delay(50); // Faster refresh
}

float measureDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if(duration == 0) return -1;
  
  return (duration * 0.034) / 2;
}

void drawRadarGrid() {
  tft.fillScreen(ST77XX_BLACK);
  
  // Draw range circles
  tft.drawCircle(centerX, centerY, maxRadius/4, ST77XX_GREEN);
  tft.drawCircle(centerX, centerY, maxRadius/2, ST77XX_GREEN);
  tft.drawCircle(centerX, centerY, (maxRadius*3)/4, ST77XX_GREEN);
  tft.drawCircle(centerX, centerY, maxRadius, ST77XX_GREEN);
  
  // Draw angle lines (60-degree cone)
  for(int angle = 60; angle <= 120; angle += 15) {
    int x = centerX + (maxRadius * cos(radians(angle)));
    int y = centerY - (maxRadius * sin(radians(angle)));
    tft.drawLine(centerX, centerY, x, y, ST77XX_GREEN);
  }
  
  // Draw main direction line (90 degrees = straight ahead)
  int mainX = centerX;
  int mainY = centerY - maxRadius;
  tft.drawLine(centerX, centerY, mainX, mainY, ST77XX_CYAN);
  
  // Distance labels
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(1);
  tft.setCursor(centerX + 2, centerY - maxRadius/4);
  tft.print(maxDistance/4);
  tft.setCursor(centerX + 2, centerY - maxRadius/2);
  tft.print(maxDistance/2);
  tft.setCursor(centerX + 2, centerY - (maxRadius*3)/4);
  tft.print((maxDistance*3)/4);
  tft.setCursor(centerX + 2, centerY - maxRadius + 5);
  tft.print(maxDistance);
  
  // Title (draw once)
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(35, 5);
  tft.print("ULTRASONIC RADAR");
  
  // Direction indicator
  tft.setCursor(75, 15);
  tft.print("FRONT");
}

void addDetection(float distance) {
  // Find empty or oldest slot
  int oldestIndex = 0;
  unsigned long oldestTime = detections[0].timestamp;
  
  for(int i = 0; i < 10; i++) {
    if(!detections[i].active) {
      oldestIndex = i;
      break;
    }
    if(detections[i].timestamp < oldestTime) {
      oldestIndex = i;
      oldestTime = detections[i].timestamp;
    }
  }
  
  // Erase old detection if it was drawn
  if(detections[oldestIndex].drawn) {
    eraseDetection(oldestIndex);
  }
  
  // Add new detection
  detections[oldestIndex].distance = distance;
  detections[oldestIndex].timestamp = millis();
  detections[oldestIndex].active = true;
  detections[oldestIndex].drawn = false;
  
  // Draw new detection
  drawDetection(oldestIndex);
}

void updateDetections() {
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 10; i++) {
    if(detections[i].active) {
      unsigned long age = currentTime - detections[i].timestamp;
      if(age > 3000) { // 3 second fade
        if(detections[i].drawn) {
          eraseDetection(i);
        }
        detections[i].active = false;
      }
    }
  }
}

void drawDetection(int index) {
  if(!detections[index].active) return;
  
  unsigned long age = millis() - detections[index].timestamp;
  
  // Map distance to radius
  int objRadius = map(detections[index].distance, 0, maxDistance, 0, maxRadius);
  int objX = centerX;
  int objY = centerY - objRadius;
  
  // Color based on age
  uint16_t color;
  if(age < 500) {
    color = ST77XX_RED;
  } else if(age < 1500) {
    color = ST77XX_YELLOW;
  } else {
    color = ST77XX_MAGENTA;
  }
  
  // Draw object
  tft.fillCircle(objX, objY, 2, color);
  
  detections[index].drawn = true;
}

void eraseDetection(int index) {
  if(!detections[index].drawn) return;
  
  int objRadius = map(detections[index].distance, 0, maxDistance, 0, maxRadius);
  int objX = centerX;
  int objY = centerY - objRadius;
  
  // Erase by drawing in black
  tft.fillCircle(objX, objY, 3, ST77XX_BLACK);
  
  detections[index].drawn = false;
}

void drawSweepLine(int angle) {
  int endX = centerX + ((maxRadius-5) * cos(radians(90 + angle)));
  int endY = centerY - ((maxRadius-5) * sin(radians(90 + angle)));
  
  // Draw sweep line in bright cyan
  tft.drawLine(centerX, centerY, endX, endY, ST77XX_CYAN);
}

void eraseSweepLine(int angle) {
  int endX = centerX + ((maxRadius-5) * cos(radians(90 + angle)));
  int endY = centerY - ((maxRadius-5) * sin(radians(90 + angle)));
  
  // Erase by drawing in black
  tft.drawLine(centerX, centerY, endX, endY, ST77XX_BLACK);
}

void displayInfo(float distance) {
  // Clear info area only
  tft.fillRect(0, 140, 160, 20, ST77XX_BLACK);
  
  // Current distance
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(5, 145);
  tft.print("Distance: ");
  if(distance > 0 && distance < maxDistance) {
    tft.print(distance, 1);
    tft.print(" cm");
  } else {
    tft.print("No Object");
  }
  
  // Status
  tft.setCursor(5, 155);
  if(distance > 0 && distance < 20) {
    tft.setTextColor(ST77XX_RED);
    tft.print("WARNING: CLOSE!");
  } else if(distance > 0 && distance < 50) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("Object Detected");
  } else {
    tft.setTextColor(ST77XX_GREEN);
    tft.print("Clear");
  }
}
