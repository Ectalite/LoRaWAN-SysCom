import asyncio
from azure.servicebus.aio import ServiceBusClient
from azure.identity.aio import DefaultAzureCredential

import json
import requests
import base64

import os

from dotenv import load_dotenv
load_dotenv()

# Get environment variables
NAMESPACE_CONNECTION_STR_syscom = os.getenv('NAMESPACE_CONNECTION_STR_syscom')
NAMESPACE_CONNECTION_STR_seeeduino = os.getenv('NAMESPACE_CONNECTION_STR_seeeduino')
token = os.getenv('token')
chat_id = os.getenv('chat_id')


list_value_to_exit_raw=['deviceName', 'data']
list_value_to_exit_rxInfo=['rssi', 'loRaSNR', 'channel']

FULLY_QUALIFIED_NAMESPACE = "ectahawk"
QUEUE_NAME_syscom = "syscom"
QUEUE_NAME_seeeduino = "seeeduino"

credential = DefaultAzureCredential()
  
async def run(ConnectionString, QueueName):
    # create a Service Bus client using the connection string
    async with ServiceBusClient.from_connection_string(
      conn_str=ConnectionString,
      logging_enable=True) as servicebus_client:
      
       async with servicebus_client:
         # get the Queue Receiver object for the queue
         receiver = servicebus_client.get_queue_receiver(queue_name=QueueName)
         async with receiver:
            while(1):
               received_msgs = await receiver.receive_messages(max_wait_time=5, max_message_count=20)
               for msg in received_msgs:
   
                  lora_dict = json.loads(str(msg))
   
                  sendTelegram(lora_dict)
                 
                  # complete the message so that the message is removed from the queue
                  await receiver.complete_message(msg)
               

def sendTelegram(loraDict):
   #Mise en forme des données reçues
   dictionnaire={}
   for element in list_value_to_exit_raw:
      if element in loraDict.keys():
         #print(mydict[element])
         dictionnaire[element]=loraDict[element]
      else :
         print(f"l'element {element} non dans le dictionnaire")
      #print("\n et maintenant rxInfo \n")

   for element in list_value_to_exit_rxInfo:
      if element in loraDict['rxInfo'][0].keys():
         #print(loraDict['rxInfo'][0][element])
         dictionnaire[element]=loraDict['rxInfo'][0][element]
      else :
         print(f"l'element {element} non dans le dictionnaire rxinfo")

   print(dictionnaire)
   print(dictionnaire['data'])
   
   message = "<b>New message:</b>\nDevice Name: "+ dictionnaire['deviceName'] \
            + "\nRSSI: " + str(dictionnaire['rssi']) + "\nSNR: " + str(dictionnaire['loRaSNR'])\
            + "\nSpreading factor: " + str(dictionnaire['channel']) \
            + "\nData: " + base64.b64decode(dictionnaire['data']).decode('utf-8')
   
   #Envoi par telegram
   requests.get("https://api.telegram.org/bot"+token+"/sendMessage?chat_id="+chat_id+"&parse_mode=HTML&text="+message)
   

async def main():
    periodic_a_task = asyncio.create_task(run(NAMESPACE_CONNECTION_STR_syscom, QUEUE_NAME_syscom))
    periodic_b_task = asyncio.create_task(run(NAMESPACE_CONNECTION_STR_seeeduino, QUEUE_NAME_seeeduino))
    await asyncio.gather(periodic_a_task, periodic_b_task)

if __name__ == '__main__':
    asyncio.run(main())
    #lora_dict = {'applicationID': '2', 'applicationName': 'Syscom', 'deviceName': 'LoraE5-1', 'devEUI': 'LPfxIDIwvTM=', 'rxInfo': [{'gatewayID': 'qlVaAAAAAQE=', 'time': None, 'timeSinceGPSEpoch': None, 'rssi': -99, 'loRaSNR': 10.8, 'channel': 7, 'rfChain': 0, 'board': 0, 'antenna': 0, 'location': {'latitude': 46.997652234796135, 'longitude': 6.938751339912415, 'altitude': 450, 'source': 'UNKNOWN', 'accuracy': 0}, 'fineTimestampType': 'NONE', 'context': 'qwhGxA==', 'uplinkID': 'L7yj2TnyTb6I192nb0KvAA==', 'crcStatus': 'CRC_OK'}], 'txInfo': {'frequency': 867900000, 'modulation': 'LORA', 'loRaModulationInfo': {'bandwidth': 125, 'spreadingFactor': 7, 'codeRate': '4/5', 'polarizationInversion': False}}, 'adr': True, 'dr': 5, 'fCnt': 10, 'fPort': 8, 'data': 'QW5kcm9pZA==', 'objectJSON': '', 'tags': {}, 'confirmedUplink': False, 'devAddr': '5AH8Tw==', 'publishedAt': '2023-03-30T12:57:59.700448359Z', 'deviceProfileID': '572ee550-3356-448f-8be4-cd8197910410', 'deviceProfileName': 'SysCom'}
    #sendTelegram(lora_dict)

