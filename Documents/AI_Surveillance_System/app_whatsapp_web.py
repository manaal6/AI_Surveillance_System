import os
import time

import cv2
import face_recognition
import numpy as np
import requests
from flask import Flask, jsonify, request

app = Flask(__name__)

KNOWN_FACES_DIR = "known_faces"
TOLERANCE = 0.5

# WhatsApp Web settings.
# Set before running:
#   $env:WHATSAPP_PHONE="+923001234567"
WHATSAPP_PHONE = os.getenv("WHATSAPP_PHONE", "")
WHATSAPP_ALERT_COOLDOWN_SEC = int(os.getenv("WHATSAPP_ALERT_COOLDOWN_SEC", "30"))

known_encodings = []
known_names = []
last_alert_time = {}


def load_known_faces():
    global known_encodings, known_names
    known_encodings.clear()
    known_names.clear()

    if not os.path.exists(KNOWN_FACES_DIR):
        os.makedirs(KNOWN_FACES_DIR)
        print(f"[*] Created '{KNOWN_FACES_DIR}' directory. Put face images here.")
        return

    print("[*] Loading known faces...")
    for filename in os.listdir(KNOWN_FACES_DIR):
        if filename.lower().endswith((".jpg", ".jpeg", ".png")):
            name = os.path.splitext(filename)[0].split("_")[0]
            img_path = os.path.join(KNOWN_FACES_DIR, filename)
            try:
                img = face_recognition.load_image_file(img_path)
                encs = face_recognition.face_encodings(img)
                if encs:
                    known_encodings.append(encs[0])
                    known_names.append(name)
                    print(f" [ENROLLED] '{name}' from {filename}")
                else:
                    print(f" [WARNING] No faces found in {filename}, skipping.")
            except Exception as e:
                print(f" [ERROR] Could not load {filename}: {e}")

    print(f"[*] Enrollment complete. {len(known_names)} faces loaded.")


def send_whatsapp_web_alert(alert_key, message):
    if not WHATSAPP_PHONE:
        print("[WHATSAPP] Skipped: set WHATSAPP_PHONE first.")
        return

    now = time.time()
    last_sent = last_alert_time.get(alert_key, 0)
    if now - last_sent < WHATSAPP_ALERT_COOLDOWN_SEC:
        print(f"[WHATSAPP] Skipped cooldown for {alert_key}.")
        return

    try:
        import pywhatkit
    except ImportError:
        print("[WHATSAPP] pywhatkit is not installed. Run: python -m pip install pywhatkit")
        return

    try:
        pywhatkit.sendwhatmsg_instantly(
            phone_no=WHATSAPP_PHONE,
            message=message,
            wait_time=15,
            tab_close=True,
            close_time=3,
        )
        last_alert_time[alert_key] = now
        print(f"[WHATSAPP] Sent: {message}")
    except Exception as e:
        print(f"[WHATSAPP] Failed: {e}")


def notify_recognition_result(results):
    if not results:
        send_whatsapp_web_alert(
            "no_face",
            "AI Surveillance Alert: Motion detected, but no face was found in the image.",
        )
        return

    known_faces = [face for face in results if face["name"] != "Unknown"]
    if known_faces:
        names = ", ".join(f"{face['name']} ({face['confidence']}%)" for face in known_faces)
        send_whatsapp_web_alert(
            "known_" + "_".join(face["name"] for face in known_faces),
            f"AI Surveillance: Known face detected - {names}.",
        )
        return

    send_whatsapp_web_alert(
        "unknown_face",
        "AI Surveillance Alert: Unknown face detected. Please check immediately.",
    )


def process_and_recognize(image_bytes):
    img_array = np.frombuffer(image_bytes, np.uint8)
    frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    if frame is None:
        raise ValueError("Could not decode image.")

    cv2.imwrite("last_captured.jpg", frame)
    print("[*] Saved captured image to 'last_captured.jpg'")

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
    locations = face_recognition.face_locations(rgb_frame)
    encodings = face_recognition.face_encodings(rgb_frame, locations)

    results = []
    for enc in encodings:
        name = "Unknown"
        confidence = 0.0

        if known_encodings:
            matches = face_recognition.compare_faces(known_encodings, enc, tolerance=TOLERANCE)
            distances = face_recognition.face_distance(known_encodings, enc)

            if True in matches:
                best_idx = np.argmin(distances)
                name = known_names[best_idx]
                dist = distances[best_idx]
                confidence = round(max(0.0, (1 - dist)) * 100, 1)

        results.append({"name": name, "confidence": confidence})
        print(f"[RECOGNIZED] Detected {name} ({confidence}%)")

    notify_recognition_result(results)
    return results


@app.route("/recognize", methods=["POST"])
def recognize_post():
    try:
        if not request.data:
            return jsonify({"error": "No image data received"}), 400

        results = process_and_recognize(request.data)
        return jsonify({"faces": results})
    except Exception as e:
        print(f"[ERROR] POST recognize failed: {e}")
        return jsonify({"error": str(e)}), 500


@app.route("/recognize", methods=["GET"])
def recognize_get():
    esp32_ip = request.args.get("esp32_ip")
    if not esp32_ip:
        return jsonify({"error": "Please provide esp32_ip query parameter"}), 400

    try:
        url = f"http://{esp32_ip}/snap"
        print(f"[*] Fetching image from ESP32-CAM: {url}")
        resp = requests.get(url, timeout=5)

        if resp.status_code != 200:
            return jsonify({"error": f"Failed to get snapshot, status code: {resp.status_code}"}), 502

        results = process_and_recognize(resp.content)
        return jsonify({"faces": results})
    except Exception as e:
        print(f"[ERROR] GET recognize failed: {e}")
        return jsonify({"error": str(e)}), 500


@app.route("/reload", methods=["GET", "POST"])
def reload_faces():
    load_known_faces()
    return jsonify({"status": "success", "enrolled_count": len(known_names)})


if __name__ == "__main__":
    load_known_faces()
    app.run(host="0.0.0.0", port=5000)
