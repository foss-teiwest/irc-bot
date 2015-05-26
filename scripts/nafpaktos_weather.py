#!/usr/bin/env python
from xml.dom import minidom
from urllib import urlopen
site = 'http://www.nafpaktia-weather.gr/tmp/database.xml'
site_data = urlopen(site).read()
xmldoc = minidom.parseString(site_data)
tags = ['city', 'date', 'time', 'temp'];
for tag in tags:
    value = xmldoc.getElementsByTagName(tag)
    print('%s => %s') % (tag.upper(), value[0].firstChild.nodeValue)