import ipaddress
with open("address.txt", "r") as address, open("address_cpy.txt", "w") as out:
    for line in address:
        out.write(str(int(ipaddress.IPv4Address(line.strip()))) + "\n")
