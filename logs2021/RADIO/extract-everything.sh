#!/usr/bin/bash

# Для запуска используется мавлинк из src/common/mavlink/venv/bin/activate
# И утилиты из src/ground/utility

# Клеим логи брокера в одно
# ./its_logfile_concat.py \
# 	-i its-broker-log-20211004T141353.zmq-log \
# 		its-broker-log-20211004T154432.zmq-log \
# 		its-broker-log-20211004T155515.zmq-log \
# 		its-broker-log-20211004T155629.zmq-log \
# 	-o its-broker-log-combined.zmq-log

# даунлинк с таймштампами
python3 make_mavlog.py \
	-i its-broker-log-combined.zmq-log \
	-o its-broker-log-combined-downlink.mav \
	--topic "radio.downlink_frame" \
	--use-mavlink

python3 make_csv.py \
	-i its-broker-log-combined-downlink.mav

# аплинк с таймштампами
python3 make_mavlog.py \
	-i its-broker-log-combined.zmq-log \
	-o its-broker-log-combined-uplink.mav \
	--topic "radio.uplink_frame" \
	--use-mavlink

python3 make_csv.py \
	-i its-broker-log-combined-uplink.mav 

# сдр с таймштампами
python3 make_mavlog.py \
	-i its-broker-log-combined.zmq-log \
	-o its-broker-log-combined-sdr.mav \
	--topic "sdr.downlink_frame" \
	--use-mavlink

python3 make_csv.py \
	-i its-broker-log-combined-sdr.mav 

# телеметрия антенны
python3 make_mavlog.py \
	-i its-broker-log-combined.zmq-log \
	-o its-broker-log-combined-antenna-tlm.mav \
	--topic "antenna.telemetry_packet" \
	--use-mavlink

python3 make_csv.py \
	-i its-broker-log-combined-antenna-tlm.mav

# комманды антенны
python3 make_mavlog.py \
	-i its-broker-log-combined.zmq-log \
	-o its-broker-log-combined-antenna-cmd.mav \
	--topic "antenna.command_packet" \
	--use-mavlink

python3 make_csv.py \
	-i its-broker-log-combined-antenna-cmd.mav 


# Метаданные из всех топиков на шине
python3 make_mdt.py --csv --json -i its-broker-log-combined.zmq-log
