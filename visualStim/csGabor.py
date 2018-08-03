import sys
from psychopy import visual, core, event, data, clock 

import numpy as np
import serial 

teensyObj = serial.Serial("/dev/ttyS0",9600)
teensyObj.close()
teensyObj.open()
#teensyObj.write(bytes(b'a0>'))


#create a window
win_x1=400
win_y1=400


mywin = visual.Window([win_x1,win_y1],allowGUI=False)
exp = data.ExperimentHandler(dataFileName="ydo")


# timing etc.
startFlag=0
timeOffset=0
curTime=0
lc=0
serTrack=0
runSession=1

# stim 1 params
g1={'phaseDelta':0.05,'Xpos':0,'Ypos':0,'spFreq':[4,0],'mask':'gauss',
    'size':(1.0,1.0),'contrast':0.0,'orientation':0}

#create some stimuli
grating1= visual.GratingStim(win=mywin, mask=g1['mask'], 
    pos=[g1['Xpos'],g1['Ypos']],sf=g1['spFreq'],
    ori=g1['orientation'])
grating1.contrast = g1['contrast']
grating1.autoDraw=True

# draw stim refresh monitor
while runSession: #this creates a never-ending loop
    serTrack=0
    bytesAvail=teensyObj.inWaiting()
    # 4,3,5,6,7
    if bytesAvail>0:

        sR=teensyObj.readline().strip().decode()
        sR=sR.split(',')
        if len(sR)==6 and sR[0]=='v':
            if int(sR[2])==999:
                runSession=0
                sR[2]=0
            g1['contrast']=int(sR[2])/100
            g1['orientation']=int(sR[1])
            g1['spFreq']=int(sR[3])/100
            serTrack=1
            
            grating1.contrast = g1['contrast']
            grating1.ori=g1['orientation']
            grating1.size = g1['size']

 

    if startFlag==0:
        timeOffset=clock.getTime()
        startFlag=1
    grating1.phase += g1['phaseDelta']
    curTime=clock.getTime()-timeOffset
    # grating1.draw()


    mywin.flip()
    lc=lc+1

    # save
    exp.addData('clockTime', curTime)
    exp.addData('g1_phase', g1['Xpos'])
    exp.addData('g1_spFreq', g1['spFreq'])
    exp.addData('g1_size', g1['size'])
    exp.addData('g1_contrast', g1['contrast'])
    exp.addData('serTrack',serTrack)
    exp.nextEntry()
    if len(event.getKeys())>0:
        break
    event.clearEvents()

# end session

exp.close()

# exp.saveAsPickle("testSave")
exp.saveAsWideText("testSaveTxt")
mywin.close()
core.quit()
