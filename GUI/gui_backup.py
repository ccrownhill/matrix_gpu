#!/usr/bin/env python3

import sys
import threading
import numpy as np
import subprocess
import socket
import serial
import time
from pydub import AudioSegment
from pydub.playback import play

from PyQt5.QtWidgets import QApplication, QLabel, QWidget, QLineEdit, QPushButton, QVBoxLayout, QHBoxLayout, QSizePolicy, QShortcut, QTextEdit
from PyQt5.QtCore import Qt, QEvent, QObject, pyqtSignal, QTimer


# Global variables
SEND_PORT = 20000

PRESSED = """
            QLabel {
                border: 4px solid #8f8f91;
                border-radius: 6px;
                background-color: #717575;
                padding: 6px;
                text-align: center;
            }
            QLabel:hover {
                background-color: #35b7de;
            }
        """

UNPRESSED = """
            QLabel {
                border: 2px solid #8f8f91;
                border-radius: 6px;
                background-color: #717575;
                padding: 6px;
                text-align: center;
            }
            QLabel:hover {
                background-color: #525151;
            }
        """

scale_factor = 0
old_scale_factor = 1
rotation_scale_factorx = 1
rotation_scale_factory = 1
rotation_scale_factorz = 1
rotation_direction = 0
thetax = 0  # in radians
thetay = 0  # in radians
thetaz = 0  # in radians
app = QApplication(sys.argv)
theremin_x = 0.0625
theremin_y = -0.0625
theremin_zoom_mode = 0
theremin_go = 0

is_rotating = 0

mode_compiler = 1
z_minimum = "-5"
z_maximum = "5"

#ser = serial.Serial(port = '/dev/tty.usbmodem1101', baudrate = 9600)


def read_theremin():
    theremin_direction = 'z'
    global theremin_x
    global theremin_y
    global theremin_zoom_mode
    global theremin_go
    global thetax
    global thetay
    global thetaz 
    global rotation_scale_factorx
    global rotation_scale_factory
    global rotation_scale_factorz
    global scale_factor
    while True:
        

        value = ser.read(1)
        int_received = int.from_bytes(value, byteorder='little')
        theremin_go = int_received & 1
        theremin_zoom_mode = (int_received >> 1) & 1
        theremin_y = (int_received >> 2) & 7
        theremin_x = (int_received >> 5) & 7

        if((theremin_x != 1) and (theremin_y == 1) ):
            theremin_direction = 'x'
        elif((theremin_x == 1) and (theremin_y != 1) ):
            theremin_direction = 'y'
        if(theremin_go):
            if(theremin_zoom_mode):
                if (theremin_direction == 'x'):
                    thetaz += ((theremin_x) - 1) * 0.0625
                    rotation_scale_factorz = (thetaz * 180) / np.pi
                    window.display_rotation(rotation_scale_factorz, 'z')
                elif(theremin_direction == 'y'):
                    scale_factor = ((theremin_y) - 1) * 0.1
                else:
                    scale_factor = 0
            elif (theremin_direction == 'x'):
            
                thetax += ((theremin_x) - 1) * 0.0625
                rotation_scale_factorx = (thetax * 180) / np.pi
                window.display_rotation(rotation_scale_factorx, 'x')
            else:
                thetay += ((theremin_y) - 1) * 0.0625
                rotation_scale_factory = (thetay * 180) / np.pi
                window.display_rotation(rotation_scale_factory, 'y')
        window.on_button_clicked()


def gestureEvent(event):
    pinch = event.gesture(Qt.PinchGesture)
    if pinch:
        if pinch.state() == Qt.GestureStarted:
            pass
        elif pinch.state() == Qt.GestureUpdated:
            global scale_factor
            if(pinch.scaleFactor() > 1):
                scale_factor = scale_factor + pinch.scaleFactor() * 0.01
            else:
                scale_factor = scale_factor - (1/pinch.scaleFactor()) * 0.01
            window.display_scale_factor(scale_factor)
                
            window.scale_factor_updated.emit(scale_factor)
        elif pinch.state() == Qt.GestureFinished:
            pass
    return True

