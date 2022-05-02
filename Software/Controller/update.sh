#!/bin/bash
curl -s -F 'controller_controller_firmware=@build/split-flap-controller.bin' openflap.local/firmware >> /dev/null