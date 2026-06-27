import os
import requests
import numpy as np
import cv2
import face_recognition
from flask import Flask, jsonify, request

app = Flask(__name__)

# ── Configuration ──────────────────────────────────────────────
KNOWN_FACES_DIR = "known_faces"
TOLERANCE = 0.5  # Lower value = stricter/fewer false positives (default 0.6)

# Cache for enrolled faces
known_encodings = []
known_names = []

def load_known_faces():
    """Load and encode all images in the known_faces directory."""
    global known_encodings, known_names
    known_encodings.clear()
    known_names.clear()

    if not os.path.exists(KNOWN_FACES_DIR):
        os.makedirs(KNOWN_FACES_DIR)
        print(f"[*] Created '{KNOWN_FACES_DIR}' directory. Put face images here (e.g. Aura_1.jpg, John_2.jpg).")
        return

    print("[*] Loading known faces...")
    for filename in os.listdir(KNOWN_FACES_DIR):
        if filename.lower().endswith((".jpg", ".jpeg", ".png")):
            # Extract name: "Aura_1.jpg" -> "Aura"
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

def process_and_recognize(image_bytes):
    """Processes image bytes, runs face recognition, and returns JSON structure."""
    img_array = np.frombuffer(image_bytes, np.uint8)
    frame = cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    if frame is None:
        raise ValueError("Could not decode image.")

    # Save the captured image so you can see it on your PC
    cv2.imwrite("last_captured.jpg", frame)
    print("[*] Saved captured image to 'last_captured.jpg'")

    rgb_frame = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)

    # Detect faces
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
                # Calculate confidence percentage (distance 0.0 = 100%, distance >= 1.0 = 0%)
                dist = distances[best_idx]
                confidence = round(max(0.0, (1 - dist)) * 100, 1)

        results.append({"name": name, "confidence": confidence})
        print(f"[RECOGNIZED] Detected {name} ({confidence}%)")
    
    return results

# ── POST Endpoint: Receive JPEG directly from ESP32-CAM ─────────
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

# ── GET Endpoint: Fetch snapshot from ESP32-CAM (compatibility) ──
@app.route("/recognize", methods=["GET"])
def recognize_get():
    # Retrieve ESP32 IP from query param or header, fallback to local lookup
    esp32_ip = request.args.get("esp32_ip")
    if not esp32_ip:
        return jsonify({"error": "Please provide esp32_ip query parameter (e.g. /recognize?esp32_ip=192.168.1.50)"}), 400
    
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

# ── Reload Endpoint ────────────────────────────────────────────
@app.route("/reload", methods=["GET", "POST"])
def reload_faces():
    load_known_faces()
    return jsonify({"status": "success", "enrolled_count": len(known_names)})

if __name__ == "__main__":
    load_known_faces()
    # Run server on all interfaces, port 5000
    app.run(host="0.0.0.0", port=5000)
