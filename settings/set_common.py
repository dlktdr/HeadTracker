#!/usr/bin/python

from array import array
import csv
from pickle import TRUE

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
colbleaddr = 10

const = list()
data = list()
dataarrays = list()
settings = list()
settingsarrays = list()

read = False

def readSettings():
  global read
  if read == False:
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
    sanity_check()
  read = True

def sanity_check():
  for row in settings:
    try:
      if float(row[colmin]) > float(row[colmax]):
        print("Minimum is greater than Maximum on Setting-" + row[colname])
        exit("Error in sanity check")
      if float(row[coldefault]) < float(row[colmin]):
        print("Default is less than Min on Setting-" + row[colname])
        exit("Error in sanity check")
      if float(row[coldefault]) > float(row[colmax]):
        print("Default is greater than Max on Setting-" + row[colname])
        exit("Error in sanity check")
    except ValueError:
      continue

def typeToJson(type) :
  if type == "float":
    return "flt"
  if type == "double":
    return "dbl"
  if type == "char":
    return "chr"
  return type

def typeToC(type) :
  if type == "u8":
    return "uint8_t"
  if type == "s8":
    return "int8_t"
  if type == "u16":
    return "uint16_t"
  if type == "s16":
    return "int16_t"
  if type == "u32":
    return "uint32_t"
  if type == "s32":
    return "int32_t"
  return type

def QVariantRet(type):
  if type == "u8":
    return ".toUInt()"
  if type == "s8":
    return ".toInt()"
  if type == "u16":
    return ".toUInt()"
  if type == "s16":
    return ".toInt()"
  if type == "u32":
    return ".toUInt()"
  if type == "s32":
    return ".toInt()"
  if type == "float":
    return ".toFloat()"
  if type == "double":
    return ".toDouble()"
  if type == "bool":
    return ".toBool()"
  if type == "char":
    return ".toString()"

