#!/bin/bash

echo "will open your config for modification, you will need to sudo"
sudo cp /boot/config.txt ~/configBU.txt
sudo echo "hdmi_cvt=1080 1920 60 5 0 0 0" /boot/config.txt
sudo echo "hdmi_group=2" /boot/config.txt
sudo echo "hdmi_mode=87" /boot/config.txt
sudo echo "hdmi_timings=1080 1 100 10 60 1920 1 4 2 4 0 0 0 60 0 144000000 3" /boot/config.txt
sudo echo "max_framebuffer_width=1920" /boot/config.txt
sudo echo "max_framebuffer_height=1920" /boot/config.txt
echo "I will reboot now, you can check if things are set right by checking tvservice -s"
sudo reboot

