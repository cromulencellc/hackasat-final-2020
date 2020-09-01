#!/usr/bin/python
#
# This script takes two files output from gimp and creates a header
# file suitable for use with the badge grahpics library.
# The two files are a raw bmp file from an indexed image 
#		Gimp->Image->Mode->Indexed
#		Gimp->File->Export As (Raw)
#	And the associated color palette 
#		Gimp->Windows->Dockable Dialogs->Palettes
#		Palettes->Palettes Menu->Import Palette (Image, 16 columns)
#		Palettes->Palettes Menu->Export As->Text File...
#
#	See scoreboard.c for example usage in badge code.


import sys
import struct 
import pdb

def usage():
	print("%s <raw bmp file> <palette file> <output prefix>\n" % sys.argv[0])


def read_palette(palette_data):
	palette = []
	for each in palette_data:
			palette.append(int(each[1:],16))
	print palette
	return palette 

def process_bmp(bmp_data, palette):
	new_image = []
	count = 0
	while( count < len(bmp_data)):
		color = struct.unpack_from(">H", bmp_data, count)
		color = color[0] / 256
		new_image.append(color)
		count+=2
	return new_image

def convert_rgb(color):
	color = (color / (2**19))*(2**11) +\
			(((color % (2**16))/(2**10))*(2**5)) +\
			((color % (2**8))/(2**3))
	return color

def output_image(image, outfile, name):
	count = 0
	outfile.write("static uint16_t " + name + "_palette[] = {\n\t")
	for each in palette:
		outfile.write(hex(convert_rgb(each)))
		outfile.write(", \n\t")
	outfile.write("};\n\n")
	outfile.write("static unsigned char " + name + "_bitmap[] = {\n\t")
	while(count < len(new_image)):
		outfile.write(hex(image[count]*16 + image[count+1]))
		outfile.write(", ")
		count += 2
		if (count % 16 == 0):
			outfile.write("\n\t")

	outfile.write("};\n")
	outfile.close()

if __name__ == "__main__":
	if len(sys.argv) != 4:
		usage()
		exit()

	try:
		bmp_file = open(sys.argv[1], "rb")
	except IOError:
		print("Cannot open bmp file: %s" % sys.argv[1])
		exit()

	try:
		palette_file = open(sys.argv[2], "rb")
	except IOError:
		print("Cannot open palette file: %s" % sys.argv[2])
		exit()

	try:
		out_file = open(sys.argv[3]+".h", "wb")
	except IOError:
		print("Cannot open output file: %s" % sys.argv[3])
		exit()


	palette = read_palette(palette_file.readlines())
	new_image = process_bmp(bmp_file.read(), palette)
	output_image(new_image, out_file, sys.argv[3])

	
