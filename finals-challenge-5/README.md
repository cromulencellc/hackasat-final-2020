# Finals Challenge 4 and Challenge 5

Take picture with pixy2. Send to Cromulence C&DH through I2C. This repo builds the final kubos-linux.img image.

This repo also handles the Kubos image for challenge 4. If you tell it to, it will build the final 
image with the corrupted bootloader 

## What the Heck is Going On?

The main files that go into the actual final image are in `/overlay`. Within overlay we have the folder `init.d`. Whatever scripts are in this folder will be added to the `init.d` folder of the final image and will run on startup.

`mission-apps` are the registered mission apps in Kubos. There is a script called `initpayload` that will create the schedule `imager` and register all the apps needed to run the payload.

The `keep_alive` app was going to be a registered app that ensured the payload never died. In the final image, this was not used since it caused the original script to run too slowly and get out of sync with the FPGA.

`leon3_i2c` is where the actual imaging and I2C magic happened.

If you choose the corrupted bootloader option it will pull our custom u-boot repo with the bad u-boot enviroment variables. That's how the final image gets corrupted.

## Usage

**Initial use:**

*Say yes to prompt asking to update mission-apps. Say yes to the prompt asking to update to the
corrupted bootloader config as well if you'd like to create the image so that challenge 4 is also included.*
<br/>
*Note: Start-script will download files that are decently large.*
<br/><br/>
`./startscript.sh`<br/>
`make build`<br/>
`make run`<br/>

**After modifying mission-apps:**

`./startscript.sh`<br/>
`make run`

**Modify packages in final image:**

`cd /kubos-linux/buildroot-2019.02.2`<br/>
`make menuconfig`

**Build final kubos image:**

`cd /kubos-linux/buildroot-2019.02.2`<br/>
`make`

**Cleaning buildroot**

If too many changes are made, sometimes a full rebuild is needed. In this case
you must clean the build first before you make again.

`cd /kubos-linux/buildroot-2019.02.2`<br/>
`make clean`

## Notes

While running the startscript you will be asked if you would like to update the config for the bootloader so that it is corrupted. This is for challenge 4. The only difference between the config files is that one pulls our uboot repo that has the incorrect bootloader enviroment variables. This means when the final image is created, the bootloader that is flashed will be corrupted. If you'd like to test challenge 5 quickly, say no to this option.

***If this is the build for the final image for challenge 4 as well, make sure that you remove the kernel from the final .img file.***

The kernel can be restored from the .trash folder, if you do not use `rm`, however, if `rm` is used, the bootloader will automatically restore it's kernel from the image.

## Quick Solving Challenge 4

Press any key to exit autoboot. When given the `U-Boot>` prompt input the following:

```
setenv mmc_boot "mmc dev 0; fatload mmc 0:1 ${fdt_addr_r} bcm2708-rpi-zero.dtb; fatload mmc 0:1 ${kernel_addr_r} kernel; bootm ${kernel_addr_r} - ${fdt_addr_r}"
setenv kubos_curr_tried 0
saveenv
reset
```

Let it boot without any interuption. It will recover the kernel then boot into Kubos.