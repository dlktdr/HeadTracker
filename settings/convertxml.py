#!/usr/bin/python

# Converts the Settings.csv into a .xml document.. Not sure what I like more
#   .csv is easy to see all the data and edit using a spreadsheet application
#   .xml is easier to add a new feature specific to one type.. Hmm

from array import array
import csv
from email.policy import strict

import xml.etree.ElementTree as ET
from xml.dom import minidom
import os

# CSV Column's
coltype = 0
coldata = 1
colname = 2
coldefault = 3
colmin = 4
colmax = 5
coldesc = 6
colfwonevnt = 7
coldivisor = 8
colround = 9

const = list()
data = list()
dataarrays = list()
settings = list()
settingsarrays = list()

with open('settings.csv', newline='') as csvfile:
    setns = csv.reader(csvfile, delimiter=',', quotechar='\"')
    itersetns = iter(setns)
    next(itersetns)
    for row in setns:
        if "const" in row[coldata].lower():
          const.append(row)
        if "setting" in row[coldata].lower():
          if "[" in row[colname]:
            settingsarrays.append(row)
          else:
            settings.append(row)
        if "data" in row[coldata].lower():
          if "[" in row[colname]:
            dataarrays.append(row)
          else:
            data.append(row)

root = ET.Element('HeadTracker')
for row in const:
  conste = ET.SubElement(root, "const")
  name = ET.SubElement(conste, "name")
  name.text = row[colname]
  if row[coldesc].strip() != "":
    name = ET.SubElement(conste, "discription")
    name.text = row[coldesc]
  name = ET.SubElement(conste, "type")
  name.text = row[coltype]
  name = ET.SubElement(conste, "value")
  name.text = row[coldefault]

def addSetting(row):
  conste = ET.SubElement(root, "setting")
  name = ET.SubElement(conste, "name")
  name.text = row[colname]
  name = ET.SubElement(conste, "discription")
  name.text = row[coldesc]
  name = ET.SubElement(conste, "type")
  name.text = row[coltype].lower().strip()
  name = ET.SubElement(conste, "default")
  name.text = row[coldefault].lower().strip()
  if row[coltype].lower().strip() != 'bool' and row[coltype].lower().strip() != 'char':
    if row[coltype].lower().strip()[:1] != 'u' and row[colmin].strip() == 0:
      name = ET.SubElement(conste, "minimum")
      name.text = row[colmin]
    name = ET.SubElement(conste, "maximum")
    name.text = row[colmax]
    if row[colround].strip() != "":
      name = ET.SubElement(conste, "roundto")
      name.text = row[colround]
    if row[colfwonevnt].strip() != "":
      name = ET.SubElement(conste, "FWOnChangeFunc")
      name.text = row[colfwonevnt]

def addData(row):
  conste = ET.SubElement(root, "data")
  name = ET.SubElement(conste, "name")
  name.text = row[colname]
  name = ET.SubElement(conste, "discription")
  name.text = row[coldesc]
  name = ET.SubElement(conste, "type")
  name.text = row[coltype].lower().strip()
  if row[coltype].lower().strip() != 'bool' and row[coltype].lower().strip() != 'char':
    if row[colround].strip() != "":
      name = ET.SubElement(conste, "round")
      name.text = row[colround]

for row in settings:
  addSetting(row)

for row in settingsarrays:
  addSetting(row)

for row in data:
  addData(row)

for row in dataarrays:
  addData(row)

xmlstr = minidom.parseString(ET.tostring(root)).toprettyxml(indent="   ",newl='\n')
with open("settings.xml", "w") as f:
    f.write(xmlstr)

