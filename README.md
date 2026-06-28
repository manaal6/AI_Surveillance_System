# ESP32-CAM AI Surveillance System

An ESP32-CAM based smart surveillance system with PIR motion detection, Firebase Realtime Database control, MIT App Inventor support, and a Python Flask face-recognition server.

The ESP32-CAM stays connected to Wi-Fi and Firebase, listens for remote commands, monitors a PIR motion sensor, captures images when motion is detected, and sends those images to a local Python server for face recognition. Known faces are treated as safe detections; unknown or missing faces trigger alarm behavior and Firebase/app updates.

## Features

- ESP32-CAM firmware for AI Thinker ESP32-CAM boards
- PIR-based motion trigger
- JPEG capture from ESP32-CAM
- Local Flask API for face recognition
- Known-face enrollment through images in `known_faces/`
- Firebase Realtime Database logging, command polling, health updates, and app status
- MIT App Inventor project included as `Manaal_Surveillance_CleanBuild.aia`
- Remote commands for arming, disarming, alarm, light, status, reboot, and face-server IP refresh
- LED and buzzer alerts for known face, unknown face, no face, and alarm states

## Project Structure

```text
.
|-- ESP32CAM_AI_Surveillance.ino   # Main Arduino firmware
|-- config.h                       # Wi-Fi, Firebase, pin, timing, and server config
|-- camera_init.h                  # ESP32-CAM and Wi-Fi initialization
|-- command_handler.h              # Firebase command dispatcher
|-- face_recognition_helper.h      # ESP32 HTTP client for Flask face API
|-- firebase_helper.h              # Firebase auth, logging, status, commands
|-- hardware.h                     # LED, buzzer, light, and state helpers
|-- motion_detect.h                # Optional frame-difference motion logic
|-- app.py                         # Flask face-recognition server
|-- known_faces/                   # Enrolled face images
|-- last_captured.jpg              # Latest image saved by the Flask server
`-- Manaal_Surveillance_CleanBuild.aia
```

## Hardware Requirements

- AI Thinker ESP32-CAM
- PIR motion sensor
- Buzzer
- Red LED
- Yellow LED
- Jumper wires and suitable power supply
- USB-to-serial programmer for uploading firmware

Default pin map:

| Component | ESP32 Pin |
| --- | --- |
| PIR sensor | GPIO 15 |
| Yellow LED | GPIO 13 |
| Red LED | GPIO 14 |
| Buzzer | GPIO 2 |
| White flash/light | GPIO 4 |

## Software Requirements

### Arduino

- Arduino IDE
- ESP32 Arduino core v2.x
- Board: `AI Thinker ESP32-CAM`
- Libraries:
  - `Firebase ESP Client` by Mobizt, v4.x
  - `ArduinoJson`, v7.x

### Python

- Python 3
- Flask
- requests
- numpy
- opencv-python
- face_recognition

Install Python dependencies:

```bash
pip install flask requests numpy opencv-python face_recognition
```

Note: `face_recognition` depends on `dlib`, which may require CMake and build tools on Windows.

## Configuration

Edit `config.h` before uploading the firmware:

```cpp
#define WIFI_SSID        "YOUR_WIFI_NAME"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"

#define FIREBASE_HOST          "your-project-default-rtdb.region.firebasedatabase.app"
#define FIREBASE_API_KEY       "YOUR_FIREBASE_API_KEY"
#define FIREBASE_USER_EMAIL    "YOUR_FIREBASE_USER_EMAIL"
#define FIREBASE_USER_PASSWORD "YOUR_FIREBASE_USER_PASSWORD"

#define DEVICE_ID    "esp32cam-01"

