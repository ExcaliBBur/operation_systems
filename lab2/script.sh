sudo rmmod tcp_udp_module.ko
sudo rmmod unix_module.ko

echo "Removing modules success"

make

sudo insmod tcp_udp_module.ko
sudo insmod unix_module.ko

echo "Inserting modules success"

make clean
