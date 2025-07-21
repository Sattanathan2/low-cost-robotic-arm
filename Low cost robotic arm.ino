#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <EEPROM.h>

// WiFi credentials
const char* ssid = "tony arm";
const char* password = "12345678";

// Create web server on port 80
WebServer server(80);

// Servo objects
Servo baseServo;
Servo rightArmServo;
Servo leftArmServo;
Servo gripperServo;

// Servo pins (adjust according to your wiring)
const int basePIN = 2;
const int rightArmPIN = 4;
const int leftArmPIN = 5;
const int gripperPIN = 18;

// Current servo positions
int basePos = 0;
int rightArmPos = 90;
int leftArmPos = 0;
int gripperPos = 0;

// Recording variables
struct ServoPosition {
  int base;
  int rightArm;
  int leftArm;
  int gripper;
};

ServoPosition recordedPositions[50];
int recordedCount = 0;
bool isRecording = false;
bool isPlaying = false;
bool autoMode = false;

// HTML page
const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Tony Robotic Arm Controller</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { font-family: Arial; text-align: center; background: #f0f0f0; margin: 0; padding: 20px; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }
        h1 { color: #333; margin-bottom: 30px; }
        .mode-buttons { margin-bottom: 30px; }
        .mode-btn { padding: 15px 30px; margin: 10px; font-size: 18px; border: none; border-radius: 5px; cursor: pointer; }
        .manual-btn { background: #4CAF50; color: white; }
        .auto-btn { background: #2196F3; color: white; }
        .servo-control { margin: 20px 0; padding: 15px; background: #f9f9f9; border-radius: 5px; }
        .servo-label { font-weight: bold; margin-bottom: 10px; }
        .slider { width: 80%; height: 25px; margin: 10px 0; }
        .value-display { font-size: 18px; font-weight: bold; color: #333; margin: 5px 0; }
        .control-buttons { margin: 20px 0; }
        .control-btn { padding: 12px 25px; margin: 10px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; }
        .record-btn { background: #FF5722; color: white; }
        .play-btn { background: #4CAF50; color: white; }
        .stop-btn { background: #f44336; color: white; }
        .auto-start-btn { background: #9C27B0; color: white; }
        .hidden { display: none; }
        .status { margin: 20px 0; padding: 15px; background: #e3f2fd; border-radius: 5px; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🦾 Tony Robotic Arm Controller</h1>
        
        <div class="mode-buttons">
            <button class="mode-btn manual-btn" onclick="setMode('manual')">📱 Manual Control</button>
            <button class="mode-btn auto-btn" onclick="setMode('auto')">🤖 Auto Mode</button>
        </div>

        <div id="status" class="status">Ready to operate</div>

        <!-- Manual Control Section -->
        <div id="manualSection">
            <div class="servo-control">
                <div class="servo-label">🔄 Base Servo (0-180°)</div>
                <input type="range" min="0" max="180" value="0" class="slider" id="baseSlider" oninput="updateServo('base', this.value)">
                <div class="value-display">Position: <span id="baseValue">0</span>°</div>
            </div>

            <div class="servo-control">
                <div class="servo-label">💪 Right Arm (40-100°)</div>
                <input type="range" min="40" max="100" value="90" class="slider" id="rightArmSlider" oninput="updateServo('rightArm', this.value)">
                <div class="value-display">Position: <span id="rightArmValue">90</span>°</div>
            </div>

            <div class="servo-control">
                <div class="servo-label">🤲 Left Arm (0-40°)</div>
                <input type="range" min="0" max="40" value="0" class="slider" id="leftArmSlider" oninput="updateServo('leftArm', this.value)">
                <div class="value-display">Position: <span id="leftArmValue">0</span>°</div>
            </div>

            <div class="servo-control">
                <div class="servo-label">✋ Gripper (0-28°)</div>
                <input type="range" min="0" max="28" value="0" class="slider" id="gripperSlider" oninput="updateServo('gripper', this.value)">
                <div class="value-display">Position: <span id="gripperValue">0</span>°</div>
            </div>

            <div class="control-buttons">
                <button class="control-btn record-btn" id="recordBtn" onclick="toggleRecord()">🔴 Start Recording</button>
                <button class="control-btn play-btn" onclick="playRecording()">▶️ Play Recording</button>
                <button class="control-btn stop-btn" onclick="stopAll()">⏹️ Stop & Reset</button>
            </div>
        </div>

        <!-- Auto Mode Section -->
        <div id="autoSection" class="hidden">
            <h3>🤖 Automatic Operation</h3>
            <p>Pre-programmed 16-step pick and place sequence</p>
            <div class="control-buttons">
                <button class="control-btn auto-start-btn" onclick="startAuto()">🚀 Start Auto Sequence</button>
                <button class="control-btn stop-btn" onclick="stopAll()">⏹️ Stop Auto</button>
            </div>
        </div>
    </div>

    <script>
        let isRecording = false;
        let currentMode = 'manual';

        function setMode(mode) {
            currentMode = mode;
            if (mode === 'manual') {
                document.getElementById('manualSection').classList.remove('hidden');
                document.getElementById('autoSection').classList.add('hidden');
            } else {
                document.getElementById('manualSection').classList.add('hidden');
                document.getElementById('autoSection').classList.remove('hidden');
            }
            updateStatus('Mode switched to ' + mode);
        }

        function updateServo(servo, value) {
            document.getElementById(servo + 'Value').innerText = value;
            fetch('/move?servo=' + servo + '&value=' + value)
                .then(response => response.text())
                .then(data => console.log(data));
        }

        function toggleRecord() {
            const btn = document.getElementById('recordBtn');
            if (!isRecording) {
                isRecording = true;
                btn.innerText = '⏺️ Stop Recording';
                btn.style.background = '#FF9800';
                fetch('/record?action=start');
                updateStatus('Recording started - adjust servos to record positions');
            } else {
                isRecording = false;
                btn.innerText = '🔴 Start Recording';
                btn.style.background = '#FF5722';
                fetch('/record?action=stop');
                updateStatus('Recording stopped');
            }
        }

        function playRecording() {
            fetch('/play')
                .then(response => response.text())
                .then(data => {
                    updateStatus('Playing recorded sequence...');
                });
        }

        function startAuto() {
            fetch('/auto')
                .then(response => response.text())
                .then(data => {
                    updateStatus('Running automatic pick & place sequence...');
                });
        }

        function stopAll() {
            fetch('/stop');
            updateStatus('Stopping all operations - returning to default position');
            // Reset sliders to default
            document.getElementById('baseSlider').value = 0;
            document.getElementById('rightArmSlider').value = 90;
            document.getElementById('leftArmSlider').value = 0;
            document.getElementById('gripperSlider').value = 0;
            document.getElementById('baseValue').innerText = 0;
            document.getElementById('rightArmValue').innerText = 90;
            document.getElementById('leftArmValue').innerText = 0;
            document.getElementById('gripperValue').innerText = 0;
        }

        function updateStatus(message) {
            document.getElementById('status').innerText = message;
        }

        // Update servo positions every 2 seconds
        setInterval(() => {
            fetch('/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('baseSlider').value = data.base;
                    document.getElementById('rightArmSlider').value = data.rightArm;
                    document.getElementById('leftArmSlider').value = data.leftArm;
                    document.getElementById('gripperSlider').value = data.gripper;
                    document.getElementById('baseValue').innerText = data.base;
                    document.getElementById('rightArmValue').innerText = data.rightArm;
                    document.getElementById('leftArmValue').innerText = data.leftArm;
                    document.getElementById('gripperValue').innerText = data.gripper;
                });
        }, 2000);
    </script>
</body>
</html>
)rawliteral";

void setup() {
    Serial.begin(115200);
    
    // Initialize EEPROM
    EEPROM.begin(512);
    
    // Attach servos
    baseServo.attach(basePIN);
    rightArmServo.attach(rightArmPIN);
    leftArmServo.attach(leftArmPIN);
    gripperServo.attach(gripperPIN);
    
    // Set initial positions
    moveToDefaults();
    
    // Start WiFi Access Point
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    
    Serial.print("AP IP address: ");
    Serial.println(IP);
    
    // Define web server routes
    server.on("/", handleRoot);
    server.on("/move", handleMove);
    server.on("/record", handleRecord);
    server.on("/play", handlePlay);
    server.on("/auto", handleAuto);
    server.on("/stop", handleStop);
    server.on("/status", handleStatus);
    
    server.begin();
    Serial.println("Web server started");
    Serial.println("Connect to WiFi: tony arm");
    Serial.println("Password: 12345678");
    Serial.print("Open browser to: ");
    Serial.println(IP);
}

void loop() {
    server.handleClient();
    
    if (isPlaying) {
        playRecordedSequence();
    }
    
    if (autoMode) {
        runAutoSequence();
    }
}

void handleRoot() {
    server.send(200, "text/html", htmlPage);
}

void handleMove() {
    String servo = server.arg("servo");
    int value = server.arg("value").toInt();
    
    if (servo == "base") {
        basePos = constrain(value, 0, 180);
        baseServo.write(basePos);
    } else if (servo == "rightArm") {
        rightArmPos = constrain(value, 40, 100);
        rightArmServo.write(rightArmPos);
    } else if (servo == "leftArm") {
        leftArmPos = constrain(value, 0, 40);
        leftArmServo.write(leftArmPos);
    } else if (servo == "gripper") {
        gripperPos = constrain(value, 0, 28);
        gripperServo.write(gripperPos);
    }
    
    // Record position if recording
    if (isRecording) {
        recordCurrentPosition();
    }
    
    server.send(200, "text/plain", "Moved " + servo + " to " + value);
    Serial.println("Moved " + servo + " to " + String(value));
}

void handleRecord() {
    String action = server.arg("action");
    
    if (action == "start") {
        isRecording = true;
        recordedCount = 0;
        server.send(200, "text/plain", "Recording started");
        Serial.println("Recording started");
    } else if (action == "stop") {
        isRecording = false;
        server.send(200, "text/plain", "Recording stopped. " + String(recordedCount) + " positions recorded");
        Serial.println("Recording stopped. Positions: " + String(recordedCount));
    }
}

void handlePlay() {
    if (recordedCount > 0) {
        isPlaying = true;
        server.send(200, "text/plain", "Playing recorded sequence");
        Serial.println("Playing recorded sequence");
    } else {
        server.send(200, "text/plain", "No recorded sequence found");
        Serial.println("No recorded sequence");
    }
}

void handleAuto() {
    autoMode = true;
    server.send(200, "text/plain", "Starting automatic sequence");
    Serial.println("Starting auto mode");
}

void handleStop() {
    isPlaying = false;
    isRecording = false;
    autoMode = false;
    moveToDefaults();
    server.send(200, "text/plain", "Stopped all operations");
    Serial.println("All operations stopped");
}

void handleStatus() {
    String json = "{";
    json += "\"base\":" + String(basePos) + ",";
    json += "\"rightArm\":" + String(rightArmPos) + ",";
    json += "\"leftArm\":" + String(leftArmPos) + ",";
    json += "\"gripper\":" + String(gripperPos);
    json += "}";
    
    server.send(200, "application/json", json);
}

void recordCurrentPosition() {
    if (recordedCount < 50) {
        recordedPositions[recordedCount].base = basePos;
        recordedPositions[recordedCount].rightArm = rightArmPos;
        recordedPositions[recordedCount].leftArm = leftArmPos;
        recordedPositions[recordedCount].gripper = gripperPos;
        recordedCount++;
        Serial.println("Position " + String(recordedCount) + " recorded");
    }
}

void playRecordedSequence() {
    static int currentStep = 0;
    static unsigned long lastMove = 0;
    
    if (millis() - lastMove > 1000) { // 1 second between moves
        if (currentStep < recordedCount) {
            baseServo.write(recordedPositions[currentStep].base);
            rightArmServo.write(recordedPositions[currentStep].rightArm);
            leftArmServo.write(recordedPositions[currentStep].leftArm);
            gripperServo.write(recordedPositions[currentStep].gripper);
            
            basePos = recordedPositions[currentStep].base;
            rightArmPos = recordedPositions[currentStep].rightArm;
            leftArmPos = recordedPositions[currentStep].leftArm;
            gripperPos = recordedPositions[currentStep].gripper;
            
            Serial.println("Playing step " + String(currentStep + 1));
            currentStep++;
            lastMove = millis();
        } else {
            isPlaying = false;
            currentStep = 0;
            Serial.println("Playback completed");
        }
    }
}

void runAutoSequence() {
    static int step = 0;
    static unsigned long lastMove = 0;
    static bool loopSequence = true;
    
    if (millis() - lastMove > 2000) { // 2 seconds between moves
        switch (step) {
            case 0: // Step 1: Base 0 to 150
                moveServoSlowly(baseServo, basePos, 150);
                basePos = 150;
                Serial.println("Auto Step 1: Base to 150°");
                break;
            case 1: // Step 2: Right arm 90 to 35
                moveServoSlowly(rightArmServo, rightArmPos, 35);
                rightArmPos = 35;
                Serial.println("Auto Step 2: Right arm to 35°");
                break;
            case 2: // Step 3: Left arm 0 to 30
                moveServoSlowly(leftArmServo, leftArmPos, 30);
                leftArmPos = 30;
                Serial.println("Auto Step 3: Left arm to 30°");
                break;
            case 3: // Step 4: Gripper open 0 to 27
                moveServoSlowly(gripperServo, gripperPos, 27);
                gripperPos = 27;
                Serial.println("Auto Step 4: Gripper open to 27°");
                delay(2000); // Wait 2 seconds
                break;
            case 4: // Step 5: Gripper close 27 to 10
                moveServoSlowly(gripperServo, gripperPos, 10);
                gripperPos = 10;
                Serial.println("Auto Step 5: Gripper close to 10°");
                break;
            case 5: // Step 6: Right arm 35 to 90
                moveServoSlowly(rightArmServo, rightArmPos, 90);
                rightArmPos = 90;
                Serial.println("Auto Step 6: Right arm to 90°");
                break;
            case 6: // Step 7: Left arm 30 to 0
                moveServoSlowly(leftArmServo, leftArmPos, 0);
                leftArmPos = 0;
                Serial.println("Auto Step 7: Left arm to 0°");
                break;
            case 7: // Step 8: Base 150 to 0
                moveServoSlowly(baseServo, basePos, 0);
                basePos = 0;
                Serial.println("Auto Step 8: Base to 0°");
                break;
            case 8: // Step 9: Right arm 90 to 30
                moveServoSlowly(rightArmServo, rightArmPos, 30);
                rightArmPos = 30;
                Serial.println("Auto Step 9: Right arm to 30°");
                break;
            case 9: // Step 10: Left arm 0 to 30
                moveServoSlowly(leftArmServo, leftArmPos, 30);
                leftArmPos = 30;
                Serial.println("Auto Step 10: Left arm to 30°");
                break;
            case 10: // Step 11: Gripper open 10 to 27
                moveServoSlowly(gripperServo, gripperPos, 27);
                gripperPos = 27;
                Serial.println("Auto Step 11: Gripper open to 27°");
                break;
            case 11: // Step 12: Gripper close 27 to 0
                moveServoSlowly(gripperServo, gripperPos, 0);
                gripperPos = 0;
                Serial.println("Auto Step 12: Gripper close to 0°");
                break;
            case 12: // Step 13: Right arm 30 to 90
                moveServoSlowly(rightArmServo, rightArmPos, 90);
                rightArmPos = 90;
                Serial.println("Auto Step 13: Right arm to 90°");
                break;
            case 13: // Step 14: Left arm 30 to 0
                moveServoSlowly(leftArmServo, leftArmPos, 0);
                leftArmPos = 0;
                Serial.println("Auto Step 14: Left arm to 0°");
                break;
            case 14: // Step 15: Base 0 to 150
                moveServoSlowly(baseServo, basePos, 150);
                basePos = 150;
                Serial.println("Auto Step 15: Base to 150°");
                break;
            case 15: // Step 16: Loop back to start
                step = -1; // Will become 0 after increment
                Serial.println("Auto Step 16: Looping sequence");
                break;
        }
        
        step++;
        lastMove = millis();
    }
}

void moveServoSlowly(Servo &servo, int startPos, int endPos) {
    int stepSize = (startPos < endPos) ? 1 : -1;
    for (int pos = startPos; pos != endPos; pos += stepSize) {
        servo.write(pos);
        delay(50); // Adjust for slower movement
    }
    servo.write(endPos);
}

void moveToDefaults() {
    basePos = 0;
    rightArmPos = 90;
    leftArmPos = 0;
    gripperPos = 0;
    
    baseServo.write(basePos);
    rightArmServo.write(rightArmPos);
    leftArmServo.write(leftArmPos);
    gripperServo.write(gripperPos);
    
    Serial.println("Moved to default positions");
}