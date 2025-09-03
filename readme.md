pip install paho-mqtt
gcc servo.c -o servo
gcc servo.c -shared -o servo

sudo apt update
sudo apt install git-core
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi
./build