import os
import sys

if not "ZEPHYR_SDK_INSTALL_DIR" in os.environ:
	print("ZEPHYR_SDK_INSTALL_DIR environment variable not set")
	sys.exit(1)
zephyr_dir = os.environ["ZEPHYR_SDK_INSTALL_DIR"]

search_string = "command"
payload = f" -DINTELLISENSE -I{zephyr_dir}/arm-zephyr-eabi/arm-zephyr-eabi/include/"

input = open("../build/compile_commands.json", "r+")

output = []

for line in input:
	if search_string in line:
		pos = line.find(" -I")
		line = line[:pos] + payload + line[pos:]
	output.append(line)
input.close()

outfile = open("../compile_commands_intellisense.json", "w")
outfile.writelines(output)
outfile.close()
