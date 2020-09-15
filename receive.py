
# -*- coding: utf-8 -*-
from bluepy.btle import Peripheral
import bluepy.btle as btle
import sys
import time
import statistics
import binascii

import urllib.request
import json
import struct
from elasticsearch import Elasticsearch

es = Elasticsearch(host='', port=9200,http_auth=('elastic','秘密'))


def conversion(value):
    # print(type(value))
    print(value)
    # value = value.hex()
    # print('value.hex type',type(value))
    # print(value)
    # value = [value[i: i+2] for i in range(0, len(value), 2)]
    # print('slice',value)
    # print(type(value))
    # value_int = [int(s,16) for s in value]
    # print(value_int)
    format = "fff"
    record = struct.unpack(format, value[:12]) 
    print(record)
    return record


def uploadSensorValues(value):
  #送信先URL
    #sensordata = {
     #   "value": value,"date": ut
    #}
    dict = {}
    dict["temp"] = value[0]
    dict["hum"] = value[1]
    dict["lux"] = value[2]
   
    # headers = {'content-type': 'applocation/json'}
    # req = urllib.request.Request(url, enc.encode(), headers)
    # with urllib.request.urlopen(req)as res:
    #     body = res.read()
    # print(res)
    # pass
    # get unixtime
    ut = str(time.time()*1000)
    dict["date"] = ut
    enc = json.dumps(dict)
    print(enc)
    try:
        res = es.index(index="aquaponics", doc_type='fish', body=enc)
    except Exception as e:
        print(e)


str_dev =  "FB:E8:A9:24:68:CB" #MACアドレス
str_dev2 =  "ED:9E:B2:A3:DD:37" #MACアドレス



class MyDelegate(btle.DefaultDelegate):
    def __init__(self, params):
        btle.DefaultDelegate.__init__(self)

class MyPeripheral(Peripheral):
    def __init__(self, addr):
        Peripheral.__init__(self, addr, addrType="random")

def uploadAtlasSensorValues(value):
  #送信先URL
    #sensordata = {
     #   "value": value,"date": ut
    #}
    dict = {}
    dict["RTD"] = value[0]
    dict["DO"] = value[1]
    dict["PH"] = value[2]
   
    # headers = {'content-type': 'applocation/json'}
    # req = urllib.request.Request(url, enc.encode(), headers)
    # with urllib.request.urlopen(req)as res:
    #     body = res.read()
    # print(res)
    # pass
    # get unixtime
    ut = str(time.time()*1000)
    dict["date"] = ut
    enc = json.dumps(dict)
    print(enc)
    try:
        res = es.index(index="aquaponics", doc_type='fish', body=enc)
    except Exception as e:
        print(e)



def main():
    # 初期設定
  try:
    perip = MyPeripheral(str_dev) #引数はMACアドレス
    perip.setDelegate(MyDelegate(btle.DefaultDelegate))

    # device name, serial number
    devname = perip.readCharacteristic(0x0003)
    print( "Device Name: %s" % devname )

    # get sensordata from BLE
    value =perip.readCharacteristic(0x001f)
    #受信データを変換
    value = conversion(value)
    #受信データをアップロード
    uploadSensorValues(value)

  except Exception as e:
    print(e)
    #atlasセンサ
  time.sleep(1)
  try:
    perip2 = MyPeripheral(str_dev2) #引数はMACアドレス
    perip2.setDelegate(MyDelegate(btle.DefaultDelegate))

    # device name, serial number
    devname = perip2.readCharacteristic(0x0003)
    print( "Device Name: %s" % devname )



    # get sensordata from BLE
    value =perip2.readCharacteristic(0x001f)
    #受信データを変換
    value = conversion(value)
    #受信データをアップロード
    uploadAtlasSensorValues(value)

  except Exception as e:
    print(e)

if __name__ == "__main__":
    while True:
        main()
        time.sleep(1)




