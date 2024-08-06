# ASUS Firmware Restoration 2.1.0.3

URL: https://dlcdnets.asus.com/pub/ASUS/wireless/GT-AX6000/Rescue_2103.zip

MD5: 6A734DAA8ABD9A01CBD921AA54C7DD76

# Firmwares

## Stock (ASUS RT-N11P B1)

URL: https://dlcdnets.asus.com/pub/ASUS/wireless/RT-N11P_B1/FW_RT_N11P_B1_300438010931.zip?model=RT-N11P-B1

MD5: 6BACCB82591E6617333A03841043EBF3

# Upgrade from Padavan to stock firmware

- Make sure the **Firmware_Stub** partition is present by typing: `cat /proc/mtd`
- Enable Telnet and SSH
- Verify that SSH enabled: `telnet 192.168.1.1 22`
- `ssh admin@192.168.1.1`
- `wget https://raw.githubusercontent.com/SmileyAG/asus_rt_n11p_b1/master/stock.trx -P /tmp/`
- Verify checksum: `md5sum /tmp/stock.trx`
- If valid, then `mtd_write -r write /tmp/stock.trx Firmware_Stub`
