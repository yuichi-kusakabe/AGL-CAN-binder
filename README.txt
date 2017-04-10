【For Build】
  Yoctoに組み込まず(提供meta先未定)
  caniviをcmake する場合、MAKE_TOOL_CHAIN_FILEにて クロス環境を指定する。

    $ PATH=/home/Devel/AGL/RCarGen3-agl/build/tmp/sysroots/x86_64-linux/usr/bin/aarch64-agl-linux:$PATH
	$ mkdir build
	$ cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../RcarGen3.cmake ..
	$ make
	# make install  ※ <wgt化は、installで作成する予定>

【For run】
1) ambd が起動している場合、ambdを停止する。
   If ambd running, Please stop ambd

	systemctl stop ambd

2) canivi-Config設定
  # can I/Fデバイスの設定。
    set up CAN IF name

 ${AFB_ROOTDIR}/canivi.json

3) Start the binder 

$ cd af_binder
$ afb-daemon --token=3210 --ldpaths=. --port=5555 --rootdir=. --verbose [--verbose [ --verbose]]

※ token,portは、システムで一意な設定とする事。
※ --verbose 指定は、ログ出力レベル に合わせ増やす(--verbose指定数によりログレベルが上る)


4) local Test from demo-client

$ afb-client-demo ws://localhost:5555/api?token=3210
<以下標準入力から入力>

canivi subscribe { "event" : "VehicleSpeed" }
   or
canivi subscribe { "event" : "*" }

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
                                        "DLC"   : 2 ,
                                        "MODE"  : "CAN_THINOUT_CHG",
                                        "CYCLE" : 200,
                                        "DATA" : [
                                                {
                                                "POS"                   : 2 ,
                                                "OFFS"                  : 0 ,
                                                "LEN"                   : 16 ,
                                                "CUSTOM"                : false,
                                                "PROPERTY"              : "EngineSpeed",
                                                "ZONE"                  : ["Zone::None"],
                                                "TYPE"                  : "uint16_t",
                                                "MULTI-PARAM"   : 0,
                                                "ADD-PARAM"             : 0
                                                }
                                        ]
                                },
                                "011" : {
                                        "DLC"   : 2 ,
                                        "MODE"  : "CAN_THINOUT_CHG",
                                        "CYCLE" : 200,
                                        "DATA" : [
                                                {
                                                "POS"                   : 2 ,
                                                "OFFS"                  : 0 ,
                                                "LEN"                   : 16 ,
                                                "CUSTOM"                : false,
                                                "PROPERTY"              : "VehicleSpeed",
                                                "ZONE"                  : ["Zone::None"],
                                                "TYPE"                  : "uint16_t",
                                                "MULTI-PARAM"   : 0,
                                                "ADD-PARAM"             : 0
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

