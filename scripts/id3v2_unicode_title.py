#!/usr/bin/python2
# coding: utf-8

import sys
from mutagen.easyid3 import EasyID3

audio = EasyID3(sys.argv[2])
audio["title"] = sys.argv[1].decode('utf-8')
audio.save()
