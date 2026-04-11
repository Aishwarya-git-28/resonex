from flask import Flask, request, jsonify, render_template
from flask_cors import CORS
from collections import deque
import threading

app = Flask(__name__)
CORS(app)

data_store = deque(maxlen=100)
lock = threading.Lock()

@app.route("/data", methods=["POST"])
def receive_data():
    payload = request.get_json(force=True)
    if not payload:
        return jsonify({"error": "no data"}), 400
    with lock:
        data_store.appendleft(payload)
    return jsonify({"ok": True})

@app.route("/feed")
def feed():
    with lock:
        return jsonify(list(data_store))

@app.route("/")
def index():
    return render_template("index.html")

if __name__ == "__main__":
    app.run(host='0.0.0.0', debug=True, port=5000)