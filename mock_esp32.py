import requests, time, random, datetime

FLASK_URL = "http://localhost:5000/data"
MODELS = ["bridge", "tower", "transformer"]

scenario = ["normal"]*5 + ["warning"]*3 + ["critical"]*2
i = 0

while True:
    status = scenario[i % len(scenario)]
    base_freq = {"normal": 50, "warning": 120, "critical": 280}[status]
    confidence = {"normal": random.uniform(0.88, 0.99),
                  "warning": random.uniform(0.75, 0.90),
                  "critical": random.uniform(0.80, 0.97)}[status]

    payload = {
        "model": random.choice(MODELS),
        "status": status,
        "confidence": round(confidence, 2),
        "dominant_freq": round(base_freq + random.uniform(-10, 10), 1),
        "amplitude": round(random.uniform(0.2, 0.9), 2),
        "timestamp": datetime.datetime.now().isoformat()
    }

    try:
        requests.post(FLASK_URL, json=payload)
        print(f"[{payload['timestamp'][11:19]}] {payload['model'].upper()} → {payload['status'].upper()} | freq: {payload['dominant_freq']}Hz | conf: {payload['confidence']}")
    except Exception as e:
        print(f"Flask not running: {e}")

    i += 1
    time.sleep(2)