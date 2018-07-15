from picamera import PiCamera
from time import sleep
import tkinter as tk
import os.path

camera = PiCamera()
camera.resolution = (1640,1232)
camera.framerate = 30
camera.exposure_mode='off'
camera.iso=0


i=0 # picture counter
j=0 # video counter
# new_file = Path('/home/pi/Desktop/video_'+str(j)+'.h264')

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
    global j
    filename = '/home/pi/Desktop/video_'+str(j)+'.h264'
    if os.path.isfile(filename):
        j+=1
        startRecord()
    else:
        camera.start_recording('/home/pi/Desktop/video_'+str(j)+'.h264')
        j+=1
def stopRecord():
    start_btn.configure(bg="#3CB371")
    camera.stop_recording()
def takePicture():
    global i
    filename = '/home/pi/Desktop/image_'+str(i)+'.jpg'
    if os.path.isfile(filename):
        i+=1
        takePicture()
    else:
        camera.capture('/home/pi/Desktop/image_'+str(i)+'.jpg') 
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




