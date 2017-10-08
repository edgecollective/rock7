import requests
import binascii

message='hello'

imei='300234065358380'

username='donblair@gmail.com'

password='rockblockcat999'

data=binascii.hexlify(message)

r = requests.post("https://core.rock7.com/rockblock/MT",data={'imei':imei,'username':username,'password':password,'data':data})

print(r.status_code,r.reason)

print(r.text[:300]+'...')
