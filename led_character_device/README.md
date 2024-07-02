# How to run:
fisrt, run:
```
make all
```
then:
```
insmod led_char_device.ko
```
character device to control led will be added to /dev/
to turn on led:
```
echo 1 > /dev/led_char_device
```
to turn off led:
```
echo 0 > /dev/led_char_device
```
to read led stage:
```
cat /dev/led_char_device
```
to remove driver:
```
rmmod led_char_device
```
and don't forget clean all files has been compiled by make:
```
make clean
```
