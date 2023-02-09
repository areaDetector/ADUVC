#!/usr/bin/env python3


import argparse
import os
from subprocess import Popen, PIPE


def get_camera_list():
    cameras = set()
    try:
        p = Popen(['cameraDetector/uvc_locater', '-c'], stdout=PIPE, stderr=PIPE)
        out, err = p.communicate()
        for line in out.decode('utf-8').splitlines():
            cameras.add(line.split(',')[1])
    except Exception as e:
        print(f'ERROR - Failed to run uvc_locater! {str(e)}')
        return
    return cameras


def parse_args():

    parser = argparse.ArgumentParser(description='Utility for generating udev rule file for specific server')
    parser.add_argument('-i', '--install', action='store_true', help='Installs the udev file into /etc/udev/rules.d/###-usbcams.rules, or replaces it if it exists.')
    return parser.parse_args()


if __name__ == '__main__':
    args = parse_args()
    cameras = get_camera_list()

    udev_file_contents = ''
    for camera in cameras:
        udev_file_contents += f'SUBSYSTEM=="usb", ATTRS{{idProduct}}=="{camera.split("x")[1]}", MODE="0666"\n'

    print(f'Generated UDEV rules:\n\n{udev_file_contents}')

    if args.install:
        try:
            numbers = []
            for file in os.listdir('/etc/udev/rules.d'):
                numbers.append(file.split('-')[0])
            
            udev_file_num = 0
            for i in range(90, 100):
                if i not in numbers or i == 99:
                    udev_file_num = i
                    break
    
            if os.path.exists(f'/etc/udev/rules.d/{udev_file_num}-usbcams.rules'):
                os.remove(f'/etc/udev/rules.d/{udev_file_num}-usbcams.rules')
            
            with open(f'/etc/udev/rules.d/{udev_file_num}-usbcams.rules', 'w') as fp:
                fp.write(udev_file_contents)
            print(f'Successfully generated /etc/udev/rules.d/{udev_file_num}-usbcams.rules.')
            print('Restart your machine or re-trigger udevadm and re-plug your devices for the rules to apply')
        except PermissionError:
            print('ERROR - You do not have permission to generate and install udev files!')
