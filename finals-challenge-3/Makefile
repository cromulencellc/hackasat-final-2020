all:
	sparc-rtems5-gcc -o build/implant.prom src/implant.S src/start.c src/implant.c src/sha512.c \
					-Wl,--gc-sections \
					-Wl,-Ttext=0x42800000 \
					-O2
					#-fpie \
	#sparc-rtems5-strip build/implant.prom
	sparc-rtems5-objcopy build/implant.prom build/implant.elf \
					--remove-section .interp \
					--remove-section .shstrtab \
					--remove-section .strtab
	sparc-rtems5-objcopy -O binary build/implant.elf build/implant.bin			
	# sparc-rtems5-gcc -o build/implant.bin implant.S start.c implant.c sha512.c \
	#				-O2 -fpie \
	#				-Wl,--gc-sections \
	#				-Wl,-Ttext=0x0
	#sparc-rtems5-strip build/implant.bin
	#sparc-rtems5-objcopy -O binary build/implant.bin build/implant.bin
	chmod 0777 build/* 

docker-build:
	docker run --rm -v /home/<user>/finals-chal3:/src rtems5:tools make
	python3 patch.py core-cpu3.prom build/implant.bin build/implant.prom

compress: compress.c
	gcc -g -o compress compress.c
