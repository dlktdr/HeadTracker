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

import set_common as s

s.readSettings()

root = ET.Element('HeadTracker')
for row in s.const:
  conste = ET.SubElement(root, "const")
  name = ET.SubElement(conste, "name")
  name.text = row[s.colname]
  if row[s.coldesc].strip() != "":
    name = ET.SubElement(conste, "discription")
    name.text = row[s.coldesc]
  name = ET.SubElement(conste, "type")
  name.text = row[s.coltype]
  name = ET.SubElement(conste, "value")
  name.text = row[s.coldefault]

def addSetting(row):
  conste = ET.SubElement(root, "setting")
  name = ET.SubElement(conste, "name")
  name.text = row[s.colname]
  name = ET.SubElement(conste, "discription")
  name.text = row[s.coldesc]
  name = ET.SubElement(conste, "type")
  name.text = row[s.coltype].lower().strip()
  name = ET.SubElement(conste, "default")
  name.text = row[s.coldefault].lower().strip()
  if row[s.coltype].lower().strip() != 'bool' and row[s.coltype].lower().strip() != 'char':
    if row[s.coltype].lower().strip()[:1] != 'u' and row[s.colmin].strip() == 0:
      name = ET.SubElement(conste, "minimum")
      name.text = row[s.colmin]
    name = ET.SubElement(conste, "maximum")
    name.text = row[s.colmax]
    if row[s.colround].strip() != "":
      name = ET.SubElement(conste, "roundto")
      name.text = row[s.colround]
    if row[s.colfwonevnt].strip() != "":
      name = ET.SubElement(conste, "fwonchange")
      name.text = row[s.colfwonevnt]
    if row[s.colbleaddr].strip() != "":
      name = ET.SubElement(conste, "bleaddr")
      name.text = row[s.colbleaddr]


def addData(row):
  conste = ET.SubElement(root, "data")
  name = ET.SubElement(conste, "name")
  name.text = row[s.colname]
  name = ET.SubElement(conste, "discription")
  name.text = row[s.coldesc]
  name = ET.SubElement(conste, "type")
  name.text = row[s.coltype].lower().strip()
  if row[s.coltype].lower().strip() != 'bool' and row[s.coltype].lower().strip() != 'char':
    if row[s.colround].strip() != "":
      name = ET.SubElement(conste, "round")
      name.text = row[s.colround]

for row in s.settings:
  addSetting(row)

for row in s.settingsarrays:
  addSetting(row)

for row in s.data:
  addData(row)

for row in s.dataarrays:
  addData(row)

xmlstr = minidom.parseString(ET.tostring(root)).toprettyxml(indent="   ",newl='\n')
with open("settings.xml", "w") as f:
    f.write(xmlstr)

