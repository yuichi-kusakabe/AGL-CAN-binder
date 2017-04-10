# AGL-CAN-binder
AGL low level CAN bus binder.

#For build and run Please see README.txt file

#This software include simple CAN(EngineSpeed and VehicleSpeed) json file
-sample EngineSpeed CAN information
CAN ID 010
data 2byte
Data thinning cycle 200ms
 
-sample VehicleSpeed CAN information
CAN ID 011
data 2byte
Data thinning cycle 200ms

simple.json
{
	"RECEPTION" :
		{
			"STANDARD": {
				"010" : {
					"DLC"	: 2 ,
					"MODE"	: "CAN_THINOUT_CHG",
					"CYCLE"	: 200,
					"DATA" : [
						{
						"POS"			: 2 ,
						"OFFS"			: 0 ,
						"LEN"			: 16 ,
						"CUSTOM"		: false, 
						"PROPERTY"		: "EngineSpeed",
						"ZONE"			: ["Zone::None"],
						"TYPE"			: "uint16_t",
						"MULTI-PARAM"	: 0,
						"ADD-PARAM"		: 0
						}
					]
				},
				"011" : {
					"DLC"	: 2 ,
					"MODE"	: "CAN_THINOUT_CHG",
					"CYCLE"	: 200,
					"DATA" : [
						{
						"POS"			: 2 ,
						"OFFS"			: 0 ,
						"LEN"			: 16 ,
						"CUSTOM"		: false, 
						"PROPERTY"		: "VehicleSpeed",
						"ZONE"			: ["Zone::None"],
						"TYPE"			: "uint16_t",
						"MULTI-PARAM"	: 0,
						"ADD-PARAM"		: 0
						}
					]
				},
			},
			"EXTENDED" : {
			}
		}
	,
	"TRANSMISSION" :
		{
			"STANDARD": {
			},
			"EXTENDED" : {
			}
		}
}

-test VehicleSpeed
root@m3ulcb:~# cansend vcan0 011#0000
root@m3ulcb:~# cansend vcan0 011#0010
root@m3ulcb:~# cansend vcan0 011#0100
root@m3ulcb:~# cansend vcan0 011#0110
root@m3ulcb:~# cansend vcan0 011#0111
ON-EVENT canivi/VehicleSpeed({"event":"canivi\/VehicleSpeed","data":{"value":0},"jtype":"afb-event"})
ON-EVENT canivi/VehicleSpeed({"event":"canivi\/VehicleSpeed","data":{"value":16},"jtype":"afb-event"})
ON-EVENT canivi/VehicleSpeed({"event":"canivi\/VehicleSpeed","data":{"value":256},"jtype":"afb-event"})
ON-EVENT canivi/VehicleSpeed({"event":"canivi\/VehicleSpeed","data":{"value":272},"jtype":"afb-event"})
ON-EVENT canivi/VehicleSpeed({"event":"canivi\/VehicleSpeed","data":{"value":273},"jtype":"afb-event"})

-test EngineSpeed
root@m3ulcb:~# cansend vcan0 010#0000
root@m3ulcb:~# cansend vcan0 010#0001
root@m3ulcb:~# cansend vcan0 010#0010
root@m3ulcb:~# cansend vcan0 010#0100
root@m3ulcb:~# cansend vcan0 010#1000
ON-EVENT canivi/EngineSpeed({"event":"canivi\/EngineSpeed","data":{"value":0},"jtype":"afb-event"})
ON-EVENT canivi/EngineSpeed({"event":"canivi\/EngineSpeed","data":{"value":1},"jtype":"afb-event"})
ON-EVENT canivi/EngineSpeed({"event":"canivi\/EngineSpeed","data":{"value":16},"jtype":"afb-event"})
ON-EVENT canivi/EngineSpeed({"event":"canivi\/EngineSpeed","data":{"value":256},"jtype":"afb-event"})
ON-EVENT canivi/EngineSpeed({"event":"canivi\/EngineSpeed","data":{"value":4096},"jtype":"afb-event"})

