#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ESP32Servo.h>

#define DHTPIN 4
#define DHTTYPE DHT22

#define SOIL_PIN 34
#define WATER_PIN 32

#define TRIG_PIN 5
#define ECHO_PIN 18

#define RELAY_PIN 23

#define SERVO_SOIL_ARM 15
#define SERVO_ULTRASONIC_MAIN 2
#define SERVO_SEED 19

#define ENA 25
#define ENB 33
#define IN1 26
#define IN2 27
#define IN3 14
#define IN4 12

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

Servo soilArmServo;
Servo ultrasonicServo;
Servo seedServo;

const char* ssid = "AgroRobot";
const char* password = "12345678";

bool autoMode = false;
bool pumpState = false;
bool ultrasonicAutoScan = true;

bool autoSeedMode = false;
unsigned long lastSeedTime = 0;

int soilValue = 0;
int waterValue = 0;

float temperature = 0;
float humidity = 0;
long distanceCM = 0;

int dryThreshold = 2500;
int waterThreshold = 500;

int motorSpeed = 220;
int turnSpeed = 255;

int ultrasonicAngle = 90;
int scanDirection = 1;
unsigned long lastScanTime = 0;

// Seed settings
int seedHomeAngle = 90;
int seedDispenseAngle = 45;
int seedHoldTime = 700;
int seedAutoInterval = 5000;

void pumpOn() {
  digitalWrite(RELAY_PIN, HIGH);
  pumpState = true;
}

void pumpOff() {
  digitalWrite(RELAY_PIN, LOW);
  pumpState = false;
}

void setMotorSpeed(int leftSpeed, int rightSpeed) {
  ledcWrite(0, leftSpeed);
  ledcWrite(1, rightSpeed);
}

