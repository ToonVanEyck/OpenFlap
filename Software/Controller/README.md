Open Flap Controller
====================

The open flap controller is based on the esp32-wroom module. The module is programmed with the esp-idf. More information about the working with esp-idf can be found [here](https://www.espressif.com/en/products/sdks/esp-idf).

Debug Over TCP Socket:
----------------------
The controller exposes tcp socket on port ```1234```, a client connecting to this port will receive debug information about the operation of the controller.
Connect to the port using:
```
nc openflap.local 1234
```
Or using the ```logcat.sh``` script:
```
./logcat.sh
```

Over-The-Air Firmware Update:
-----------------------------
A new ```.bin``` file can be uploaded through the controller hosted webpage. Alternatively, the ```update.sh``` script can be used. This script use the latest binary from the build directory.

```
./update.sh
```


generate single nvs image:
--------------------------
Set according values in ```nvs_data.csv```
```
${IDF_PATH}/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate --outdir build nvs_data.csv nvs.bin 0x4000
```

flash nvs partition images and firmware:
----------------------------------------
```
esptool.py -p /dev/ttyUSB0 -b 460800 --before=default_reset --after=hard_reset write_flash --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 build/bootloader/bootloader.bin 0x10000 build/split-flap-controller.bin 0x8000 build/partition_table/partition-table.bin 0xd000 build/ota_data_initial.bin 0x9000 build/nvs.bin
```
