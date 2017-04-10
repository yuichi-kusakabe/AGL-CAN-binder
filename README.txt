【Build方法】
  Yoctoに組み込まず(提供meta先未定)
  caniviをcmake する場合、MAKE_TOOL_CHAIN_FILEにて クロス環境を指定する。

    $ PATH=/home/Devel/AGL/RCarGen3-agl/build/tmp/sysroots/x86_64-linux/usr/bin/aarch64-agl-linux:$PATH
	$ mkdir build
	$ cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=../RcarGen3.cmake ..
	$ make
	# make install  ※ <wgt化は、installで作成する予定>

【起動方法】
1) ambd が起動している場合、ambdを停止する。

	systemctl stop ambd

2) canivi-Config設定
  # can I/Fデバイスの設定。

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


