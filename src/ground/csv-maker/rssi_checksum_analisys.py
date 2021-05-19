import json
import pandas as pd
import numpy as np
from scipy.stats import gaussian_kde
import matplotlib.pyplot as plt
from pprint import pprint

def flatten_json(df):
	json_struct = json.loads(df.to_json(orient="records"))
	df_flat = pd.json_normalize(json_struct)
	return df_flat

def fix_rssi_pkt(value):
	if value < 0:
		value = value+256
	return -value/2

df1 = pd.read_json("its-broker-log-20210519T170033_mdt_csv/radio.downlink_frame.json")
df1 = flatten_json(df1)
df1.set_index("data.cookie")
df1 = df1[["time", "data.cookie", "data.checksum_valid"]]

df2 = pd.read_json("its-broker-log-20210519T170033_mdt_csv/radio.rssi_packet.json")
df2 = flatten_json(df2)
df2 = df2[["data.cookie", "data.rssi_pkt", "data.rssi_signal", "data.snr_pkt"]]
df2["data.rssi_pkt2"] = [fix_rssi_pkt(x) for x in df2["data.rssi_pkt"]]
df2.set_index("data.cookie")

df = df1.join(df2, on="data.cookie", lsuffix=".df1", rsuffix=".df2")
df = df[0:-1]
fig, axs = plt.subplots(3)

x = range(0, df.shape[0])
y = df["data.rssi_pkt2"]
xy = np.vstack([x, y])
for no, a in enumerate(y):
	if a != a:
		print(no, a)
z = gaussian_kde(xy)(xy)
axs[0].scatter(x, y, c=z, s=100)

x = range(0, df.shape[0])
y = [1 if x else 0 for x in df["data.checksum_valid"]]
xy = np.vstack([x, y])
z = gaussian_kde(xy)(xy)
axs[1].scatter(x, y, c=z, s=100)


df["bin"] = pd.cut(df["data.rssi_pkt2"], bins=100)
df_bar = df[["data.rssi_pkt2", "bin"]].groupby("bin").count()
axs[2].bar(range(0, df_bar.shape[0]), df_bar["data.rssi_pkt2"])

plt.show()