class EventFilter(QObject):
    def eventFilter(self, obj, event):
        if event.type() == QEvent.Gesture:
            gestureEvent(event)
            return True
        elif event.type() == QEvent.MouseButtonPress:
            if window.rotateL_button.geometry().contains(event.pos()):
                start_rotating(0, 'z')
                window.rotateL_button.setStyleSheet(PRESSED)
                return True
            elif window.rotateR_button.geometry().contains(event.pos()):
                start_rotating(1, 'z')
                window.rotateR_button.setStyleSheet(PRESSED)
                return True
            elif window.rotateXAnti_button.geometry().contains(event.pos()):
                start_rotating(0, 'x')
                window.rotateXAnti_button.setStyleSheet(PRESSED)
                return True
            elif window.rotateXClock_button.geometry().contains(event.pos()):
                start_rotating(1, 'x')
                window.rotateXClock_button.setStyleSheet(PRESSED)
                return True
            elif window.rotateYAnti_button.geometry().contains(event.pos()):
                start_rotating(0, 'y')
                window.rotateYAnti_button.setStyleSheet(PRESSED)
                return True
            elif window.rotateYClock_button.geometry().contains(event.pos()):
                start_rotating(1, 'y')
                window.rotateYClock_button.setStyleSheet(PRESSED)
                return True
        elif event.type() == QEvent.MouseButtonRelease:
            if is_rotating:
                stop_rotating()
            window.rotateL_button.setStyleSheet(UNPRESSED)
            window.rotateR_button.setStyleSheet(UNPRESSED)
            window.rotateXAnti_button.setStyleSheet(UNPRESSED)
            window.rotateXClock_button.setStyleSheet(UNPRESSED)
            window.rotateYAnti_button.setStyleSheet(UNPRESSED)
            window.rotateYClock_button.setStyleSheet(UNPRESSED)
            
            return True
        return False
    
class Constantin(QWidget):
    def __init__(self):
        super().__init__()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if len(sys.argv) > 1:
            self.addr = sys.argv[1]
        else:
            self.addr = "192.168.2.99"
        self.setGeometry(0, 100, 500, 400)  # position x, position y, width, height
        self.setWindowTitle('Constantin\'s Window just for him')
        self.layout = QVBoxLayout(self)
        self.setLayout(self.layout)
        top_layout = QHBoxLayout()

        self.label = QLabel('<h2>Constantin your welcome</h2>', self)
        top_layout.addWidget(self.label)
        self.Qpush = QPushButton('Switch to GUI', self)
        self.Qpush.clicked.connect(switch_constantin)
        top_layout.addWidget(self.Qpush)
        self.submit = QPushButton('Submit', self)
        self.submit.clicked.connect(self.enter_pressed)
        top_layout.addWidget(self.submit)
        self.layout.addLayout(top_layout)

        self.textbox = QTextEdit(self)
        self.layout.addWidget(self.textbox)
        self.show()


    def enter_pressed(self):
        Received = self.textbox.toPlainText()
        
        process = subprocess.Popen(
            ["../compiler/bin/conv"],
            stdin = subprocess.PIPE, # connect stdin over pipe
            stdout = subprocess.PIPE, # connect stdout over pipe
            stderr = subprocess.PIPE,
            text = True
        )
        send_str = Received
        vals, err = process.communicate(input=send_str)
        process.terminate()
        if process.returncode != 0:
            print(f"Error: {process.returncode}")
            print(send_str)
            self.setWindowTitle("Error Detected, Deleting Source Code...")
            return
        
        process = subprocess.Popen(
            ["../assembler/bin/assembler", "-f", "bin"],
            stdin = subprocess.PIPE, # connect stdin over pipe
            stdout = subprocess.PIPE, # connect stdout over pipe
            stderr = subprocess.PIPE,
        )
        vals, err = process.communicate(input=vals.encode('utf-8'))
        process.terminate()
        if err:
            #print(err)
            pass
        if process.returncode != 0:
            print(f"Error: {process.returncode}")
            return
        self.socket.sendto(vals, (self.addr, SEND_PORT))


