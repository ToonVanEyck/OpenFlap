# Testing commands

Building all tests:
```
python software/controller_v2/build_test.py -s software/controller_v2/ -b build/controller/
```

Runing all tests with pytest:
```
pytest software/controller_v2/ --build-dir build/controller/ -m esp32
```

curl --header "Content-Type: application/json" POST --data '[{"module": 0,"character_set": [" ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "$", "!", "?", ".", ",", ":", "/", "@", "#", "&"],"character": "5","calibration": {"offset": 0}}]' http://192.168.0.43:80/api/module


curl --header "Content-Type: application/json" GET http://192.168.0.43:80/api/module

curl --header "Content-Type: application/json" POST --data '[{"module": 0,"character_set": ["Y", "O", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "€", "$", "!", "?", ".", ",", ":", "/", "@", "#", "&"]}]' http://192.168.0.43:80/api/module