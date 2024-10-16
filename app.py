from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO, emit
import logging
from concurrent.futures import ThreadPoolExecutor

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode="threading")

# Variabel global untuk menyimpan data sensor dan status relay
sensor_data = {
    'temperature': 0,
    'humidity': 0
}
relay_status = 'OFF'

# ThreadPoolExecutor untuk menangani tugas latar belakang
executor = ThreadPoolExecutor(max_workers=2)

# Fungsi untuk memperbarui data sensor dan mengirimkan pembaruan ke klien
def update_sensor_data(data):
    global sensor_data
    sensor_data['temperature'] = data.get('temperature', 0)
    sensor_data['humidity'] = data.get('humidity', 0)
    socketio.emit('sensor_update', sensor_data)

@app.route('/')
def index():
    return render_template('index.html')

# Terima data dari ESP32 melalui POST request
@app.route('/data', methods=['POST'])
def receive_data():
    try:
        data = request.get_json(force=True)
        if data:
            app.logger.debug(f"Received data: {data}")
            # Proses data di latar belakang untuk mempercepat respon
            executor.submit(update_sensor_data, data)
            return jsonify({'status': 'success'}), 200
        else:
            app.logger.error("Invalid data format")
            return jsonify({'status': 'error', 'message': 'Invalid data'}), 400
    except Exception as e:
        app.logger.error(f"Error processing request: {e}")
        return jsonify({'status': 'error', 'message': 'Internal server error'}), 500

# Endpoint untuk mengontrol relay (on/off)
@app.route('/lamp/<command>', methods=['POST'])
def control(command):
    global relay_status
    if command == 'on':
        relay_status = 'ON'
    elif command == 'off':
        relay_status = 'OFF'
    else:
        return jsonify({'status': 'error', 'message': 'Invalid command'}), 400

    return jsonify({'status': 'success', 'relay': relay_status}), 200

# ESP32 akan mengirim GET request ke endpoint ini untuk mendapatkan status relay
@app.route('/relay', methods=['GET'])
def control_relay():
    global relay_status
    return jsonify({'relay': relay_status})

@socketio.on("connect")
def handle_connect():
    print("Client Connected")

@socketio.on("disconnect")
def handle_disconnect():
    print("Client Disconnected")

if __name__ == '__main__':
    app.logger.setLevel(logging.ERROR)  # Aktifkan hanya logging untuk error
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