#define FACE_SERVER_IP          "192.168.x.x"
#define FACE_SERVER_PORT        5000
```

Keep real Wi-Fi and Firebase credentials private. Do not publish `config.h` with production secrets.

## Firebase Database Paths

The firmware reads and writes these Realtime Database paths:

| Path | Purpose |
| --- | --- |
| `/commands/{DEVICE_ID}/pending` | Remote command input |
| `/devices/{DEVICE_ID}` | Device status, firmware, IP, state |
| `/system_health/{DEVICE_ID}` | Uptime, heap, state, Wi-Fi RSSI |
| `/logs/{DEVICE_ID}` | Event history |
| `/mit_app/{DEVICE_ID}/latest` | Latest status for MIT App Inventor |
| `/settings/{DEVICE_ID}/face_server_ip` | Runtime face server IP |
| `/alerts/{DEVICE_ID}` | Motion alert values |

Supported commands:

```text
ARM
DISARM
SILENT
ALARM
STATUS
LIGHTON
LIGHTOFF
RESETBASELINE
REFRESHIP
REBOOT
TEST
```

After each command is handled, the firmware resets `/commands/{DEVICE_ID}/pending` to `NONE`.

## Face Recognition Server

Place one or more clear face images inside `known_faces/`.

Filename format:

```text
Name.jpg
Name_1.jpg
Name_2.png
```

The server uses the filename before the first underscore as the person's name. For example, `Ali_1.jpeg` is enrolled as `Ali`.

Run the server:

```bash
python app.py
```

The Flask server starts on:

```text
http://0.0.0.0:5000
```

Available endpoints:

| Endpoint | Method | Description |
| --- | --- | --- |
| `/recognize` | `POST` | Accepts JPEG bytes from ESP32-CAM and returns recognized faces |
| `/recognize?esp32_ip=192.168.x.x` | `GET` | Compatibility mode that fetches a snapshot from an ESP32 endpoint |
| `/reload` | `GET`/`POST` | Reloads images from `known_faces/` |

Example response:

```json
{
  "faces": [
    {
      "name": "Ali",
      "confidence": 82.4
    }
  ]
}
```

## Firmware Upload

1. Open `ESP32CAM_AI_Surveillance.ino` in Arduino IDE.
2. Select board `AI Thinker ESP32-CAM`.
3. Set the correct upload port.
4. Update `config.h`.
5. Put the ESP32-CAM into flash mode.
6. Upload the firmware.
7. Open Serial Monitor at `115200` baud.
8. Restart the board and check Wi-Fi, camera, Firebase, and face-server logs.

## Typical Startup Order

1. Connect your computer and ESP32-CAM to the same Wi-Fi network.
2. Start the Python server with `python app.py`.
3. Confirm your computer IP address and set it as `FACE_SERVER_IP` or update `/settings/{DEVICE_ID}/face_server_ip` in Firebase.
4. Power or reset the ESP32-CAM.
5. Send `ARM` through Firebase or the MIT App Inventor app.
6. Trigger the PIR sensor.
7. Watch Serial Monitor, Flask logs, Firebase, and the app status.

## Behavior

- Known face detected: yellow LED turns on briefly, alarm is muted, system returns to armed state.
- Unknown face detected: red LED and alarm activate, Firebase/app report an intruder warning.
- No face detected: warning signal is shown and logged.
- Face server timeout/error: system treats the event as suspicious and triggers alarm behavior.

## Troubleshooting

- Camera init fails: check board selection, power supply, and ESP32-CAM model.
- Firebase not ready: verify API key, database URL, user email/password, and Firebase rules.
- Face server unreachable: make sure the PC and ESP32-CAM are on the same network and that port `5000` is allowed through the firewall.
- No known faces loaded: check that `known_faces/` exists and images contain clear, front-facing faces.
- Recognition is too strict or too loose: adjust `TOLERANCE` in `app.py`; lower values are stricter.
- ESP32 resets during Wi-Fi/Firebase: use a stable 5V supply with enough current.

## Security Notes

- Do not commit real Wi-Fi passwords, Firebase credentials, or personal face images to a public repository.
- Rotate exposed Firebase credentials before sharing this project publicly.
- Use Firebase rules that limit access to trusted users only.
- Run the face-recognition server only on trusted networks.

## License

No license file is currently included. Add a license before publishing or reusing this project externally.
