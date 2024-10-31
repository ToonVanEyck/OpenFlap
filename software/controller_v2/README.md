# Testing commands

Building all tests:
```
python software/controller_v2/build_test.py -s software/controller_v2/ -b build/controller/
```

Runing all tests with pytest:
```
pytest software/controller_v2/ --build-dir build/controller/ -m esp32
```