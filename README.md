# low-cost-robotic-arm
Low-cost -robotic arm                       browser-based smart controller for robotic arms with real-time control, auto mode, and recording.














| 🧠 Feature               | ✅ Description                                           |
| ------------------------ | ------------------------------------------------------- |
| Manual Mode              | Control servos using sliders with live angle updates    |
| Auto Mode                | One-click start for a 16-step pick-and-place sequence   |
| Record & Replay          | Capture custom sequences and replay them anytime        |
| Real-Time Feedback       | UI syncs with actual servo positions from hardware      |
| Fully Web-Based          | No extra software needed — runs directly in the browser |
| Lightweight & Responsive | Designed for both desktop and mobile devices            |

    Frontend: HTML, CSS, JavaScript (Vanilla)

    Backend: REST API (/move, /record, /play, /stop, /status)

    Hardware: Arduino, ESP32, or any microcontroller with PWM & WiFi

    Servos: Standard SG90 or similar (Base, Right Arm, Left Arm, Gripper)





🔌 Hardware Setup

    4x Servo motors (Base, Right Arm, Left Arm, Gripper)

    Microcontroller (e.g., Arduino Uno, ESP32)

    Power supply for servos

    Connect servos to PWM-compatible pins



    🌐 Backend API

Implement a simple server (Arduino sketch, Flask, Node.js, etc.) to support:

    /move?servo=base&value=90

    /record?action=start|stop

    /play

    /auto

    /stop

    /status (returns JSON with current servo positions)
