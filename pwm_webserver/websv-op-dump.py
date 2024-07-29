import cv2
from flask import Flask, Response, request, render_template, jsonify
import subprocess


app = Flask(__name__)



# Biến toàn cục để lưu trạng thái
status = {'forward': False, 'backward': False, 'left': False, 'right': False}


def generate_frames():
    camera = cv2.VideoCapture(1)
    if not camera.isOpened():
        raise RuntimeError("Could not start camera.")

    try:
        while True:
            success, frame = camera.read()
            if not success:
                break
            else:
                ret, buffer = cv2.imencode('.jpg', frame)
                frame = buffer.tobytes()
                yield (b'--frame\r\n'
                       b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n\r\n')
    finally:
        camera.release()

@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    return render_template('index.html', status=status)



@app.route('/update_status', methods=['POST'])
def update_status():

    button = request.form.get('button')
    state = request.form.get('state') == 'true'
    if button in status:
        status[button] = state
        pressed_buttons = [btn for btn, is_pressed in status.items() if is_pressed]

        if pressed_buttons:
            echo_command = f"echo {' '.join(pressed_buttons)} > /dev/softpwm"
            subprocess.run(echo_command, shell=True, check=True)
        else:
            echo_command = f"echo {''} > /dev/softpwm"
            subprocess.run(echo_command, shell=True, check=True)

    return '', 204

@app.route('/status')
def get_status():
    return jsonify(status)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8000)






