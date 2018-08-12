# csBehavior v0.99
# 
#
# GUI is all class based now.
# still todo: __ fully decouple engine from UI parsearg; __ asyncio (i more or less have manual yields)
# critical: reference to TVs in task!
# 
# 
# 8/11/2018
# Chris Deister - cdeister@brown.edu
# Anything that is licenseable is governed by a MIT License found in the github directory. 


from tkinter import *
import tkinter.filedialog as fd
import serial
import numpy as np
import h5py
import os
import datetime
import time
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import socket
import sys
import pygsheets
from Adafruit_IO import Client
import pandas as pd
from scipy.stats import norm

root = Tk()


class csGUI(object):
	
	def __init__(self, master, varDict):
		self.master = master

		c1Wd=14
		c2Wd=8
		cpRw=0

		self.taskBar = Frame(self.master)
		self.master.title("csVisual")
		self.tb = Button(self.taskBar,text="set path",width=8,command=lambda: self.getPath(varDict))
		self.tb.grid(row=cpRw,column=1)
		self.dirPath_label=Label(self.taskBar, text="Save Path:", justify=LEFT)
		self.dirPath_label.grid(row=cpRw,column=0,padx=0,sticky=W)
		self.dirPath_TV=StringVar(self.taskBar)
		self.dirPath_TV.set(varDict['dirPath'])
		self.dirPath_entry=Entry(self.taskBar, width=24, textvariable=self.dirPath_TV)
		self.dirPath_entry.grid(row=cpRw+1,column=0,padx=0,columnspan=2,sticky=W)
		
		cpRw=2
		self.comPath_teensy_label=Label(self.taskBar, text="COM (Teensy) path:", justify=LEFT)
		self.comPath_teensy_label.grid(row=cpRw,column=0,padx=0,sticky=W)
		self.comPath_teensy_TV=StringVar(self.taskBar)
		self.comPath_teensy_TV.set(varDict['comPath_teensy'])
		self.comPath_teensy_entry=Entry(self.taskBar, width=24, textvariable=self.comPath_teensy_TV)
		self.comPath_teensy_entry.grid(row=cpRw+1,column=0,padx=0,columnspan=2,sticky=W)
		
		beRW=4
		self.baudEntry_label = Label(self.taskBar,text="BAUD Rate:",justify=LEFT)
		self.baudEntry_label.grid(row=beRW, column=0,sticky=W)
		self.baudSelected=IntVar(self.taskBar)
		self.baudSelected.set(19200)
		self.baudPick = OptionMenu(self.taskBar,self.baudSelected,19200,115200,9600,500000)
		self.baudPick.grid(row=beRW, column=1,sticky=W)
		self.baudPick.config(width=8)

		sbRw=5
		self.subjID_label=Label(self.taskBar, text="Subject ID:", justify=LEFT)
		self.subjID_label.grid(row=sbRw,column=0,padx=0,sticky=W)
		self.subjID_TV=StringVar(self.taskBar)
		self.subjID_TV.set(varDict['subjID'])
		self.subjID_entry=Entry(self.taskBar, width=10, textvariable=self.subjID_TV)
		self.subjID_entry.grid(row=sbRw,column=1,padx=0,sticky=W)

		ttRw=6
		self.teL=Label(self.taskBar, text="Total Trials:",justify=LEFT)
		self.teL.grid(row=ttRw,column=0,padx=0,sticky=W)
		self.totalTrials_TV=StringVar(self.taskBar)
		self.totalTrials_TV.set(varDict['totalTrials'])
		self.te = Entry(self.taskBar, text="Quit",width=10,textvariable=self.totalTrials_TV)
		self.te.grid(row=ttRw,column=1,padx=0,sticky=W)
		
		
		ttRw=7
		self.teL=Label(self.taskBar, text="Current Session:",justify=LEFT)
		self.teL.grid(row=ttRw,column=0,padx=0,sticky=W)
		self.curSession_TV=StringVar(self.taskBar)
		self.curSession_TV.set(varDict['curSession'])
		self.te = Entry(self.taskBar,width=10,textvariable=self.curSession_TV)
		self.te.grid(row=ttRw,column=1,padx=0,sticky=W)

		lcThrRw=8
		self.lickAThr_label=Label(self.taskBar, text="Lick Thresh:", justify=LEFT)
		self.lickAThr_label.grid(row=lcThrRw,column=0,padx=0,sticky=W)
		self.lickAThr_TV=StringVar(self.taskBar)
		self.lickAThr_TV.set(varDict['lickAThr'])
		self.lickAThr_entry=Entry(self.taskBar, width=10, textvariable=self.lickAThr_TV)
		self.lickAThr_entry.grid(row=lcThrRw,column=1,padx=0,sticky=W)

		lcThrRw=9
		self.minStimTime_label=Label(self.taskBar, text="Min Stim Time:", justify=LEFT)
		self.minStimTime_label.grid(row=lcThrRw,column=0,padx=0,sticky=W)
		self.minStimTime_TV=StringVar(self.taskBar)
		self.minStimTime_TV.set(varDict['minStimTime'])
		self.minStimTime_entry=Entry(self.taskBar, width=10, textvariable=self.minStimTime_TV)
		self.minStimTime_entry.grid(row=lcThrRw,column=1,padx=0,sticky=W)

		btnRw=10
		self.shapingTrial_TV=IntVar()
		self.shapingTrial_TV.set(varDict['shapingTrial'])
		self.shapingTrial_Toggle=Checkbutton(self.taskBar,text="Shaping Trial",\
			variable=self.shapingTrial_TV,onvalue=1,offvalue=0)
		self.shapingTrial_Toggle.grid(row=btnRw,column=0,sticky=W)
		self.shapingTrial_Toggle.select()

		# Main Buttons
		ttRw=11
		self.blL=Label(self.taskBar, text=" ——————— ",justify=LEFT)
		self.blL.grid(row=ttRw,column=0,padx=0,sticky=W)

		cprw=12
		self.chanPlotIV=IntVar()
		self.chanPlotIV.set(varDict['chanPlot'])
		Radiobutton(self.taskBar, text="Load Cell", \
			variable=self.chanPlotIV, value=4).grid(row=cprw,column=0,padx=0,sticky=W)
		Radiobutton(self.taskBar, text="Lick Sensor", \
			variable=self.chanPlotIV, value=5).grid(row=cprw+1,column=0,padx=0,sticky=W)
		Radiobutton(self.taskBar, text="Motion", \
			variable=self.chanPlotIV, value=6).grid(row=cprw+2,column=0,padx=0,sticky=W)
		Radiobutton(self.taskBar, text="Scope", \
			variable=self.chanPlotIV, value=8).grid(row=cprw,column=1,padx=0,sticky=W)
		Radiobutton(self.taskBar, text="Thr Licks", \
			variable=self.chanPlotIV, value=9).grid(row=cprw+1,column=1,padx=0,sticky=W)
		Radiobutton(self.taskBar, text="Nothing", \
			variable=self.chanPlotIV, value=0).grid(row=cprw+2,column=1,padx=0,sticky=W)


		# MQTT Stuff

		ttRw=15
		self.blL=Label(self.taskBar, text=" ——————— ",justify=LEFT)
		self.blL.grid(row=ttRw,column=0,padx=0,sticky=W)

		btnRw=16
		self.logMQTT_TV=IntVar()
		self.logMQTT_TV.set(varDict['chanPlot'])
		self.logMQTT_Toggle=Checkbutton(self.taskBar,text="Log MQTT Info?",\
			variable=self.logMQTT_TV,onvalue=1,offvalue=0)
		self.logMQTT_Toggle.grid(row=btnRw,column=0)
		self.logMQTT_Toggle.select()

		ttRw=17
		self.hpL=Label(self.taskBar, text="Hash Path:",justify=LEFT)
		self.hpL.grid(row=ttRw,column=0,padx=0,sticky=W)
		self.hashPath_TV=StringVar(self.taskBar)
		self.hashPath_TV.set(varDict['hashPath'])
		self.te = Entry(self.taskBar,width=10,textvariable=self.hashPath_TV)
		self.te.grid(row=ttRw,column=1,padx=0,sticky=W)

		ttRw=18
		self.vpR=Label(self.taskBar, text="Vol/Rwd (~):",justify=LEFT)
		self.vpR.grid(row=ttRw,column=0,padx=0,sticky=W)
		self.volPerRwd_TV=StringVar(self.taskBar)
		self.volPerRwd_TV.set(varDict['volPerRwd'])
		self.te = Entry(self.taskBar,width=10,textvariable=self.volPerRwd_TV)
		self.te.grid(row=ttRw,column=1,padx=0,sticky=W)
		
		# Main Buttons
		ttRw=19
		self.blL=Label(self.taskBar, text=" ——————— ",justify=LEFT)
		self.blL.grid(row=ttRw,column=0,padx=0,sticky=W)

		btnRw=20
		self.tc = Button(self.taskBar,text="Task: Detection",width=c1Wd,command=self.do_task)
		self.tc.grid(row=btnRw,column=0)
		self.tc['state'] = 'disabled'
		print(self.tc['state'])

		btnRw=20

		self.quitButton = Button(self.taskBar,text="Quit",width=c1Wd,command=lambda: self.closeup(varDict))
		self.quitButton.grid(row=btnRw+1,column=0)
		
		self.stimButton = Button(self.taskBar,text="Stim",width=c1Wd,command= lambda: self.makeDevControl(varDict))
		self.stimButton.grid(row=btnRw+1,column=1)
		self.taskBar.pack(side=TOP, fill=X)

	def getPath(self,varDict):
		try:
			selectPath = fd.askdirectory(title ="what what?")
		except:
			selectPath='/'

		self.dirPath_TV.set(selectPath)
		self.subjID_TV.set(os.path.basename(selectPath))
		varDict['dirPath']=selectPath
		varDict['subjID']=os.path.basename(selectPath)
		self.tc['state'] = 'normal'
		# if there is a csVar.sesVarDict.csv load it. 
		try:
			tempMeta=pd.read_csv(selectPath +'/' + 'sesVars.csv',index_col=0,header=None)
			for x in range(0,len(tempMeta)):
				varKey=tempMeta.iloc[x].name
				varVal=tempMeta.iloc[x][1]
				
				# now we need to divine curVar's data type.
				# we first try to see if it is numeric.
				try:
					tType=float(varVal)
					if int(tType)==tType:
						tType=int(tType)
					# update any text variables that may exist.
					try:
						exec('self.' + varKey + '_TV.set({})'.format(tType))
					except:
						g=1
				except:
					tType=varVal
					# update any text variables that may exist.
					try:
						exec('self.' + varKey + '_TV.set("{}")'.format(tType))
					except:
						g=1
				varDict[varKey]=tType
		except:
			g=1
		# update_GVars()
	def updateDictFromGUI(self,varDict):
		for key in list(varDict.keys()):
			try:
				a=eval('self.{}_TV.get()'.format(key))                
				try:
					a=float(a)
					if a.is_integer():
						a=int(a)
					exec('varDict["{}"]={}'.format(key,a))
				except:
					exec('varDict["{}"]="{}"'.format(key,a))
			except:
				g=1
		return varDict
	def dictToPandas(self,varDict):
			curKey=[]
			curVal=[]
			for key in list(varDict.keys()):
				curKey.append(key)
				curVal.append(varDict[key])
				self.pdReturn=pd.Series(curVal,index=curKey)
			return self.pdReturn
	def closeup(self,varDict):
		# self.tc['state'] = 'normal'
		self.updateDictFromGUI(varDict)
		try:
			self.sesVarDict_bindings=self.dictToPandas(varDict)
			self.sesVarDict_bindings.to_csv(varDict['dirPath'] + '/' +'sesVars.csv')
		except:
			g=1

		try:
			varDict['sessionOn']=0
		except:
			varDict['canQuit']=1
			quitButton['text']="Quit"


		if varDict['canQuit']==1:
			# try to close a plot and exit    
			try:
				plt.close(varDict['detectPlotNum'])
				os._exit(1)
			# else exit
			except:
				os._exit(1)
	
	# This just decouples
	def do_task(self):
		runDetectionTask()

	def commandTeensy(self,varDict,commandStr):
		varDict['comPath_teensy']=self.comPath_teensy_TV.get()
		teensy=csSer.connectComObj(varDict['comPath_teensy'],varDict['baudRate_teensy'])
		teensy.write("{}".format(commandStr).encode('utf-8'))
		teensy.close()
		return varDict
		
	def deltaTeensy(self,varDict,commandHeader,delta):
		varDict['comPath_teensy']=self.comPath_teensy_TV.get()
		teensy=csSer.connectComObj(varDict['comPath_teensy'],varDict['baudRate_teensy'])
		[cVal,sChecked]=csSer.checkVariable(teensy,"{}".format(commandHeader),0.005)
		cVal=cVal+delta
		if cVal<=0:
			cVal=1
		comString=commandHeader+str(cVal)+'>'
		teensy.write(comString.encode('utf-8'))
		teensy.close()

	def rampTeensyChan(self,varDict,rampAmp,rampDur,interRamp,rampCount,chanNum,stimType):
		varDelay = 0.01
		totalStimTime=(rampDur*rampCount)+(interRamp*rampCount)
		varDict['comPath_teensy']=self.comPath_teensy_TV.get()
		teensy=csSer.connectComObj(csVar.sesVarDict['comPath_teensy'],csVar.sesVarDict['baudRate_teensy'])
		if chanNum == 1:
			time.sleep(varDelay)
			time.sleep(varDelay)
			teensy.write("g{}>".format(stimType).encode('utf-8'))
			time.sleep(varDelay)
			time.sleep(varDelay)
			teensy.write("f{}>".format(rampAmp).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("e{}>".format(rampDur).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("m{}>".format(rampCount).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("d{}>".format(interRamp).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("j0>".format(interRamp).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("p0>".format(interRamp).encode('utf-8'))
			time.sleep(varDelay)
		elif chanNum == 2:
			time.sleep(varDelay)
			time.sleep(varDelay)
			teensy.write("k{}>".format(stimType).encode('utf-8'))
			time.sleep(varDelay)
			time.sleep(varDelay)
			teensy.write("j{}>".format(rampAmp).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("i{}>".format(rampDur).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("p{}>".format(rampCount).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("h{}>".format(interRamp).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("f0>".format(interRamp).encode('utf-8'))
			time.sleep(varDelay)
			teensy.write("m0>".format(interRamp).encode('utf-8'))
			time.sleep(varDelay)
		[cVal,sChecked]=csSer.checkVariable(teensy,"k",0.005)
		teensy.write("a7>".encode('utf-8'))
		time.sleep(totalStimTime/1000)
		time.sleep(0.2)
		teensy.write("a0>".encode('utf-8'))
		csSer.flushBuffer(teensy)
		time.sleep(varDelay)
		teensy.close()

	def markOffset(self,varDict):
		varDict['comPath_teensy']=self.comPath_teensy_TV.get()
		teensy=csSer.connectComObj(varDict['comPath_teensy'],varDict['baudRate_teensy'])
		wVals=[]
		lIt=0
		while lIt<=50:
			[rV,vN]=csSer.checkVariable(teensy,'w',0.002)
			if vN:
				wVals.append(rV)
				lIt=lIt+1
		varDict['loadBaseline']=np.mean(wVals)
		self.offsetTV.set(float(np.mean(wVals)))
		print(float(np.mean(wVals)))
		teensy.close()
		return varDict

	def makeDevControl(self,varDict):
		dCBWd = 12
		self.deviceControl_frame = Toplevel(self.master)
		self.deviceControl_frame.title('Other Dev Control')

		odStart = 0

		self.weightOffsetBtn = Button(self.deviceControl_frame,text="Offset",width=dCBWd,\
			command=lambda: self.markOffset(varDict))
		self.weightOffsetBtn.grid(row=0,column=2)
		self.weightOffsetBtn['state'] = 'normal'

		self.offsetTV=StringVar(self.deviceControl_frame)
		self.offsetTV.set(varDict['loadBaseline'])
		self.offsetTV_Entry = Entry(self.deviceControl_frame,width=8,textvariable=self.offsetTV)
		self.offsetTV_Entry.grid(row=0,column=3)

		self.testRewardBtn = Button(self.deviceControl_frame,text="Rwd",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"z26>"))
		self.testRewardBtn.grid(row=0,column=0)
		self.testRewardBtn['state'] = 'normal'

		self.triggerBtn = Button(self.deviceControl_frame,text="Trig",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"z25>"))
		self.triggerBtn.grid(row=0,column=1)
		self.triggerBtn['state'] = 'normal'


		# ******* Neopixel Stuff
		self.pDur_label = Label(self.deviceControl_frame,text="Neopixels:",justify=LEFT)
		self.pDur_label.grid(row=1, column=0)

		self.whiteLightBtn = Button(self.deviceControl_frame,text="NP:White",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"n2>"))
		self.whiteLightBtn.grid(row=2,column=0)
		self.whiteLightBtn['state'] = 'normal'

		self.clearLightBtn = Button(self.deviceControl_frame,text="NP:Clear",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"n1>")) 
		self.clearLightBtn.grid(row=2,column=1)
		self.clearLightBtn['state'] = 'normal'

		self.redLightBtn = Button(self.deviceControl_frame,text="NP:Red",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"n3>")) 
		self.redLightBtn.grid(row=3,column=0)
		self.redLightBtn['state'] = 'normal'

		self.greenLightBtn = Button(self.deviceControl_frame,text="NP:Green",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"n4>")) 
		self.greenLightBtn.grid(row=3,column=1)
		self.greenLightBtn['state'] = 'normal'

		self.greenLightBtn = Button(self.deviceControl_frame,text="NP:Blue",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"n5>")) 
		self.greenLightBtn.grid(row=4,column=0)
		self.greenLightBtn['state'] = 'normal'

		self.greenLightBtn = Button(self.deviceControl_frame,text="NP:Purp",width=dCBWd,\
			command=lambda: self.commandTeensy(varDict,"n6>")) 
		self.greenLightBtn.grid(row=4,column=1)
		self.greenLightBtn['state'] = 'normal'

		self.incBrightnessBtn = Button(self.deviceControl_frame,text="NP: +",width=dCBWd,\
			command=lambda: self.deltaTeensy(varDict,'b',10))
		self.incBrightnessBtn.grid(row=5,column=0)
		self.incBrightnessBtn['state'] = 'normal'

		self.decBrightnessBtn = Button(self.deviceControl_frame,text="NP: -",width=dCBWd,\
			command=lambda: self.deltaTeensy(varDict,'b',-10))
		self.decBrightnessBtn.grid(row=5,column=1)
		self.decBrightnessBtn['state'] = 'normal'

	def makePulse(self,varDict):

			self.dC_Pulse = Toplevel(self.master)
			self.dC_Pulse.title('Stim Control')

			self.ramp1DurTV=StringVar(self.dC_Pulse)
			self.ramp1DurTV.set(int(varDict['ramp1Dur']))
			self.ramp1AmpTV=StringVar(self.dC_Pulse)
			self.ramp1AmpTV.set(int(varDict['ramp1Amp']))
			self.ramp2DurTV=StringVar(self.dC_Pulse)
			self.ramp2DurTV.set(int(varDict['ramp2Dur']))
			self.ramp2AmpTV=StringVar(self.dC_Pulse)
			self.ramp2AmpTV.set(int(varDict['ramp1Amp']))

			dCBWd = 12
			
			ptst=0
			self.pDur_label = Label(self.dC_Pulse,text="Dur (ms):",justify=LEFT)
			self.pDur_label.grid(row=0, column=ptst+2)

			self.amplitude_label = Label(self.dC_Pulse,text="Amp:",justify=LEFT)
			self.amplitude_label.grid(row=0, column=ptst+3)

			self.pulseTrainDac1Btn = Button(self.dC_Pulse,text="Pulses DAC1",width=dCBWd,\
				command=lambda: self.rampTeensyChan(varDict,int(self.ramp1AmpTV.get()),int(self.ramp1DurTV.get()),90,10,1,0))
			self.pulseTrainDac1Btn.grid(row=1,column=ptst)
			self.pulseTrainDac1Btn['state'] = 'normal'

			self.rampDAC1Btn = Button(self.dC_Pulse,text="Ramp DAC1",width=dCBWd,\
				command=lambda: self.rampTeensyChan(int(self.ramp1AmpTV.get()),int(self.ramp1DurTV.get()),100,1,1,1)) 
			self.rampDAC1Btn.grid(row=1,column=ptst+1)
			self.rampDAC1Btn['state'] = 'normal'

			
			self.ramp1DurTV_Entry = Entry(self.dC_Pulse,width=8,textvariable=self.ramp1DurTV)
			self.ramp1DurTV_Entry.grid(row=1,column=ptst+2)


			self.ramp1AmpTV_Entry = Entry(self.dC_Pulse,width=8,textvariable=self.ramp1AmpTV)
			self.ramp1AmpTV_Entry.grid(row=1,column=ptst+3)

			self.pulseTrainDac2Btn = Button(dC_Pulse,text="Pulses DAC2",width=dCBWd,\
				command=lambda: self.rampTeensyChan(int(self.ramp2AmpTV.get()),int(self.ramp2DurTV.get()),90,10,2,0)) 
			self.pulseTrainDac2Btn.grid(row=2,column=ptst)
			self.pulseTrainDac2Btn['state'] = 'normal'

			self.rampDAC2Btn = Button(self.dC_Pulse,text="Ramp DAC2",width=dCBWd,\
				command=lambda: rampTeensyChan(int(self.ramp2AmpTV.get()),int(self.ramp2DurTV.get()),100,1,2,1)) 
			self.rampDAC2Btn.grid(row=2,column=ptst+1)
			self.rampDAC2Btn['state'] = 'normal'

			
			self.ramp2DurTV_Entry = Entry(self.dC_Pulse,width=8,textvariable=self.ramp2DurTV)
			self.ramp2DurTV_Entry.grid(row=2,column=ptst+2)

			self.ramp2AmpTV_Entry = Entry(self.dC_Pulse,width=8,textvariable=self.ramp2AmpTV)
			self.ramp2AmpTV_Entry.grid(row=2,column=ptst+3)
class csVariables(object):
	def __init__(self,sesVarDict={},stimVars={}):

		self.sesVarDict={'curSession':1,'comPath_teensy':'/dev/cu.usbmodem4589151',\
		'baudRate_teensy':19200,'subjID':'an1','taskType':'detect','totalTrials':10,\
		'logMQTT':1,'mqttUpDel':0.05,'curWeight':20,'rigGMTZoneDif':5,'volPerRwd':0.01,\
		'waterConsumed':0,'consumpTarg':1.5,'dirPath':'/Users/Deister/BData',\
		'hashPath':'/Users/cad','trialNum':0,'sessionOn':1,'canQuit':1,\
		'contrastChange':0,'orientationChange':1,'spatialChange':1,'dStreams':12,\
		'rewardDur':500,'lickAThr':900,'lickLatchA':0,'minNoLickTime':1000,\
		'toTime':4000,'shapingTrial':1,'chanPlot':5,'minStimTime':1500,\
		'minTrialVar':200,'maxTrialVar':11000,'loadBaseline':0,'loadScale':1,\
		'serBufSize':4096,'ramp1Dur':2000,'ramp1Amp':4095,'ramp2Dur':2000,'ramp2Amp':4095,\
		'detectPlotNum':100}

		self.stimVars={'contrast':1,'sFreq':4,'orientation':0}


	def getRig(self):
		# returns a string that is the hostname
		mchString=socket.gethostname()
		self.hostMachine=mchString.split('.')[0]
		return self.hostMachine
	def dictToPandas(self,dictName):
		curKey=[]
		curVal=[]
		for key in list(dictName.keys()):
			curKey.append(key)
			curVal.append(dictName[key])
			self.pdReturn=pd.Series(curVal,index=curKey)
		return self.pdReturn
	def pandasToDict(self,pdName,curDict,colNum):

		varIt=0
		csvNum=0

		for k in list(pdName.index):

			if len(pdName.shape)>1:
				a=pdName[colNum][varIt]
				csvNum=pdName.shape[1]
			elif len(pdName.shape)==1:
				a=pdName[varIt]

			try:
				a=float(a)
				if a.is_integer():
					a=int(a)
				curDict[k]=a
				varIt=varIt+1

			except:
				curDict[k]=a
				varIt=varIt+1
		
		return curDict
	def updateDictFromGUI(self,dictName):
		for key in list(dictName.keys()):
			try:
				a=eval('{}_TV.get()'.format(key))                
				try:
					a=float(a)
					if a.is_integer():
						a=int(a)
					exec('dictName["{}"]={}'.format(key,a))
				except:
					exec('dictName["{}"]="{}"'.format(key,a))
			except:
				g=1
class csHDF(object):
	def __init__(self,a):
		self.a=1
	
	def makeHDF(self,basePath,subID,dateStamp):
		cStr=datetime.datetime.now().strftime("%Y-%m-%d_%H:%M:%S")
		fe=os.path.isfile(basePath+"{}_behav_{}.hdf".format(subID,cStr))
		if fe:
			os.path.isfile(basePath+"{}_behav_{}.hdf".format(subID,cStr))
			self.sesHDF = h5py.File(basePath+"{}_behav_{}_dup.hdf".format(subID,cStr), "a")
		elif fe==0:
			self.sesHDF = h5py.File(basePath+"{}_behav_{}.hdf".format(subID,cStr), "a")
		return self.sesHDF
class csMQTT(object):
	def __init__(self,dStamp):
		self.dStamp=datetime.datetime.now().strftime("%m_%d_%Y")

	def connectBroker(self,hashPath):
		simpHash=open(hashPath)
		self.aio = Client(simpHash.readline(1),simpHash.readline(2))
		return self.aio

	def getDailyConsumption(self,mqObj,sID,rigGMTDif,hourThresh):
		# Get last reward count logged.
		# assume nothing
		waterConsumed=0
		hourDif=22
		# I assume the mqtt gmt day is the same as our rigs day for now.
		dayOffset=0
		monthOffset=0

		# but grab the last point logged on the MQTT feed.
		gDP=mqObj.receive('{}_waterConsumed'.format(sID))
		# Look at when it was logged.
		crStr=gDP.created_at[0:10]

		rigHr=int(datetime.datetime.fromtimestamp(time.time()).strftime('%H'))
		rigHr=rigHr+rigGMTDif
		# if you offset the rig for GMT and go over 24,
		# that means GMT has crossed the date line. 
		# thus, we need to add a day to our rig's day
		if rigHr>24:
			rigHr=rigHr-24
			dayOffset=1


		# add the GMT diff to the current time in our time zone.
		mqHr=int(gDP.created_at[11:13])

		#compare year (should be a given, but never know)
		if crStr[0:4]==dStamp[6:10]:

			#compare month (less of a given)
			# I allow for a month difference of 1 in case we are on a month boundary before GMT correction.
			if abs(int(crStr[5:7])-int(dStamp[0:2]))<2:
				# todo: add month boundary logic.

				#compare day if there is more than a dif of 1 then can't be 12-23.9 hours dif.
				dayDif=(int(dStamp[3:5])+dayOffset)-int(crStr[8:10])

				if abs(dayDif)<2:
					hourDif=rigHr-mqHr

					if hourDif<=hourThresh:
						waterConsumed=float('{:0.3f}'.format(float(gDP.value)))
		
		
		self.waterConsumed=waterConsumed
		self.hourDif=hourDif
		
		return self.waterConsumed,self.hourDif

	def rigOnLog(self,mqObj,sID,sWeight,hostName,mqDel):
		
		# a) log on to the rig's on-off feed.
		mqObj.send('rig_{}'.format(hostName),1)
		time.sleep(mqDel)

		# b) log the rig string the subject is on to the subject's rig tracking feed.
		mqObj.send('{}_rig'.format(sID),'{}_on'.format(hostName))
		time.sleep(mqDel)

		# c) log the weight to subject's weight tracking feed.
		mqObj.send('{}_weight'.format(sID,sWeight),sWeight)

	# def updateTrial(self,mqObj,sID,sWeight,hostName):
		
	#     mqObj.send('{}_trial'.format(sID),'{}_{}'.format(,'h'))


	def rigOffLog(self,mqObj,sID,sWeight,hostName,mqDel):
		
		# a) log off to the rig's on-off feed.
		mqObj.send('rig_{}'.format(hostName),0)
		time.sleep(mqDel)

		# b) log the rig string the subject is on to the subject's rig tracking feed.
		mqObj.send('{}_rig'.format(sID),'{}_off'.format(hostName))
		time.sleep(mqDel)

		# c) log the weight to subject's weight tracking feed.
		mqObj.send('{}_weight'.format(sID,sWeight),sWeight)

	def openGoogleSheet(self,gAPIHashPath):
		#gAPIHashPath='/Users/cad/simpHashes/client_secret.json'
		self.gc = pygsheets.authorize(gAPIHashPath)
		return self.gc
	
	def updateGoogleSheet(self,sheetCon,subID,cellID,valUp):
		sh = sheetCon.open('WR Log')
		wsTup=sh.worksheets()
		wks = sh.worksheet_by_title(subID)
		curData=np.asarray(wks.get_all_values(returnas='matrix'))
		dd=np.where(curData==cellID)
		# Assuming indexes are in row 1, then I just care about dd[1]
		varCol=dd[1]+1
		# now let's figure out which row to update
		entries=curData[:,dd[1]]
		# how many entries exist in that column
		numRows=len(entries)
		lastEntry=curData[numRows-1,dd[1]]
		if lastEntry=='':
			wks.update_cell((numRows,varCol),valUp)
			self.updatedRow=numRows
			self.updatedCol=varCol
		elif lastEntry != '':
			wks.update_cell((numRows+1,varCol),valUp)
			self.updatedRow=numRows
			self.updatedCol=varCol
		return 
class csSerial(object):
	
	def __init__(self,a):
		
		self.a=1

	def connectComObj(self,comPath,baudRate):
		self.comObj = serial.Serial(comPath,baudRate,timeout=0)
		self.comObj.close()
		self.comObj.open()
		return self.comObj

	def readSerialBuffer(self,comObj,curBuf,bufMaxSize):
		
		comObj.timeOut=0
		curBuf=curBuf+comObj.read(1)
		curBuf=curBuf+comObj.read(min(bufMaxSize-1, comObj.in_waiting))
		i = curBuf.find(b"\n")
		r = curBuf[:i+1]
		curBuf = curBuf[i+1:]
		
		echoDecode=bytes()
		tDataDecode=bytes()
		eR=[]
		sR=[]

		eB=r.find(b"echo")
		eE=r.find(b"~")
		tDB=r.find(b"tData")
		tDE=r.find(b"\r")

		if eB>=0 and eE>=1:
			echoDecode=r[eB:eE+1]
			eR=echoDecode.strip().decode()
			eR=eR.split(',')
		if tDB>=0 and tDE>=1:
			tDataDecode=r[tDB:tDE]
			sR=tDataDecode.strip().decode()
			sR=sR.split(',')

		self.curBuf = curBuf
		self.echoLine = eR
		self.dataLine = sR
		return self.curBuf,self.echoLine,self.dataLine

	def readSerialData(self,comObj,headerString,varCount):
		sR=[]
		newData=0
		bytesAvail=comObj.inWaiting()

		if bytesAvail>0:
			sR=comObj.readline().strip().decode()
			sR=sR.split(',')
			if len(sR)==varCount and sR[0]==headerString:
				newData=1

		self.sR=sR
		self.newData=newData
		self.bytesAvail = bytesAvail
		return self.sR,self.newData,self.bytesAvail

	def flushBuffer(self,comObj):
		while comObj.inWaiting()>0:
			sR=comObj.readline().strip().decode()
			sR=[]

	def checkVariable(self,comObj,headChar,fltDelay):
		comObj.write('{}<'.format(headChar).encode('utf-8'))
		time.sleep(fltDelay)
		[tString,self.dNew,self.bAvail]=self.readSerialData(comObj,'echo',4)
		if self.dNew:
			if tString[1]==headChar:
				self.returnVar=int(tString[2])
		elif self.dNew==0:
			self.returnVar=0
		return self.returnVar,self.dNew
class csPlot(object):
	
	def __init__(self,stPlotX={},stPlotY={},stPlotRel={},pClrs={},pltX=[],pltY=[]):
		
		# initial y bounds for oscope plot

		#start state
		self.stPlotX={'init':0.10,'wait':0.10,'stim':0.30,'catch':0.30,'rwd':0.50,'TO':0.50}
		self.stPlotY={'init':0.65,'wait':0.40,'stim':0.52,'catch':0.28,'rwd':0.52,'TO':0.28}
		# # todo:link actual state dict to plot state dict, now its a hack
		self.stPlotRel={'0':0,'1':1,'2':2,'3':3,'4':4,'5':5}
		self.pClrs={'right':'#D9220D','cBlue':'#33A4F3','cPurp':'#6515D9',\
		'cOrange':'#F7961D','left':'cornflowerblue','cGreen':'#29AA03'}

		self.pltX=[]
		for xVals in list(self.stPlotX.values()):
			self.pltX.append(xVals)
		self.pltY=[]
		for yVals in list(self.stPlotY.values()):
			self.pltY.append(yVals)

	def makeTrialFig(self,fNum):
		self.binDP=[]
		# Make feedback figure.
		self.trialFig = plt.figure(fNum)
		self.trialFig.suptitle('trial # 0 of  ; State # ',fontsize=10)
		self.trialFramePosition='+250+0' # can be specified elsewhere
		mng = plt.get_current_fig_manager()
		eval('mng.window.wm_geometry("{}")'.format(self.trialFramePosition))
		plt.show(block=False)
		self.trialFig.canvas.flush_events()

		# add the lickA axes and lines.
		self.lA_Axes=self.trialFig.add_subplot(2,2,1) #col,rows
		self.lA_Axes.set_ylim([-100,1200])
		self.lA_Axes.set_xticks([])
		# self.lA_Axes.set_yticks([])
		self.lA_Line,=self.lA_Axes.plot([],color="cornflowerblue",lw=1)
		self.trialFig.canvas.draw_idle()
		plt.show(block=False)
		self.trialFig.canvas.flush_events()
		self.lA_Axes.draw_artist(self.lA_Line)
		self.lA_Axes.draw_artist(self.lA_Axes.patch)
		self.trialFig.canvas.flush_events()

		# STATE AXES
		self.stAxes = self.trialFig.add_subplot(2,2,2) #col,rows
		self.stAxes.set_ylim([-0.02,1.02])
		self.stAxes.set_xlim([-0.02,1.02])
		self.stAxes.set_axis_off()
		self.stMrkSz=28
		self.txtOff=-0.02
		self.stPLine,=self.stAxes.plot(self.pltX,self.pltY,marker='o',\
			markersize=self.stMrkSz,markeredgewidth=2,\
			markerfacecolor="white",markeredgecolor="black",lw=0)
		k=0
		for stAnTxt in list(self.stPlotX.keys()):
			tASt="{}".format(stAnTxt)
			self.stAxes.text(self.pltX[k],self.pltY[k]+self.txtOff,tASt,\
				horizontalalignment='center',fontsize=9,fontdict={'family': 'monospace'})
			k=k+1

		self.curStLine,=self.stAxes.plot(self.pltX[1],self.pltY[1],marker='o',markersize=self.stMrkSz+1,\
			markeredgewidth=2,markerfacecolor=self.pClrs['cBlue'],markeredgecolor='black',lw=0,alpha=0.5)
		plt.show(block=False)
		
		self.trialFig.canvas.flush_events()
		self.stAxes.draw_artist(self.stPLine)
		self.stAxes.draw_artist(self.curStLine)
		self.stAxes.draw_artist(self.stAxes.patch)

		# OUTCOME AXES
		self.outcomeAxis=self.trialFig.add_subplot(2,2,3) #col,rows
		self.outcomeAxis.axis([-2,100,-0.2,5.2])
		self.outcomeAxis.yaxis.tick_left()

		self.stimOutcomeLine,=self.outcomeAxis.plot([],[],marker="o",markeredgecolor="black",\
			markerfacecolor="cornflowerblue",markersize=12,lw=0,alpha=0.5,markeredgewidth=2)
		
		self.noStimOutcomeLine,=self.outcomeAxis.plot([],[],marker="o",markeredgecolor="black",\
			markerfacecolor="red",markersize=12,lw=0,alpha=0.5,markeredgewidth=2)
		self.outcomeAxis.set_title('dprime: ',fontsize=10)
		self.binDPOutcomeLine,=self.outcomeAxis.plot([],[],color="black",lw=1)
		plt.show(block=False)
		self.trialFig.canvas.flush_events()
		self.outcomeAxis.draw_artist(self.stimOutcomeLine)
		self.outcomeAxis.draw_artist(self.noStimOutcomeLine)
		self.outcomeAxis.draw_artist(self.binDPOutcomeLine)
		self.outcomeAxis.draw_artist(self.outcomeAxis.patch)

	def quickUpdateTrialFig(self,trialNum,totalTrials,curState):
		self.trialFig.suptitle('trial # {} of {}; State # {}'.format(trialNum,totalTrials,curState),fontsize=10)
		self.trialFig.canvas.flush_events()

	def updateTrialFig(self,xData,yData,trialNum,totalTrials,curState,yLims):
		try:
			self.trialFig.suptitle('trial # {} of {}; State # {}'.format(trialNum,totalTrials,curState),fontsize=10)
			self.lA_Line.set_xdata(xData)
			self.lA_Line.set_ydata(yData)
			self.lA_Axes.set_xlim([xData[0],xData[-1]])
			self.lA_Axes.set_ylim([yLims[0],yLims[1]])
			self.lA_Axes.draw_artist(self.lA_Line)
			self.lA_Axes.draw_artist(self.lA_Axes.patch)
			

			self.trialFig.canvas.draw_idle()
			self.trialFig.canvas.flush_events()

		except:
			 a=1
	
	def updateStateFig(self,curState):
		try:
			self.curStLine.set_xdata(self.pltX[curState])
			self.curStLine.set_ydata(self.pltY[curState])
			self.stAxes.draw_artist(self.stPLine)
			self.stAxes.draw_artist(self.curStLine)
			self.stAxes.draw_artist(self.stAxes.patch)

			self.trialFig.canvas.draw_idle()
			self.trialFig.canvas.flush_events()

		except:
			 a=1

	def updateOutcome(self,stimTrials,stimResponses,noStimTrials,noStimResponses,totalTrials):
		sM=0.001
		nsM=0.001
		dpBinSz=10

		if len(stimResponses)>0:
			sM=np.mean(stimResponses)
		if len(noStimResponses)>0:
			nsM=np.mean(noStimResponses)
		


		dpEst=norm.ppf(max(sM,0.0001))-norm.ppf(max(nsM,0.0001))
		# self.outcomeAxis.set_title('CR: {} , FR: {}'.format(sM,nsM),fontsize=10)
		self.outcomeAxis.set_title('dprime: {:0.3f}'.format(dpEst),fontsize=10)
		self.stimOutcomeLine.set_xdata(stimTrials)
		self.stimOutcomeLine.set_ydata(stimResponses)
		self.noStimOutcomeLine.set_xdata(noStimTrials)
		self.noStimOutcomeLine.set_ydata(noStimResponses)
		
		if len(noStimResponses)>0 and len(stimResponses)>0:
			sMb=int(np.mean(stimResponses[-dpBinSz:])*100)*0.01
			nsMb=int(np.mean(noStimResponses[-dpBinSz:])*100)*0.01
			self.binDP.append(norm.ppf(max(sMb,0.0001))-norm.ppf(max(nsMb,0.0001)))
			self.binDPOutcomeLine.set_xdata(np.linspace(1,len(self.binDP),len(self.binDP)))
			self.binDPOutcomeLine.set_ydata(self.binDP)
			self.outcomeAxis.draw_artist(self.binDPOutcomeLine)
		
		self.outcomeAxis.set_xlim([-1,totalTrials+1])
		self.outcomeAxis.draw_artist(self.stimOutcomeLine)
		self.outcomeAxis.draw_artist(self.noStimOutcomeLine)
		self.outcomeAxis.draw_artist(self.outcomeAxis.patch)

		self.trialFig.canvas.draw_idle()
		self.trialFig.canvas.flush_events()

# $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
# $$$$$$$$$$$$$ Main Program Body $$$$$$$$$$$$$$$$
# $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

# initialize class instances and some flags.
csVar=csVariables(1)
csSesHDF=csHDF(1)
csAIO=csMQTT(1)
csSer=csSerial(1)
csPlt=csPlot(1)
csGui = csGUI(root,csVar.sesVarDict)


# create a matplotlib figure for trial feedback etc.
# csPlt.makeTrialFig(100)

# datestamp/rig id/session variables
cTime = datetime.datetime.now()
dStamp=cTime.strftime("%m_%d_%Y")
curMachine=csVar.getRig()

# ****************************
# ***** trial data logging ***
# ****************************

# pre-alloc lists for variables that only change across trials.

contrastList=[]
orientationList=[]
spatialFreqs=[]
waitPad=[]



def runDetectionTask():

	csPlt.makeTrialFig(csVar.sesVarDict['detectPlotNum'])
	# A) Update the dict from gui, in case the user changed things.

	csVar.sesVarDict=csGui.updateDictFromGUI(csVar.sesVarDict)
	print(csVar.sesVarDict['logMQTT'])
	

	# C) Create a com object to talk to the main Teensy. 
	# todo: create a timeout
	teensy=csSer.connectComObj(csVar.sesVarDict['comPath_teensy'],csVar.sesVarDict['baudRate_teensy'])
	
	# D) Task specific: preallocate sensory variables that need randomization.
	# prealloc random stuff (assume no more than 1k trials)
	maxTrials=1000
	csVar.sesVarDict['contrastChange']=1
	if csVar.sesVarDict['contrastChange']:
		contList=np.array([0,0,0,1,2,5,10,20,40,50,70,90,100,100])
		randContrasts=contList[np.random.randint(0,len(contList),size=maxTrials)]
	elif csVar.sesVarDict['contrastChange']==0:
		defaultContrast=100
		randContrasts=defaultContrast*np.ones(maxTrials)
		teensy.write('c{}>'.format(defaultContrast).encode('utf-8'))
	csVar.sesVarDict['orientationChange']=1
	if csVar.sesVarDict['orientationChange']:
		orientList=np.array([90,0,270])
		# orientList=np.array([0,45,90,120,180,225,270,315])
		randOrientations=orientList[np.random.randint(0,len(orientList),size=maxTrials)]
	elif csVar.sesVarDict['orientationChange']==0:
		defaultOrientation=0
		randOrientations=defaultOrientation*np.ones(maxTrials)
		teensy.write('o{}>'.format(defaultOrientation).encode('utf-8'))
	randSpatialContinuous=0
	csVar.sesVarDict['spatialChange']=1
	if csVar.sesVarDict['spatialChange']:
		randSpatials=np.random.randint(0,5,size=maxTrials)
	elif csVar.sesVarDict['spatialChange']==0:
		defaultSpatial=1
		randSpatials=defaultSpatial*np.ones(maxTrials)
		teensy.write('s{}>'.format(defaultSpatial).encode('utf-8'))

	randWaitTimePad=np.random.randint(csVar.sesVarDict['minTrialVar'],csVar.sesVarDict['maxTrialVar'],size=maxTrials)

	# D) Flush the teensy serial buffer. Send it to the init state (#0).
	csSer.flushBuffer(teensy)
	teensy.write('a0>'.encode('utf-8'))
	time.sleep(0.01)

	# E) Make sure the main Teensy is actually in state 0.
	# check the state.
	sChecked=0
	while sChecked==0:
		[tTeensyState,sChecked]=csSer.checkVariable(teensy,'a',0.005)

	while tTeensyState != 0:
		print("not in 0, will force")
		teensy.write('a0>'.encode('utf-8'))
		time.sleep(0.005)
		cReturn=csSer.checkVariable(teensy,'a',0.005)
		if cReturn(1)==1:
			tTeensyState=cReturn(0)
	
	# F) Get lick sensor and load cell baseline. Estimate and log weight.
	try:
		wVals=[]
		lIt=0
		while lIt<=50:
			[rV,vN]=csSer.checkVariable(teensy,'w',0.002)
			if vN:
				wVals.append(rV)
				lIt=lIt+1
		csVar.sesVarDict['curWeight']=(np.mean(wVals)-csVar.sesVarDict['loadBaseline'])*0.1;
		preWeight=csVar.sesVarDict['curWeight']
		print("pre weight={}".format(preWeight))
	except:
		csVar.sesVarDict['curWeight']=20

	# Optional: Update MQTT Feeds
	csVar.sesVarDict['logMQTT']=1
	if csVar.sesVarDict['logMQTT']==1:

		aioHashPath=csVar.sesVarDict['hashPath'] + '/simpHashes/cdIO.txt'
		# aio is csAIO's mq broker object.
		aio=csAIO.connectBroker(aioHashPath)
		try:
			csAIO.rigOnLog(aio,csVar.sesVarDict['subjID'],csVar.sesVarDict['curWeight'],curMachine,csVar.sesVarDict['mqttUpDel'])
		except:
			print('no mqtt logging')

		try:
			print('logging to sheet')
			gHashPath=csVar.sesVarDict['hashPath'] + '/simpHashes/client_secret.json'
			gSheet=csAIO.openGoogleSheet(gHashPath)
			csAIO.updateGoogleSheet(gSheet,csVar.sesVarDict['subjID'],'Weight Pre',csVar.sesVarDict['curWeight'])
			print('logged to sheet')
		except:
			print('did not log to google sheet')


	# F) Set some session flow variables before the task begins
	# Turn the session on. 
	csVar.sesVarDict['sessionOn']=1
	# Set the state of the quit button to 0, it now ends the session.
	# This is part of the scheme to ensure data always gets saved. 
	csVar.sesVarDict['canQuit']=0
	# quitButton['text']="End Ses"
	# For reference, confirm the Teensy's interrupt (sampling) rate. 
	csVar.sesVarDict['sampRate']=1000
	# Set maxDur to be two hours.
	csVar.sesVarDict['maxDur']=3600*2*csVar.sesVarDict['sampRate']
	# Determine the max samples. We preallocate a numpy array to this depth.
	npSamps=csVar.sesVarDict['maxDur']
	sesData = np.memmap('cur.npy', mode='w+',dtype=np.int32,shape=(npSamps,csVar.sesVarDict['dStreams']))
	np.save('sesData.npy',sesData)

	dStreamLables=['interrupt','trialTime','stateTime','teensyState','lick0_Data',\
	'motion','scopeState','aOut1','aOut2','pythonState','thrLicksA']

	# Temp Trial Variability
	f=csSesHDF.makeHDF(csVar.sesVarDict['dirPath']+'/',csVar.sesVarDict['subjID'] + '_ses{}'.\
		format(csVar.sesVarDict['curSession']),dStamp)

	pyState=1
	lastLick=0
	lickCounter=0
	
	tContrast=0
	tOrientation=0

	sHeaders=np.array([0,0,0,0,0,0])
	sList=[0,1,2,3,4,5]
	trialSamps=[0,0]
	
	serialBuf=bytearray()
	sampLog=[]
	
	csGui.tc['state'] = 'disabled'
	stimResponses=[]
	stimTrials=[]
	noStimResponses=[]
	noStimTrials=[]
	loopCnt=0
	csVar.sesVarDict['trialNum']=0
	csVar.sesVarDict['lickLatchA']=0
	outSyncCount=0

	# Send to 1, wait state.
	teensy.write('a1>'.encode('utf-8')) 
	while csVar.sesVarDict['sessionOn']:
		# try to execute the task.
		try:
			
			# # a) Do we keep running?
			csVar.sesVarDict['totalTrials']=int(csGui.totalTrials_TV.get())
			try:
				csVar.sesVarDict['shapingTrial']=int(csGui.shapingTrial_TV.get())
			except:
				csVar.sesVarDict['shapingTrial']=0
				shapingTrial_TV.set('0')
			csVar.sesVarDict['lickAThr']=int(csGui.lickAThr_TV.get())
			csVar.sesVarDict['chanPlot']=csGui.chanPlotIV.get()
			csVar.sesVarDict['minStimTime']=int(csGui.minStimTime_TV.get())
			if csVar.sesVarDict['trialNum']>csVar.sesVarDict['totalTrials']:
				csVar.sesVarDict['sessionOn']=0

			
			# b) Look for teensy data.
			[serialBuf,eR,tString]=csSer.readSerialBuffer(teensy,serialBuf,csVar.sesVarDict['serBufSize'])
			if len(tString)==csVar.sesVarDict['dStreams']-1:

				intNum=int(tString[1])
				tStateTime=int(tString[3])
				tTeensyState=int(tString[4])
		

				tFrameCount=0  # Todo: frame counter in.
				for x in range(0,csVar.sesVarDict['dStreams']-2):
					sesData[intNum,x]=int(tString[x+1])
				sesData[intNum,csVar.sesVarDict['dStreams']-2]=pyState # The state python wants to be.
				sesData[intNum,csVar.sesVarDict['dStreams']-1]=0 # Thresholded licks
				loopCnt=loopCnt+1
				
				# Plot updates.
				plotSamps=200
				updateCount=500
				lyMin=-1
				lyMax=1025
				if csVar.sesVarDict['chanPlot']==9 or csVar.sesVarDict['chanPlot']==7:
					lyMin=-0.1
					lyMax=1.1
				if loopCnt>plotSamps and np.mod(loopCnt,updateCount)==0:
					if csVar.sesVarDict['chanPlot']==0:
						csPlt.quickUpdateTrialFig(csVar.sesVarDict['trialNum'],\
							csVar.sesVarDict['totalTrials'],tTeensyState)
					elif csVar.sesVarDict['chanPlot'] != 0:
						csPlt.updateTrialFig(np.arange(len(sesData[loopCnt-plotSamps:loopCnt,csVar.sesVarDict['chanPlot']])),\
							sesData[loopCnt-plotSamps:loopCnt,csVar.sesVarDict['chanPlot']],csVar.sesVarDict['trialNum'],\
							csVar.sesVarDict['totalTrials'],tTeensyState,[lyMin,lyMax])


				# look for licks
				latchTime=50
				if sesData[loopCnt-1,5]>=csVar.sesVarDict['lickAThr'] and csVar.sesVarDict['lickLatchA']==0:
					sesData[loopCnt-1,csVar.sesVarDict['dStreams']-1]=1
					csVar.sesVarDict['lickLatchA']=latchTime
					# these are used in states
					lickCounter=lickCounter+1
					lastLick=tStateTime

				elif csVar.sesVarDict['lickLatchA']>0:
					csVar.sesVarDict['lickLatchA']=csVar.sesVarDict['lickLatchA']-1

				# 2) Does pyState match tState?
				if pyState == tTeensyState:
					stateSync=1

				elif pyState != tTeensyState:
					stateSync=0
				
				
				# If we are out of sync for too long, push another change.
				if stateSync==0:
					outSyncCount=outSyncCount+1
					if outSyncCount>=100:
						teensy.write('a{}>'.format(pyState).encode('utf-8'))  

				# 4) Now look at what state you are in and evaluate accordingly
				if pyState == 1 and stateSync==1:
					
					if sHeaders[pyState]==0:
						csVar.sesVarDict['trialNum']=csVar.sesVarDict['trialNum']+1
						csVar.sesVarDict['minNoLickTime']=np.random.randint(900,2900)
						csPlt.updateStateFig(1)
						trialSamps[0]=loopCnt-1

						# reset counters that track state stuff.
						lickCounter=0
						lastLick=0                    
						outSyncCount=0
						# get contrast and orientation
						# trials are 0 until incremented, so incrementing
						# trial after these picks ensures 0 indexing without -1.
						tContrast=randContrasts[csVar.sesVarDict['trialNum']]
						tOrientation=randOrientations[csVar.sesVarDict['trialNum']]
						tSpatial=randSpatials[csVar.sesVarDict['trialNum']]
						preTime=randWaitTimePad[csVar.sesVarDict['trialNum']]

						contrastList.append(tContrast)
						orientationList.append(tOrientation)
						spatialFreqs.append(tSpatial)
						waitPad.append(preTime)

						# update visual stim params
						teensy.write('c{}>'.format(tContrast).encode('utf-8'))
						teensy.write('o{}>'.format(tOrientation).encode('utf-8'))
						teensy.write('s{}>'.format(tSpatial).encode('utf-8'))
					   
						# update the trial
						print('start trial #{}'.format(csVar.sesVarDict['trialNum']))
						print('contrast: {:0.2f} orientation: {}'.format(tContrast,tOrientation))

						# close the header and flip the others open.
						sHeaders[pyState]=1
						sHeaders[np.setdiff1d(sList,pyState)]=0
					
					# exit if we've waited long enough (preTime) and animal isn't licking.
					if (tStateTime-lastLick)>csVar.sesVarDict['minNoLickTime'] and tStateTime>preTime:
						# we know we will be out of sync
						stateSync=0
						# we set python to new state.
						if tContrast>0:
							pyState=2
							# we ask teensy to go to new state.
							teensy.write('a2>'.encode('utf-8'))
						elif tContrast==0:
							pyState=3
							# we ask teensy to go to new state.
							teensy.write('a3>'.encode('utf-8'))
				if pyState == 2 and stateSync==1:
					if sHeaders[pyState]==0:
						csPlt.updateStateFig(pyState)
						reported=0
						lickCounter=0
						lastLick=0
						outSyncCount=0
						sHeaders[pyState]=1
						sHeaders[np.setdiff1d(sList,pyState)]=0                        
	 
					if lastLick>0.02:
						reported=1

					if tStateTime>csVar.sesVarDict['minStimTime']:
						if reported==1 or csVar.sesVarDict['shapingTrial']:
							stimTrials.append(csVar.sesVarDict['trialNum'])
							stimResponses.append(1)
							stateSync=0
							pyState=4
							teensy.write('a4>'.encode('utf-8'))
							csPlt.updateOutcome(stimTrials,stimResponses,noStimTrials,noStimResponses,\
								csVar.sesVarDict['totalTrials'])
						elif reported==0:
							stimTrials.append(csVar.sesVarDict['trialNum'])
							stimResponses.append(0)
							stateSync=0
							pyState=1
							trialSamps[1]=loopCnt
							sampLog.append(np.diff(trialSamps)[0])
							teensy.write('a1>'.encode('utf-8'))
							csPlt.updateOutcome(stimTrials,stimResponses,noStimTrials,noStimResponses,\
								csVar.sesVarDict['totalTrials'])
							print('miss: last trial took: {} seconds'.format(sampLog[-1]/1000))

				
				if pyState == 3 and stateSync==1:
					if sHeaders[pyState]==0:
						csPlt.updateStateFig(pyState)
						reported=0
						lickCounter=0
						lastLick=0
						outSyncCount=0
						sHeaders[pyState]=1
						sHeaders[np.setdiff1d(sList,pyState)]=0
	 
					if lastLick>0.005:
						reported=1

					if tStateTime>csVar.sesVarDict['minStimTime']:
						if reported==1:
							noStimTrials.append(csVar.sesVarDict['trialNum'])
							noStimResponses.append(1)
							# aio.send('{}_trial'.format(csVar.sesVarDict['subjID']),3)
							stateSync=0
							pyState=5
							teensy.write('a5>'.encode('utf-8'))
							csPlt.updateOutcome(stimTrials,stimResponses,noStimTrials,noStimResponses,\
								csVar.sesVarDict['totalTrials'])
						elif reported==0:
							noStimTrials.append(csVar.sesVarDict['trialNum'])
							noStimResponses.append(0)
							# aio.send('{}_trial'.format(csVar.sesVarDict['subjID']),4)
							stateSync=0
							pyState=1
							trialSamps[1]=loopCnt
							sampLog.append(np.diff(trialSamps)[0])
							teensy.write('a1>'.encode('utf-8'))
							csPlt.updateOutcome(stimTrials,stimResponses,noStimTrials,noStimResponses,\
								csVar.sesVarDict['totalTrials'])
							print('cor rejection: last trial took: {} seconds'.format(sampLog[-1]/1000))

				if pyState == 4 and stateSync==1:
					if sHeaders[pyState]==0:
						csPlt.updateStateFig(pyState)
						lickCounter=0
						lastLick=0
						outSyncCount=0
						csVar.sesVarDict['waterConsumed']=csVar.sesVarDict['waterConsumed']+csVar.sesVarDict['volPerRwd']
						sHeaders[pyState]=1
						sHeaders[np.setdiff1d(sList,pyState)]=0
					
					# exit
					if tStateTime>csVar.sesVarDict['rewardDur']:
						trialSamps[1]=loopCnt
						sampLog.append(np.diff(trialSamps)[0])
						stateSync=0
						pyState=1
						outSyncCount=0
						teensy.write('a1>'.encode('utf-8'))
						print('last trial took: {} seconds'.format(sampLog[-1]/1000))

				if pyState == 5 and stateSync==1:
					if sHeaders[pyState]==0:
						csPlt.updateStateFig(pyState)
						lickCounter=0
						lastLick=0
						outSyncCount=0
						sHeaders[pyState]=1
						sHeaders[np.setdiff1d(sList,pyState)]=0
					
					# exit
					if tStateTime>csVar.sesVarDict['toTime']:
						trialSamps[1]=loopCnt
						sampLog.append(np.diff(trialSamps)[0])
						stateSync=0
						pyState=1
						teensy.write('a1>'.encode('utf-8'))
						print('last trial took: {} seconds'.format(sampLog[-1]/1000))
		except:
			sesData.flush()
			np.save('sesData.npy',sesData)
			print(x)
			print(loopCnt)
			print(tString)
			sesData[intNum,x]=int(tString[x+1])
			csGui.tc['state'] = 'normal'
			csVar.sesVarDict['curSession']=csVar.sesVarDict['curSession']+1
			teensy.write('a0>'.encode('utf-8'))
			time.sleep(0.05)
			teensy.write('a0>'.encode('utf-8'))

			print('finished {} trials'.format(csVar.sesVarDict['trialNum']-1))
			csVar.sesVarDict['trialNum']=0
			csVar.updateDictFromGUI(csVar.sesVarDict)
			csVar.sesVarDict_bindings=csVar.dictToPandas(csVar.sesVarDict)
			csVar.sesVarDict_bindings.to_csv(csVar.sesVarDict['dirPath'] + '/' +'csVar.sesVarDict.csv')
			print(stimResponses)
			f["session_{}".format(csVar.sesVarDict['curSession']-1)]=sesData[0:loopCnt,:]
			f["session_{}".format(csVar.sesVarDict['curSession']-1)].attrs['contrasts']=contrastList
			f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['stimResponses']=stimResponses
			f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['stimTrials']=stimTrials
			f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['noStimResponses']=noStimResponses
			f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['noStimTrials']=noStimTrials
			f["session_{}".format(csVar.sesVarDict['curSession']-1)].attrs['orientations']=orientationList
			f["session_{}".format(csVar.sesVarDict['curSession']-1)].attrs['spatialFreqs']=spatialFreqs
			f["session_{}".format(csVar.sesVarDict['curSession']-1)].attrs['waitTimePads']=waitPad
			f["session_{}".format(csVar.sesVarDict['curSession']-1)].attrs['trialDurs']=sampLog
			f.close()

			

			# Update MQTT Feeds
			if csVar.sesVarDict['logMQTT']==1:
				try:
					csVar.sesVarDict['curWeight']=(np.mean(sesData[-1000:-1,4])-csVar.sesVarDict['loadBaseline'])*0.1
					csAIO.rigOffLog(aio,csVar.sesVarDict['subjID'],csVar.sesVarDict['curWeight'],curMachine,csVar.sesVarDict['mqttUpDel'])

					# update animal's water consumed feed.
					csVar.sesVarDict['waterConsumed']=int(csVar.sesVarDict['waterConsumed']*10000)/10000
					aio.send('{}_waterConsumed'.format(csVar.sesVarDict['subjID']),csVar.sesVarDict['waterConsumed'])
					topAmount=csVar.sesVarDict['consumpTarg']-csVar.sesVarDict['waterConsumed']
					topAmount=int(topAmount*10000)/10000
					if topAmount<0:
						topAmount=0
				 
					print('give {:0.3f} ml later by 12 hrs from now'.format(topAmount))
					aio.send('{}_topVol'.format(csVar.sesVarDict['subjID']),topAmount)
				except:
					a=1
			
			csVar.updateDictFromGUI(csVar.sesVarDict)
			csVar.sesVarDict_bindings=csVar.dictToPandas(csVar.sesVarDict)
			csVar.sesVarDict_bindings.to_csv(csVar.sesVarDict['dirPath'] + '/' +'csVar.sesVarDict.csv')

			csSer.flushBuffer(teensy) 
			teensy.close()
			csVar.sesVarDict['canQuit']=1
			quitButton['text']="Quit"
						 
	
	f["session_{}".format(csVar.sesVarDict['curSession'])]=sesData[0:loopCnt,:]
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['contrasts']=contrastList
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['stimResponses']=stimResponses
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['stimTrials']=stimTrials
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['noStimResponses']=noStimResponses
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['noStimTrials']=noStimTrials
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['orientations']=orientationList
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['spatialFreqs']=spatialFreqs
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['waitTimePads']=waitPad
	f["session_{}".format(csVar.sesVarDict['curSession'])].attrs['trialDurs']=sampLog

	
	f.close()

	csGui.tc['state'] = 'normal'
	csVar.sesVarDict['curSession']=csVar.sesVarDict['curSession']+1
	# curSession_TV.set(csVar.sesVarDict['curSession'])
	teensy.write('a0>'.encode('utf-8'))
	time.sleep(0.05)
	teensy.write('a0>'.encode('utf-8'))

	print('finished {} trials'.format(csVar.sesVarDict['trialNum']-1))
	csVar.sesVarDict['trialNum']=0



	# Update MQTT Feeds
	if csVar.sesVarDict['logMQTT']==1:
		try:
			csVar.sesVarDict['curWeight']=(np.mean(sesData[loopCnt-plotSamps:loopCnt,4])-csVar.sesVarDict['loadBaseline'])*0.1
			csVar.sesVarDict['waterConsumed']=int(csVar.sesVarDict['waterConsumed']*10000)/10000
			topAmount=csVar.sesVarDict['consumpTarg']-csVar.sesVarDict['waterConsumed']
			topAmount=int(topAmount*10000)/10000
			if topAmount<0:
				topAmount=0
			print('give {:0.3f} ml later by 12 hrs from now'.format(topAmount))

			try:
				csAIO.rigOffLog(aio,csVar.sesVarDict['subjID'],csVar.sesVarDict['curWeight'],curMachine,csVar.sesVarDict['mqttUpDel'])
				aio.send('{}_waterConsumed'.format(csVar.sesVarDict['subjID']),csVar.sesVarDict['waterConsumed'])
				aio.send('{}_topVol'.format(csVar.sesVarDict['subjID']),topAmount)
			except:
				print('failed to log mqtt info')
	   
			# update animal's water consumed feed.

			try:
				gDStamp=datetime.datetime.now().strftime("%m/%d/%Y")
				gTStamp=datetime.datetime.now().strftime("%H:%M:%S")
			except:
				print('did not log to google sheet')
		
			try:
				print('attempting to log to sheet')
				gSheet=csAIO.openGoogleSheet(gHashPath)
				canLog=1
			except:
				print('failed to open google sheet')
				canLog=0
		
			if canLog==1:
				try:
					csAIO.updateGoogleSheet(gSheet,csVar.sesVarDict['subjID'],'Weight Post',csVar.sesVarDict['curWeight'])
					csAIO.updateGoogleSheet(gSheet,csVar.sesVarDict['subjID'],'Delivered',csVar.sesVarDict['waterConsumed'])
					csAIO.updateGoogleSheet(gSheet,csVar.sesVarDict['subjID'],'Place',curMachine)
					csAIO.updateGoogleSheet(gSheet,csVar.sesVarDict['subjID'],'Date Stamp',gDStamp)
					csAIO.updateGoogleSheet(gSheet,csVar.sesVarDict['subjID'],'Time Stamp',gTStamp)
				except:
					print('did not log some things')
		except:
			print("failed to log")

	print('finished your session')
	csVar.updateDictFromGUI(csVar.sesVarDict)
	csVar.sesVarDict_bindings=csVar.dictToPandas(csVar.sesVarDict)
	csVar.sesVarDict_bindings.to_csv(csVar.sesVarDict['dirPath'] + '/' +'csVar.sesVarDict.csv')

	csSer.flushBuffer(teensy)
	teensy.close()
	csVar.sesVarDict['canQuit']=1
	csGui.quitButton['text']="Quit"





# makeTaskGUI()
# makePulse()
# makeDevControl()
mainloop()


