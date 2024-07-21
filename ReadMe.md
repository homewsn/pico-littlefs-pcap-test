The program is designed to reproduce the behavior of the [pico-littlefs-usb](https://github.com/oyama/pico-littlefs-usb) and [littlefs](https://github.com/littlefs-project/littlefs) software package using only a PC. Reads a pre-recorded pcap file of the actual exchange between the PC OS and the USB device (MCU) on which the software package is installed; analyzes USB MSC packets; and then calls the pico-littlefs-usb layer functions. The source code of the package is the same on PC and USB device, the rand library function is replaced with a simple increment, so the execution result should theoretically be the same.
So far, six tests have been carried out: three each on Windows 10 and Ubuntu 20.04.
The test results are:

- Win10 > pico-littlefs-pcap-test -t1w -c

| MCU  | PC |
| ------------- | ------------- |
| create test dir |  |
| USB cable inserted |  |
|  | copy file bbb.txt |
|  | delete file bbb.txt |
|  | copy same file bbb.txt again |
| USB cable pulled out and reinserted |  |
| read file bbb.txt -> no file | read file bbb.txt -> invalid content |
Test result: failed

- Ubuntu > pico-littlefs-pcap-test -t1u -c

| MCU  | PC |
| ------------- | ------------- |
| create test dir |  |
| USB cable inserted |  |
|  | copy file bbb.txt |
|  | delete file bbb.txt |
|  | copy same file bbb.txt again |
| USB cable pulled out and reinserted |  |
| read file bbb.txt -> valid content | read file bbb.txt -> valid content |
Test result: passed

- Win10(Ubuntu) > pico-littlefs-pcap-test -t2w(-t2u) -c

| MCU  | PC |
| ------------- | ------------- |
| create test dir |  |
| create test/aaa.bin (120000 bytes) |  |
| USB cable inserted |  |
| | read file aaa.bin -> valid content |
Test result: passed

- Win10 > pico-littlefs-pcap-test -t3w -c

| MCU  | PC |
| ------------- | ------------- |
| create test dir |  |
| USB cable inserted |  |
|  | copy file bbb.txt |
|  | delete file bbb.txt |
|  | copy file bbb.txt with different content and less size |
| USB cable pulled out and reinserted |  |
| read file bbb.txt -> no file | read file bbb.txt -> valid content |
Test result: failed

- Ubuntu > pico-littlefs-pcap-test -t3u -c

| MCU  | PC |
| ------------- | ------------- |
| create test dir |  |
| USB cable inserted |  |
|  | copy file bbb.txt |
|  | delete file bbb.txt |
|  | copy file bbb.txt with different content and less size |
| USB cable pulled out and reinserted |  |
| read file bbb.txt -> invalid content | read file bbb.txt -> invalid content |
Test result: failed
