import asyncio
from azure.servicebus.aio import ServiceBusClient
from azure.identity.aio import DefaultAzureCredential
import os

# importing all required libraries
import telebot
from telethon.sync import TelegramClient
from telethon.tl.types import InputPeerUser, InputPeerChannel
from telethon import TelegramClient, sync, events

import json

FULLY_QUALIFIED_NAMESPACE = "ectahawk"
QUEUE_NAME = "syscom"
NAMESPACE_CONNECTION_STR = "Endpoint=sb://ectahawk.servicebus.windows.net/;SharedAccessKeyName=Python;SharedAccessKey=cAIMSR1T9+j3sX6mbBjFYGZ0q+itqxzde+ASbDtZea8=;EntityPath=syscom"

credential = DefaultAzureCredential()

api_id = '29995377'
api_hash = 'f05c581f119ee03934bcc156b3960ca7'
token = '6056319696:AAFtXhIrj7ypMJj1oGaADkEP07J0j_ThRSs'

# your phone number
phone = '+33769203372'

# creating a telegram session and assigning
# it to a variable client
client = TelegramClient('session', api_id, api_hash)
  
# connecting and building the session
client.connect()

# in case of script ran first time it will
# ask either to input token or otp sent to
# number or sent or your telegram id
if not client.is_user_authorized():
  
    client.send_code_request(phone)
     
    # signing in the client
    client.sign_in(phone, input('Enter the code: '))

async def run():
    # create a Service Bus client using the connection string
    async with ServiceBusClient.from_connection_string(
        conn_str=NAMESPACE_CONNECTION_STR,
        logging_enable=True) as servicebus_client:

        async with servicebus_client:
            # get the Queue Receiver object for the queue
            receiver = servicebus_client.get_queue_receiver(queue_name=QUEUE_NAME)
            async with receiver:
                received_msgs = await receiver.receive_messages(max_wait_time=5, max_message_count=20)
                for msg in received_msgs:
                    #print("Received: " + str(msg))
                    jess_dict = json.loads(str(msg))
                    #if "" in jess_dict:
                    print(jess_dict)
                    # complete the message so that the message is removed from the queue
                    #await receiver.complete_message(msg)
                    try:
                        # receiver user_id and access_hash, use
                        # my user_id and access_hash for reference
                        receiver = InputPeerUser('user_id', 'user_hash')
                     
                        # sending message using telegram client
                        await client.send_message(receiver, jess_dict, parse_mode='html')
                    except Exception as e:
                         
                        # there may be many error coming in while like peer
                        # error, wrong access_hash, flood_error, etc
                        print(e);


asyncio.run(run())
 
async def main():
    await client.start()
    await client.run_until_disconnected()

if __name__ == '__main__':
    asyncio.run(main())

# disconnecting the telegram session
client.disconnect()