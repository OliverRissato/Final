import { DynamoDB } from '@aws-sdk/client-dynamodb';
import { DynamoDBDocument } from '@aws-sdk/lib-dynamodb';

const dynamo = DynamoDBDocument.from(new DynamoDB());

export const handler = async (event) => {

    console.log('STK2 Received event:', JSON.stringify(event, null, 2));

    const params = {
        TableName: "IoTCatalog",
        Item :{
            "serialNumber": event.serialNumber,
            "timestamp": event.dateTime,
            "activated": event.activated,
            "clientId": event.clientId,
            "device": event.device,
            "type": event.type,
            "payload": event.payload
            }
    };
    
    console.log (JSON.stringify(params));
    
    console.log("STK3 Saving Telemetry Data");
    
        dynamo.put (params, function(err, data) {
        if (err) {
            console.error("STK Unable to add device. Error JSON:", JSON.stringify(err, null, 2));
            context.fail();
        } else {
            console.log(data)
            console.log("STK4 Data saved:", JSON.stringify(params, null, 2));
            context.succeed();
            return {"message": "Item created in DB"}
        }
    });

};