class MainWindow(QWidget):
    scale_factor_updated = pyqtSignal(float)
    stopped_rotating = pyqtSignal()
    started_rotating = pyqtSignal(tuple)

    def __init__(self):
        super().__init__()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if len(sys.argv) > 1:
            self.addr = sys.argv[1]
        else:
            self.addr = "192.168.2.99"
        self.setWindowTitle('Epic GUI')
        self.setGeometry(0, 100, 500, 400)  # position x, position y, width, height

        layout = QVBoxLayout(self)
        self.setLayout(layout)

        self.fps_label = QLabel('FPS: 0', self)
        self.fps_label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.fps_label.setAlignment(Qt.AlignRight)
        fps_bar_layout = QHBoxLayout()

        self.con_button = QPushButton('Switch to Constantin', self)
        self.con_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.con_button.clicked.connect(switch_constantin)

        fps_bar_layout.addWidget(self.con_button)

        self.EPIC_button = QPushButton('EPIC MODE', self)
        self.EPIC_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.EPIC_button.clicked.connect(EPIC_MODE)
        self.EPIC_button.setStyleSheet("""
            QPushButton {
                border: 4px solid #FF0000;
                border-radius: 6px;
                background-color: #00FF00;
                padding: 6px;
                text-align: center;
                color: #0000FF;
            }
            QPushButton:hover {
                background-color: #35b7de;
            }
        """)
        

        fps_bar_layout.addWidget(self.EPIC_button)

        fps_bar_layout.addWidget(self.fps_label)
        layout.addLayout(fps_bar_layout)

        helloMsg = QLabel('<h1>EPIC GUI!!!</h1>', self)
        helloMsg.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        helloMsg.setAlignment(Qt.AlignCenter)
        layout.addWidget(helloMsg)

        self.InputBox = QLineEdit(self)
        self.InputBox.setPlaceholderText('Enter your desired maths function')
        self.InputBox.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.MinimumExpanding)
        layout.addWidget(self.InputBox)

        button = QPushButton('Submit', self)
        shortcut = QShortcut(Qt.Key_Return, self)
        button.clicked.connect(self.on_button_clicked)
        shortcut.activated.connect(button.click)
        button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        layout.addWidget(button)

        self.timer = QTimer(self)
        self.timer.label = QLabel('Rotating : 0', self)
        self.timer.label.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.timer.label.setAlignment(Qt.AlignCenter)
        layout.addWidget(self.timer.label)
        self.timer.timeout.connect(self.on_rotate_button_held)

        self.mode_button = QPushButton('Switch to 3D Mode', self)
        self.mode_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.mode_button.setCheckable(True)
        self.mode_button.clicked.connect(self.on_mode_button_clicked)
        self.mode_layout = QHBoxLayout()
        self.mode_layout.addWidget(self.mode_button)
        layout.addLayout(self.mode_layout)        

        button_layout = QVBoxLayout()
        layout.addLayout(button_layout)

        rotate_buttons_layout = QHBoxLayout()
        button_layout.addLayout(rotate_buttons_layout)

        self.rotateL_button = QLabel('Rotate Z Anticlockwise', self)
        self.rotateL_button.setStyleSheet(UNPRESSED)
        self.rotateL_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateL_button.setAlignment(Qt.AlignCenter)
        rotate_buttons_layout.addWidget(self.rotateL_button)

        self.rotateR_button = QLabel('Rotate Z Clockwise', self)
        self.rotateR_button.setStyleSheet(UNPRESSED)
        self.rotateR_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateR_button.setAlignment(Qt.AlignCenter)
        rotate_buttons_layout.addWidget(self.rotateR_button)

        rotate_x_buttons_layout = QHBoxLayout()
        button_layout.addLayout(rotate_x_buttons_layout)

        self.rotateXAnti_button = QLabel('Rotate X Anticlockwise', self)
        self.rotateXAnti_button.setStyleSheet(UNPRESSED)
        self.rotateXAnti_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateXAnti_button.setAlignment(Qt.AlignCenter)
        rotate_x_buttons_layout.addWidget(self.rotateXAnti_button)

        self.rotateXClock_button = QLabel('Rotate X Clockwise', self)
        self.rotateXClock_button.setStyleSheet(UNPRESSED)
        self.rotateXClock_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateXClock_button.setAlignment(Qt.AlignCenter)
        rotate_x_buttons_layout.addWidget(self.rotateXClock_button)

        rotate_y_buttons_layout = QHBoxLayout()
        button_layout.addLayout(rotate_y_buttons_layout)

        self.rotateYAnti_button = QLabel('Rotate Y Anticlockwise', self)
        self.rotateYAnti_button.setStyleSheet(UNPRESSED)
        self.rotateYAnti_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateYAnti_button.setAlignment(Qt.AlignCenter)
        rotate_y_buttons_layout.addWidget(self.rotateYAnti_button)

        self.rotateYClock_button = QLabel('Rotate Y Clockwise', self)
        self.rotateYClock_button.setStyleSheet(UNPRESSED)
        self.rotateYClock_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateYClock_button.setAlignment(Qt.AlignCenter)
        rotate_y_buttons_layout.addWidget(self.rotateYClock_button)
        self.rotateYClock_button.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.rotateYClock_button.setAlignment(Qt.AlignCenter)
        rotate_y_buttons_layout.addWidget(self.rotateYClock_button)

        self.grabGesture(Qt.PinchGesture)

        self.scale_value = QLabel('Scale Value Pending : ' + str(scale_factor), self)
        self.scale_value.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.scale_value.setAlignment(Qt.AlignCenter)
        layout.addWidget(self.scale_value)

        self.total_scale_value = QLabel('Total Scale Value : ' + str(scale_factor), self)
        self.total_scale_value.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.total_scale_value.setAlignment(Qt.AlignCenter)
        layout.addWidget(self.total_scale_value)

        self.event_filter = EventFilter()
        self.installEventFilter(self.event_filter)

        self.scale_factor_updated.connect(self.update_scale_factor)
        self.stopped_rotating.connect(self.stopping_rotating)
        self.started_rotating.connect(self.on_rotate_clicked)

        self.on_mode_button_clicked()

    def resizeEvent(self, event):
        self.update_element_sizes()
        super().resizeEvent(event)

    def update_element_sizes(self):
        width = self.width()
        height = self.height()
        scale_factor = min(width / 500, height / 400)

        font_size = max(10, int(16 * scale_factor))
        self.setStyleSheet(f"font-size: {font_size}px;")

    def update_scale_factor(self, new_scale_factor):
        global scale_factor
        scale_factor = new_scale_factor

    def display_scale_factor(self, scale_factor):
        self.scale_value.setText(f'Scale Value Pending : {scale_factor:.5f}')

    def display_rotation(self, rotation, axis):
        self.timer.label.setText(f'Rotating {axis.upper()} (degrees): {rotation:.2f}')



    def on_button_clicked(self):
        global old_scale_factor
        global z_minimum
        global z_maximum

        if (not mode_compiler):
            if(self.min_z.text().strip() == ""):
                z_minimum = "-5"
            else:
                z_minimum = self.min_z.text()
            if(self.max_z.text().strip() == ""):
                z_maximum = "5"
            else:
                z_maximum = self.max_z.text()
            try:
                int(z_minimum)
                int(z_maximum)
                if(int(z_minimum) >= int(z_maximum)):
                    raise ValueError
            except ValueError:
                self.timer.label.setText(f'Error: You Think your clever dont you ;)')
                return
            except:
                self.timer.label.setText(f'Error: Invalid Range')
                return
            

        Received = self.InputBox.text()
        current_scale_factor = old_scale_factor + scale_factor #was multiply
        if float(current_scale_factor).is_integer():
            Received = Received.replace("x", "(x/" + str(current_scale_factor) + ".)")
            Received = Received.replace("y", "(y/" + str(current_scale_factor) + ".)")
        else:
            Received = Received.replace("x", "(x/" + str(current_scale_factor) + ")")
            Received = Received.replace("y", "(y/" + str(current_scale_factor) + ")")
        x_rotation = f"{thetax} "
        y_rotation = f"{thetay} "
        z_rotation = f"{thetaz} "
        old_scale_factor = current_scale_factor
        self.total_scale_value.setText(f'Total Scale Value : {current_scale_factor:.5f}')
        process = subprocess.Popen(
            ["../compiler/bin/conv", "-s"],
            stdin = subprocess.PIPE, # connect stdin over pipe
            stdout = subprocess.PIPE, # connect stdout over pipe
            stderr = subprocess.PIPE,
            text = True
        )
        if(mode_compiler):
            send_str = ".plotxy " + x_rotation + y_rotation + z_rotation + Received
        else:
            send_str = ".simple_plotxy " + z_minimum + ' ' + z_maximum + ' ' + Received #TODO add lims
        vals, err = process.communicate(input=send_str)
        process.terminate()
        if err:
            #print(err)
            pass
        if process.returncode != 0:
            print(f"Error: {process.returncode}")
            print(send_str)
            self.timer.label.setText(f'Error: Invalid Function')
            self.setWindowTitle("Error Detected, Deleting Source Code...")
            return
        for program in vals.split('<'):
            if (len(program.splitlines()) > 256):
                print(len(program.splitlines()))
                print(program)
                self.timer.label.setText(f'Error: Function too complex sorry :,(')
                return
        
        process = subprocess.Popen(
            ["../assembler/bin/assembler", "-f", "bin"],
            stdin = subprocess.PIPE, # connect stdin over pipe
            stdout = subprocess.PIPE, # connect stdout over pipe
            stderr = subprocess.PIPE,
        )
        vals, err = process.communicate(input=vals.encode('utf-8'))
        process.terminate()
        if err:
            #print(err)
            pass
        if process.returncode != 0:
            print(f"Error: {process.returncode}")
            self.timer.label.setText(f'Assembler Error CODE: {process.returncode}')
            return
        else:
            self.timer.label.setText(f'Plotting Your Function: ')
        self.socket.sendto(vals, (self.addr, SEND_PORT))
        #print(".plotxy " + x_rotation + y_rotation + z_rotation + Received) # for testing purposes

    def on_rotate_clicked(self, direction_axis):
        direction, axis = direction_axis
        global rotation_direction
        rotation_direction = -1 if direction == 0 else 1
        self.current_axis = axis
        self.timer.start(10)

    def stopping_rotating(self):
        self.timer.stop()
        global thetax, thetay, thetaz
        if self.current_axis == 'x':
            thetax = (rotation_scale_factorx / 180) * np.pi
        elif self.current_axis == 'y':
            thetay = (rotation_scale_factory / 180) * np.pi
        elif self.current_axis == 'z':
            thetaz = (rotation_scale_factorz / 180) * np.pi

    def on_rotate_button_held(self):
        global rotation_scale_factorx
        global rotation_scale_factory
        global rotation_scale_factorz
        if rotation_scale_factorx > 360:
            rotation_scale_factorx -= 360
        elif rotation_scale_factory > 360:
            rotation_scale_factory -= 360
        elif rotation_scale_factorz > 360:
            rotation_scale_factorz -= 360
        elif rotation_scale_factorx < -360:
            rotation_scale_factorx += 360
        elif rotation_scale_factory < -360:
            rotation_scale_factory += 360
        elif rotation_scale_factorz < -360:
            rotation_scale_factorz += 360
        if self.current_axis == 'x':   
            rotation_scale_factorx += 0.9 * rotation_direction
            self.timer.label.setText(f'Rotating {self.current_axis.upper()} (degrees): {rotation_scale_factorx:.2f}')
        elif self.current_axis == 'y':
            rotation_scale_factory += 0.9 * rotation_direction
            self.timer.label.setText(f'Rotating {self.current_axis.upper()} (degrees): {rotation_scale_factory:.2f}')
        elif self.current_axis == 'z':
            rotation_scale_factorz += 0.9 * rotation_direction
            self.timer.label.setText(f'Rotating {self.current_axis.upper()} (degrees): {rotation_scale_factorz:.2f}')
        self.timer.label.adjustSize()

    def on_mode_button_clicked(self):
        global mode_compiler
        mode_compiler = not mode_compiler
        if not mode_compiler:
            self.mode_button.setText('Switch to 3D Mode')
            self.min_z = QLineEdit(self)
            self.min_z.setPlaceholderText('Enter min range')
            self.min_z.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.MinimumExpanding)
            self.mode_layout.addWidget(self.min_z)
            self.max_z = QLineEdit(self)
            self.max_z.setPlaceholderText('Enter max range')
            self.max_z.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.MinimumExpanding)
            self.mode_layout.addWidget(self.max_z)
            
        else:
            self.mode_button.setText('Switch to 2D Mode')
            self.mode_layout.removeWidget(self.max_z)
            self.max_z.deleteLater()
            self.mode_layout.removeWidget(self.min_z)
            self.min_z.deleteLater()
        

