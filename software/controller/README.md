# Testing commands

Building all tests:
```
python software/controller/build_test.py -s software/controller/ -b build/controller/
```

Running all tests with pytest:
```
pytest software/controller/ --build-dir build/controller/ -m esp32
```

curl --header "Content-Type: application/json" POST --data '[{"module": 0,"character_set": [" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "$", "!", "?", ".", ",", ":", "/", "@", "#", "&"],"character": "5","calibration": {"offset": 0}}]' http://openflap.local/api/module


curl --header "Content-Type: application/json" http://openflap.local/api/module

curl --header "Content-Type: application/json" POST --data '[{"module": 0,"character_set": ["Y", "O", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "$", "!", "?", ".", ",", ":", "/", "@", "#", "&"]}]' http://openflap.local/api/module

curl -T build/module/bin/OpenFlap_Module_App.bin http://openflap.local/api/module/firmware