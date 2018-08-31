from picamera import PiCamera
import datetime
from time import sleep
import tkinter as tk
import os.path
import RPi.GPIO as GPIO
import io
import numpy as np

camera = PiCamera()
camera.resolution = (1640,1232)
camera.framerate = 30
camera.exposure_mode='off'
camera.iso=0

# ********************** initialize GPIO
GPIO.setmode(GPIO.BOARD)
# we are going to trigger via low->high
# so pull down the trigger pin (can emulate on chip)
# 11, GPIO17 is on trigger
# 12, GPIO18 is off trigger
GPIO.setup(11, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)
GPIO.setup(12, GPIO.IN,pull_up_down=GPIO.PUD_DOWN)


# button callback fxns
def startPreview():
    print(float(camera.analog_gain))
    print(float(camera.exposure_speed))
    preview_btn.configure(bg="gainsboro")
    camera.start_preview(fullscreen=False, window=(0,50,1280,720))
def stopPreview():
    preview_btn.configure(bg="#3CB371")
    camera.stop_preview()
def startRecord():
    start_btn.configure(bg="gainsboro")
    cStr=datetime.datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
    output = np.empty((1232,1640,3),dtype=np.uint8)
    filename = '/home/pi/Desktop/Captures/video_'+cStr+'.h264'
    waitForPin=1
    while waitForPin:
        checkPin=GPIO.input(11)
        if checkPin==1:
            waitForPin=0
            #camera.capture(output,'rgb')
            camera.start_recording('/home/pi/Desktop/Captures/video_'+cStr+'.h264',sps_timing=True)
            camera.start_preview(fullscreen=False, window=(0,50,1280,720))


def stopRecord():
    waitForPin=0
    start_btn.configure(bg="#3CB371")
    camera.stop_recording()
    camera.stop_preview()
def takePicture():
    cStr=datetime.datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
    filename = '/home/pi/Desktop/Captures/image_'+cStr+'.jpg'
    if os.path.isfile(filename):
        i+=1
        takePicture()
    else:
        camera.capture('/home/pi/Desktop/Captures/image_'+cStr+'.jpg') 
        i+=1

# simple button GUI
window = tk.Tk()
window.title("Recording Controls")
preview_btn = tk.Button(window, text="Start Preview", command=startPreview, height=4, width=15, bg="#3CB371")
preview_btn.pack()
start_btn = tk.Button(window, text="Start Record", command=startRecord, height=4, width=15, bg="#3CB371")
start_btn.pack()
stop_btn = tk.Button(window, text="Stop Record", command=stopRecord, height=4, width=15, bg="#CD5C5C")
stop_btn.pack()
pic_btn = tk.Button(window, text="Take Picture", command=takePicture, height=4, width=15, bg="#FFFACD")
pic_btn.pack()
end_btn = tk.Button(window, text="Exit Preview", command=stopPreview, height=4, width=15)
end_btn.pack()
window.mainloop()




