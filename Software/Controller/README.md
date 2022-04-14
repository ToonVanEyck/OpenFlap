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