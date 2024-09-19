#!/bin/bash
curl -s -F 'controller_firmware=@build/split-flap-controller.bin' openflap.local/firmware >> /dev/null