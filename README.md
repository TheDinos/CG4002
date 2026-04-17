# Prerequisites

## Ultra96

•	Python 3.8 or newer installed

•	pip package manager available

## Laptop broker

•	SoC VPN available

## Hardware 
•	Arduino 1.8.19 installed

•	Required libraries: esp-mqtt, ArduinoJson, I2CDev (provided in libraries folder), MPU6050 (provided in libraries folder)

# Hardware Setup
1. Edit IP address of mqtt broker and wifi address/password for both glove and bow
2. Compile and upload

# Communications Setup
## Transfer communication files
1.	Unzip comms.zip
2.	Copy the extracted comms folder to the Ultra96.
3.	Ensure the folder is located at: /home/xilinx/comms
4.	If the folder is placed elsewhere, update the file path references inside: comms.py

## Install Python Dependencies on Ultra96
1.	pip install paho-mqtt
2.	pip install numpy
3.	pip install pynq

## Setup MQTT Broker (Laptop)
1.	Unzip comms_broker.zip
2.	Install Eclipse Mosquitto
3.	Place the Mosquitto executable inside the comms_broker folder. 
4.	Ensure Mosquitto has the required permissions to be executed from that directory.

# Game setup
## Starting the MQTT broker (Laptop)
1.	Ensure the laptop is connected to the SoC VPN
2.	Navigate to the broker directory cd comms_broker
3.	Start the MQTT broker: mosquitto -c mosquitto.conf -v

## Establish Reverse SSH to Ultra96
1.	Run the following command from the laptop terminal: 
ssh -R 8883:localhost:8883 xilinx@makerslab-fpga-48.ddns.comp.nus.edu.sg
Replace makerslab-fpga-48.ddns.comp.nus.edu.sg with the hostname of the Ultra96.

## Configure FPGA environment

Once connected to the Ultra96, run the following commands:
1.	sudo su
2.	export XILINX_XRT=/usr
3.	source /tmp/kws-venv/bin/activate
4.	source /etc/profile.d/xrt_setup.sh

## Run communications program on Ultra96

1.	Navigate to the communications folder: cd /home/xilinx/comms
2.	Run: python3 comms.py

## Unity Project Setup
1. Download project from (link)
2. Using Unity Hub, open the project
3. In InputManager, change the IP address to the comms
4. Change the CA accordingly
5. Build and run the project on iOS device

