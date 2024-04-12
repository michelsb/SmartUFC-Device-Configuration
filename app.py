import os
import requests
from flask import Flask, request, jsonify

# List Services
#curl -G -X GET 'http://localhost:4041/iot/services' -H 'fiware-service: smartufc' -H 'fiware-servicepath: /campuspici' | jq

# List Devices
#curl -G -X GET 'http://localhost:4041/iot/devices' -H 'fiware-service: smartufc' -H 'fiware-servicepath:/campuspici' | jq

# Get All Devices Information
#curl -G -X GET 'http://localhost:1026/v2/entities' -d 'options=keyValues' -H 'fiware-service: smartufc' -H 'fiware-servicepath: /campuspici' | jq


# curl -iX PUT \
#   'http://localhost:4041/iot/devices/airConditioner001001001' \
#   -H 'Content-Type: application/json' \
#   -H 'fiware-service: smartufc' \
#   -H 'fiware-servicepath: /campuspici' \
#   -d '{
#   "static_attributes": [       
#        {"name": "pcbId", "type": "Text", "value":"airConditioner001002001"}       
#    ]
# }'


# Broker address and port
# fiware-service
# /campuspici 
# Iot-Agent address and port

# Get environment variables
SMARTCONFIG_PORT = os.getenv('SMARTCONFIG_PORT')
ORION_HOST = os.getenv('ORION_HOST')
ORION_PORT = os.getenv('ORION_PORT')
IOTA_HOST = os.getenv('IOTA_HOST')
IOTA_NORTH_PORT = os.getenv('IOTA_NORTH_PORT')
MOSQUITTO_PORT = os.getenv('MOSQUITTO_PORT')
FIWARE_SERVICE = os.getenv('FIWARE_SERVICE')
FIWARE_SERVICEPATH = os.getenv('FIWARE_SERVICEPATH')
headers={'fiware-service':FIWARE_SERVICE,'fiware-servicepath':FIWARE_SERVICEPATH}
device_config={'device_id':None,'api_key':None,'mqtt_port':MOSQUITTO_PORT}

app = Flask(__name__)

def getDeviceIdByEntityName(entity_name):
    device_id = entity_name.replace(":","")
    device_id = device_id[0].lower()+device_id[1:]
    return device_id    

def getDeviceIdByPcbIdType(pcbId,type):
    device_id = type.lower()+pcbId
    return device_id

def getDeviceByPcbId(pcbId):
    
    parameters={'options':'keyValues','attrs':'id,type','q':'pcbId=='+pcbId}
    # returns: api_key, device_id, broker_ip, broker_port

    try:
        response = requests.get('http://'+ORION_HOST+':'+ORION_PORT+'/v2/entities', timeout=5,headers=headers,params=parameters)
        response.raise_for_status()   
        return response.json()
    except requests.exceptions.HTTPError as errh:
        print(errh)
    except requests.exceptions.ConnectionError as errc:
        print(errc)
    except requests.exceptions.Timeout as errt:
        print(errt)
    except requests.exceptions.RequestException as err:
        print(err)

def getDeviceById(type, device_id):
    
    id = type+":"+device_id
    parameters={'options':'keyValues','attrs':'id,type','q':'id=='+id}
    # returns: api_key, device_id, broker_ip, broker_port
    try:
        response = requests.get('http://'+ORION_HOST+':'+ORION_PORT+'/v2/entities', timeout=5,headers=headers,params=parameters)
        response.raise_for_status()   
        return response.json()
    except requests.exceptions.HTTPError as errh:
        print(errh)
    except requests.exceptions.ConnectionError as errc:
        print(errc)
    except requests.exceptions.Timeout as errt:
        print(errt)
    except requests.exceptions.RequestException as err:
        print(err)

def getApiKeyByEntityType(entity_type):
    try:
        response = requests.get('http://'+IOTA_HOST+':'+IOTA_NORTH_PORT+'/iot/services', timeout=5,headers=headers)
        response.raise_for_status()        
        data = response.json()
        for service in data['services']:
            if service["entity_type"]==entity_type:
                return service["apikey"]
        return None    
    except requests.exceptions.HTTPError as errh:
        print(errh)
    except requests.exceptions.ConnectionError as errc:
        print(errc)
    except requests.exceptions.Timeout as errt:
        print(errt)
    except requests.exceptions.RequestException as err:
        print(err)


@app.route('/device',methods=['GET'])
def getDeviceConfiguration():
    args = request.args 
    pcbId = args.get("pcbId")  
    type = args.get("type")
    device_id=getDeviceIdByPcbIdType(pcbId,type)
    device_config['device_id']=device_id
    api_key=getApiKeyByEntityType(type) # test if None
    if api_key is None:
        return jsonify({"error": "No service group found for this PCB ID",}), 403
    device_config['api_key']=api_key
    return jsonify(device_config) 

# @app.route('/device',methods=['GET'])
# def getDeviceConfiguration():
#     args = request.args        
#     data = getDeviceByPcbId(args.get("id"))
#     if len(data) == 0:
#         return jsonify({"error": "No device created for this PCB ID",}), 403
#     if len(data) > 1:
#         return jsonify({"error": "More than one device for this PCB ID",}), 403
#     else:
#         device_id=getDeviceIdByEntityName(data[0]["id"])
#         device_config['device_id']=device_id
#         api_key=getApiKeyByEntityType(data[0]["type"]) # test if None
#         if device_id is None:
#             return jsonify({"error": "No service group found for this PCB ID",}), 403
#         device_config['api_key']=api_key
#         return jsonify(device_config)


if __name__ == '__main__':
    app.run(host='0.0.0.0',port=SMARTCONFIG_PORT)