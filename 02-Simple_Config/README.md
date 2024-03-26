# Key-Value based Configurations
This examples shows how to use configuration service without JSON deserialization. You could simply get each parameter by fetching it with its path.  
## Download Settings
Keep this JSON object in mind.
```json
{
    "speed":180,
    "alarm":{
        "number":"+188554436",
        "msg1":"Fire, please help",
        "msg2":"Emergency, please help"
    }
}
```
The code below downloads all parameters into an object and then extracts the required parameters individually.
```cpp
int speed = 100;
String alarm_num;
String alarm_msg1;

OTAdrive_ns::KeyValueList configs;
configs = OTADRIVE.getConfigValues();

if (configs.containsKey("speed"))
    speed = configs.value("speed").toInt();

if(configs.containsKey("alarm.number"))
    alarm_num = configs.value("alarm.number");

if(configs.containsKey("alarm.msg1"))
    alarm_msg1 = configs.value("alarm.msg1");
```