def read_input_and_update_scale():
    while True:
        global theremin_go
        if theremin_go:
            print("theremin go")

def stop_rotating():
    window.stopped_rotating.emit()

def start_rotating(direction, axis):
    window.started_rotating.emit((direction, axis))
    global is_rotating
    is_rotating = 1

def display_fps(fps):
    window.fps_label.setText(f'FPS: {fps:.0f}')

def switch_constantin():
    if(window.isVisible()):
        window.hide()
        constantin.show()
    else:
        constantin.hide()
        window.show()

def EPIC_MODE():
    if(not music_thread.is_alive()):
        music_thread.start()
    
    
def play_sound():
    file_path = '../GUI/music/shreksophone.mp3'
    sound = AudioSegment.from_file(file_path)
    while True:
        play(sound)
        
    

def receive_fps():
    host='0.0.0.0'
    port=65434
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        s.bind((host, port))
        #print(f"Server started on {host}:{port}")
        while True:
            data,addr = s.recvfrom(1024)
            #print(f"Received data: {data.decode('utf-8')}")
            fps = float(data.decode('utf-8'))
            display_fps(fps)
            time.sleep(1)

window = MainWindow()
window.show()

constantin = Constantin()
constantin.hide()

# theremin_thread = threading.Thread(target=read_theremin, daemon=True)
# theremin_thread.start()

fps_thread = threading.Thread(target=receive_fps, daemon=True)
fps_thread.start()

music_thread = threading.Thread(target=play_sound, daemon=True)

sys.exit(app.exec_())
