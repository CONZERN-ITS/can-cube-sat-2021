import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_json("its-broker-log-20210519T170033_mdt_csv/radio.downlink_frame.json")
first = df["time"][0]
df["time"] = [x - first for x in df["time"]]
df["minutes"] = df["time"] / 60
df["bad_checksum"] = [not x["checksum_valid"] for x in df["data"]]
df["bin"] = pd.cut(df["time"], bins=50)
#del df["data"]
print(df.head())

all = df.groupby("bin")["bin", "bad_checksum"].count()
bad = df[df["bad_checksum"] == True].groupby("bin")["bin", "bad_checksum"].count()
print(bad)

fig, axs = plt.subplots(2)
axs[0].bar(range(0, all.shape[0]), all["bad_checksum"])
axs[0].bar(range(0, bad.shape[0]), bad["bad_checksum"])

axs[1].bar(range(0, bad.shape[0]), bad["bad_checksum"] / all["bad_checksum"] * 100)

plt.show()
