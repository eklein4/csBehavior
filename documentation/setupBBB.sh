#!/bin/bash

sudo apt-get update
sudo apt-get upgrade -y

sudo apt-get install curl -y
sudo apt-get remove python-pip -y
sudo apt-get remove python3-pip -y
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py

sudo apt-get remove python-pip -y
sudo python get-pip.py 
sudo python3 get-pip.py

sudo apt-get install build-essential libzmq3-dev -y
sudo apt-get install python-dev -y
sudo apt-get install python3-dev -y

sudo pip install distlib setuptools wheel
sudo pip3 install distlib setuptools wheel

sudo pip install numpy
sudo pip3 install numpy
sudo apt-get install cython -y
sudo apt-get install python-dateutil python3-dateutil -y
sudo pip install pytz
sudo pip3 install pytz
sudo pip install bottleneck
sudo pip3 install bottleneck
sudo pip install numexpr
sudo pip3 install numexpr
sudo apt-get install xsel xclip libxml2-dev libxslt1-dev -y
sudo apt-get install python-lxml python3-lxml -y 

sudo fallocate -l 1G tmpswap
sudo mkswap tmpswap
swapon tmpswap
sudo pip3 install pandas
sudo pip install pandas

sudo apt-get install python-h5py python3-h5py -y
sudo apt-get install python-openpyxl python3-openpyxl -y
sudo apt-get install python-tables python3-tables -y
sudo apt-get install python-zmq python3-zmq -y

sudo pip install pygsheets adafruit.io
sudo pip3 install pygsheets adafruit.io

sudo pip install pyserial
sudo pip3 install pyserial

sudo apt-get install python-tk python3-tk python-matplotlib python3-matplotlib
sudo pip install jupyter
sudo pip3 install jupyter