void forwardRobot() {
  setMotorSpeed(motorSpeed, motorSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backwardRobot() {
  setMotorSpeed(motorSpeed, motorSpeed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void leftRobot() {
  setMotorSpeed(turnSpeed, turnSpeed);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void rightRobot() {
  setMotorSpeed(turnSpeed, turnSpeed);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopRobot() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

long readDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  if (duration == 0) {
    return 0;
  }

  return duration * 0.034 / 2;
}

void autoScanUltrasonic() {
  if (!ultrasonicAutoScan) {
    return;
  }

  if (millis() - lastScanTime >= 35) {
    lastScanTime = millis();

    ultrasonicAngle += scanDirection;

    if (ultrasonicAngle >= 180) {
      ultrasonicAngle = 180;
      scanDirection = -1;
    }

    if (ultrasonicAngle <= 0) {
      ultrasonicAngle = 0;
      scanDirection = 1;
    }

    ultrasonicServo.write(ultrasonicAngle);
  }
}

void dispenseSeed() {
  seedServo.write(seedDispenseAngle);
  delay(seedHoldTime);
  seedServo.write(seedHomeAngle);
}

void autoSeedControl() {
  if (!autoSeedMode) {
    return;
  }

  if (millis() - lastSeedTime >= seedAutoInterval) {
    lastSeedTime = millis();
    dispenseSeed();
  }
}

void readSensors() {
  soilValue = analogRead(SOIL_PIN);
  waterValue = analogRead(WATER_PIN);

  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  distanceCM = readDistance();

  if (autoMode) {
    if (soilValue > dryThreshold && waterValue > waterThreshold) {
      pumpOn();
    } else {
      pumpOff();
    }
  }
}

void ultrasonicLeft() {
  ultrasonicAutoScan = false;
  ultrasonicAngle = 0;
  ultrasonicServo.write(ultrasonicAngle);
}

void ultrasonicCenter() {
  ultrasonicAutoScan = false;
  ultrasonicAngle = 90;
  ultrasonicServo.write(ultrasonicAngle);
}

void ultrasonicRight() {
  ultrasonicAutoScan = false;
  ultrasonicAngle = 180;
  ultrasonicServo.write(ultrasonicAngle);
}

void ultrasonicAutoOn() {
  ultrasonicAutoScan = true;
}

void soilArmUp() {
  soilArmServo.write(0);
}

void soilArmDown() {
  soilArmServo.write(90);
}

String dashboardPage() {
  readSensors();

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<style>";
  html += "body{font-family:Arial;text-align:center;background:#eaf7ea;margin:0;padding:0;}";
  html += "h2{background:#2e7d32;color:white;padding:15px;margin:0;}";
  html += ".card{background:white;margin:12px;padding:15px;border-radius:12px;box-shadow:0 0 8px #999;}";
  html += "button{padding:13px 18px;margin:5px;font-size:16px;border:none;border-radius:8px;background:#2e7d32;color:white;}";
  html += "input{padding:8px;margin:5px;font-size:16px;width:100px;}";
  html += ".red{background:#c62828;}";
  html += ".blue{background:#1565c0;}";
  html += ".orange{background:#ef6c00;}";
  html += ".gray{background:#555;}";
  html += "</style></head><body>";

  html += "<h2>ESP32 Smart Agriculture Robot</h2>";

  html += "<div class='card'>";
  html += "<h3>Sensor Data</h3>";
  html += "Soil Moisture: " + String(soilValue) + "<br>";
  html += "Water Level: " + String(waterValue) + "<br>";
  html += "Temperature: " + String(temperature) + " &deg;C<br>";
  html += "Humidity: " + String(humidity) + " %<br>";
  html += "Obstacle Distance: " + String(distanceCM) + " cm<br>";
  html += "Pump Status: " + String(pumpState ? "ON" : "OFF") + "<br>";
  html += "Water Auto Mode: " + String(autoMode ? "ON" : "OFF") + "<br>";
  html += "Ultrasonic Mode: " + String(ultrasonicAutoScan ? "AUTO SCAN" : "MANUAL") + "<br>";
  html += "Seed Auto Mode: " + String(autoSeedMode ? "ON" : "OFF") + "<br>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Robot Movement</h3>";
  html += "<a href='/forward'><button>Forward</button></a><br>";
  html += "<a href='/left'><button>Strong Left</button></a>";
  html += "<a href='/stop'><button class='red'>Stop</button></a>";
  html += "<a href='/right'><button>Strong Right</button></a><br>";
  html += "<a href='/backward'><button>Backward</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Speed Control</h3>";
  html += "<a href='/speedLow'><button class='gray'>Low</button></a>";
  html += "<a href='/speedMed'><button class='blue'>Medium</button></a>";
  html += "<a href='/speedHigh'><button class='orange'>High</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Water Pump</h3>";
  html += "<a href='/pumpOn'><button class='blue'>Pump ON</button></a>";
  html += "<a href='/pumpOff'><button class='red'>Pump OFF</button></a><br>";
  html += "<a href='/autoOn'><button>Water Auto ON</button></a>";
  html += "<a href='/autoOff'><button class='orange'>Water Auto OFF</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Main Ultrasonic Mount</h3>";
  html += "<a href='/ultraAuto'><button class='orange'>Auto Scan</button></a><br>";
  html += "<a href='/ultraLeft'><button>Left</button></a>";
  html += "<a href='/ultraCenter'><button class='blue'>Center</button></a>";
  html += "<a href='/ultraRight'><button>Right</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Soil Moisture Sensor Arm</h3>";
  html += "<a href='/soilUp'><button>UP</button></a>";
  html += "<a href='/soilDown'><button class='red'>DOWN</button></a>";
  html += "</div>";

  html += "<div class='card'>";
  html += "<h3>Seed Dispenser</h3>";
  html += "Home Position: 90<br>";
  html += "Dispense Angle: " + String(seedDispenseAngle) + "<br>";
  html += "Return Time: " + String(seedHoldTime) + " ms<br>";
  html += "Auto Interval: " + String(seedAutoInterval) + " ms<br>";
  html += "<form action='/setSeed' method='GET'>";
  html += "Dispense Angle: <input type='number' name='angle' min='0' max='180' value='" + String(seedDispenseAngle) + "'><br>";
  html += "Return Time ms: <input type='number' name='hold' min='100' max='5000' value='" + String(seedHoldTime) + "'><br>";
  html += "Auto Interval ms: <input type='number' name='interval' min='1000' max='60000' value='" + String(seedAutoInterval) + "'><br>";
  html += "<button class='blue' type='submit'>Save Seed Settings</button>";
  html += "</form>";
  html += "<a href='/seed'><button>Dispense Once</button></a><br>";
  html += "<a href='/seedAutoOn'><button class='orange'>Auto Seed ON</button></a>";
  html += "<a href='/seedAutoOff'><button class='red'>Auto Seed OFF</button></a>";
  html += "</div>";

  html += "</body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);

  digitalWrite(RELAY_PIN, LOW);
  pinMode(RELAY_PIN, OUTPUT);
  pumpState = false;

  dht.begin();

  pinMode(SOIL_PIN, INPUT);
  pinMode(WATER_PIN, INPUT);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  ledcSetup(0, 1000, 8);
  ledcAttachPin(ENA, 0);

  ledcSetup(1, 1000, 8);
  ledcAttachPin(ENB, 1);

  setMotorSpeed(motorSpeed, motorSpeed);
  stopRobot();

  soilArmServo.attach(SERVO_SOIL_ARM);
  ultrasonicServo.attach(SERVO_ULTRASONIC_MAIN);
  seedServo.attach(SERVO_SEED);

  soilArmUp();
  ultrasonicServo.write(90);
  seedServo.write(seedHomeAngle);

  WiFi.softAP(ssid, password);

  server.on("/", []() {
    server.send(200, "text/html", dashboardPage());
  });

  server.on("/forward", []() {
    forwardRobot();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/backward", []() {
    backwardRobot();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/left", []() {
    leftRobot();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/right", []() {
    rightRobot();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/stop", []() {
    stopRobot();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/speedLow", []() {
    motorSpeed = 130;
    setMotorSpeed(motorSpeed, motorSpeed);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/speedMed", []() {
    motorSpeed = 220;
    setMotorSpeed(motorSpeed, motorSpeed);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/speedHigh", []() {
    motorSpeed = 255;
    setMotorSpeed(motorSpeed, motorSpeed);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/pumpOn", []() {
    autoMode = false;
    pumpOn();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/pumpOff", []() {
    autoMode = false;
    pumpOff();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/autoOn", []() {
    autoMode = true;
    pumpOff();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/autoOff", []() {
    autoMode = false;
    pumpOff();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/ultraAuto", []() {
    ultrasonicAutoOn();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/ultraLeft", []() {
    ultrasonicLeft();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/ultraCenter", []() {
    ultrasonicCenter();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/ultraRight", []() {
    ultrasonicRight();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/soilUp", []() {
    soilArmUp();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/soilDown", []() {
    soilArmDown();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/setSeed", []() {
    if (server.hasArg("angle")) {
      seedDispenseAngle = server.arg("angle").toInt();
      if (seedDispenseAngle < 0) seedDispenseAngle = 0;
      if (seedDispenseAngle > 180) seedDispenseAngle = 180;
    }

    if (server.hasArg("hold")) {
      seedHoldTime = server.arg("hold").toInt();
      if (seedHoldTime < 100) seedHoldTime = 100;
      if (seedHoldTime > 5000) seedHoldTime = 5000;
    }

    if (server.hasArg("interval")) {
      seedAutoInterval = server.arg("interval").toInt();
      if (seedAutoInterval < 1000) seedAutoInterval = 1000;
      if (seedAutoInterval > 60000) seedAutoInterval = 60000;
    }

    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/seed", []() {
    dispenseSeed();
    lastSeedTime = millis();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/seedAutoOn", []() {
    autoSeedMode = true;
    lastSeedTime = millis();
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.on("/seedAutoOff", []() {
    autoSeedMode = false;
    seedServo.write(seedHomeAngle);
    server.sendHeader("Location", "/");
    server.send(303);
  });

  server.begin();

  Serial.println("AgroRobot Hotspot Started");
  Serial.println("WiFi Name: AgroRobot");
  Serial.println("Password: 12345678");
  Serial.println("Open browser: 192.168.4.1");
}

void loop() {
  server.handleClient();
  readSensors();
  autoScanUltrasonic();
  autoSeedControl();
  delay(10);
}
