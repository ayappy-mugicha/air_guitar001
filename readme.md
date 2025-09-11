pip install paho-mqtt

<!-- gcc -c -fPIC servo.c -o libservo.o -->
gcc -shared -o libservo.so servo.c

sudo apt update
sudo apt install git-core
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi
./build

https://akizukidenshi.com/catalog/g/g110350/