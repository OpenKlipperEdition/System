# Audio audio subsystem

## Module Function Introduction

Audio Interface Controller (AIC) supports I2S or IIS, a protocol defined by Philips Semiconductors. AIC supports normal I2S and MSB adjusted I2S formats. The AIC consists of buffers, status registers, control registers, serializers, and counters, and is used to transfer digitized audio between the processor's system memory (external I2S CODEC). The AIC may record the digitized audio by storing the samples in system memory. For playback of digitized audio or generation of synthesized audio, the AIC retrieves digitized audio samples from system memory and sends them to the CODEC over a serial connection in I2S format. An internal digital-to-analog converter in the CODEC then converts the audio samples into analog audio waveforms. The audio sample data may be stored to or retrieved from system memory via a DMA controller or via programmed I/O. Among them, ordinary I2S and MSB-justified-I2S operate at various clock rates. These clock rates can be obtained by dividing the PLL clock by two programmable dividers, or from an internal clock source.

## Drive source code location

Audio controller source code location:

***module_drivers/sound/soc/ingenic/as-v1/***

Board source code location:

***module_drivers/sound/soc/ingenic/board/x2660_halley.c***

Internal codec source code location:

***module_drivers/sound/soc/ingenic/icodec/icdc_inno_v3.c***

## Device tree configuration

Location of device tree:

***module_drivers/dts/x2600.dtsi***

Device tree description:

```c
aic: aic@0x10020000 {

    compatible = "ingenic,x2600-aic";

    reg = <0x10020000 0x100>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_AUDIO>;

    dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_AIC_TX)>,

    <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_AIC_RX)>;

    dma-names = "tx", "rx";

    i2s: i2s {

        compatible = "ingenic,x2600-i2s";

        status = "ok";

    };

    i2s_tloop: i2s_tloop {

        compatible = "ingenic,i2s-tloop";

        dmas = <&pdma INGENIC_DMA_TYPE(INGENIC_DMA_REQ_AIC_LOOP_RX)>;

        dma-names = "rx";

        status = "ok";

    };

};

icodec: icodec@0x10021000 {

    compatible = "ingenic,x2600-icodec";

    reg = <0x10021000 0x140>;

    status = "okay";

};

dmic: dmic@0x134c0000 {

    compatible = "ingenic,x2600-dmic";

    reg = <0x134c0000 0xa010>;

    interrupt-parent = <&core_intc>;

    interrupts = <IRQ_DMIC>;

    ingenic,fifo-size = <4096>;

    ingenic,fth_quirk;

    status = "disable";

};
```

### Default configuration of device tree

The default compilation of device tree will generate audio devices and configure them in ***module_drivers/dts/x2660_halley_v1.0.dts*** as follows:

```c
&aic {

    status = "okay";

    pinctrl-names = "default";

};

&dmic {

    status = "okay";

    pinctrl-names = "default";

    pinctrl-0 = <&dmic_pc_2ch>;

};

/{

    .......

    sound_x2660_cdc {

        status = "okay";

        compatible = "ingenic,x2660-halley-sound";

        ingenic,model = "x2660";

        ingenic,dai-link = "i2s-icodec", "i2s-tloop", "dmic";

        ingenic,stream = "i2s-icodec", "i2s-tloop", "dmic";

        ingenic,cpu-dai =  <&i2s>, <&i2s_tloop>, <&dmic>;

        ingenic,platform = <&aic>, <&i2s_tloop>, <&dmic>;

        ingenic,codec = <&icodec>, <&dump_pcm_codec>, <&dmic>;

        ingenic,codec-dai = "icodec", "pcm-dump", "dmic-dump-codec";

        ingenic,audio-routing = "Speaker", "HPOUTL", "DACL", "MICBIAS" ,

                "ADCL",  "MICL",

                "ADCR",  "MICR";

        ingenic,spken-gpio = <&gpe 5 GPIO_ACTIVE_HIGH INGENIC_GPIO_NOBIAS>;

    };

};
```

### Device tree custom configuration

Users can turn off audio devices according to actual needs or configure ingenic, spken-gpio attributes.

## Kernel compilation configuration

The kernel configuration for **SND_ASOC_INGENIC** is as follows:

```
Symbol: SND_ASOC_INGENIC [=y]

Type : tristate

Prompt: [AUDIO] ASoC support for Ingenic

Location:

-> Ingenic device-drivers Configurations

Defined at module_drivers/sound/soc/ingenic/Kconfig:1

Depends on: MACH_XBURST [=n] || MACH_XBURST2 [=y]

Selects: SND_SOC [=y] && SND [=y] && SOUND [=y]
```

Select board file:

```
Symbol: SND_ASOC_INGENIC_X2660_HALLEY [=y]

Type : boolean

Prompt: Audio support for x2660 halley_v10 board

Location:

-> Ingenic device-drivers Configurations

-> [AUDIO] ASoC support for Ingenic (SND_ASOC_INGENIC [=y])

-> Ingenic Board Type Select

-> SOC x2600/x2660 codec Type select (<choice> [=y])

Defined at module_drivers/sound/soc/ingenic/Kconfig:404

Depends on: <choice>

Selects: SND_ASOC_PDMA [=y] && SND_ASOC_INGENIC_AIC [=y] && \

SND_ASOC_INGENIC_AIC_I2S [=y] && SND_ASOC_INGENIC_MONO_RIGHT [=y] && \

SND_ASOC_INGENIC_DMIC_V2 [=y] && SND_ASOC_INGENIC_ICDC_INNO_V3 [=y] && \

SND_ASOC_INGENIC_DUMP_CODEC [=y] && SND_ASOC_INGENIC_AIC_I2S_TLOOP [=y]
```

### Default compile configuration of kernel

The default configuration of the audio driver in the kernel is as follows:

![](assets/Audio音频子系统.0.png)

![](assets/Audio音频子系统.1.png)

### Kernel custom compile configuration

Users can remove the configuration of this driver according to actual needs.

## Device Node Generation

After successful loading of the driver, the following nodes are generated:

```
ls /dev/snd/

controlC0 pcmC0D0c pcmC0D0p pcmC0D1c pcmC0D2c timer
```

## Application Instructions

### asound.conf introduction

The asound.conf configuration file is the default configuration file of the alsa-lib. The path is/etc/, which can be used to configure some additional functions of the alsa library. This file is not required for the alsa library to run, and the alsa library can run normally without it. Asound. conf allows for more advanced control of the sound card or device, providing access to the pcm plug-in method in the alsa-lib, allowing you to do more complex control, such as the sound card can be combined into one or more sound cards to access multiple I/O.

In ALSA, the PCM plugin extends the functionality and characteristics of the PCM device. The plugin can automatically handle tasks such as naming devices, sample rate conversion, copying samples between channels, writing to files, working with multiple input/output connections for sound cards/devices (asynchronous sampling), using multi-channel sound cards/devices, etc.

* 1. **hw plugin**

**This plugin communicates directly with the ALSA kernel driver, it is a raw communication without any conversion.**

This plug-in can be used to rename PCM devices.

For example:

```
# arecord -l

**** List of CAPTURE Hardware Devices ****

card 0: x2660 [x2660], device 0: i2s-icodec 10021000.icodec-0 []

  Subdevices: 1/1

  Subdevice #0: subdevice #0

card 0: x2660 [x2660], device 1: i2s-tloop dump_pcm_codec-1 []

  Subdevices: 1/1

  Subdevice #0: subdevice #0

card 0: x2660 [x2660], device 2: dmic 134c0000.dmic-2 []

  Subdevices: 1/1

  Subdevice #0: subdevice #0
```

When using arecord or aplay for recording and playing, the method of "hw:0,0" is generally used to specify the device used for recording and playing, which is not intuitive enough to use. You can define an alias for the device through the hw plug-in. For example:

```
pcm.amic {

    type hw

    card 0

    device 0

}
```

In this example, "hw: 0, 0" has been renamed and can be used in the following way during recording and playback:

```
arecord -D amic-c 2 -f S16_LE -r 16000 -d 5 baic0_16000-32-1.wav

aplay -D amic baic0_16000-32-1.wav
```

* 2. **slave plugin**

Subordinate plug-ins can be specified directly with a string, or entered in a composite configuration node. You can also specify some restrictions (such as static rate or channel count).

For example:

```
pcm_slave.slave_rate48000Hz {

    pcm "hw:0,0"

    rate 48000

}

pcm.rate48000Hz {

    type plug

    slave slave_rate48000Hz

}
```

or

```
pcm.rate48000Hz {

    type plug

    slave {

        pcm "hw:0,0"

        rate 48000

    }

}
```

The above example can be understood as: a virtual pcm device named rate48000Hz is defined, which actually uses the "hw:0,0" pcm device and is set to support only 48000Hz sampling rate. When using the rate48000Hz pcm device to record and play audio, only 48000 sampling rate is supported on the hardware, and fixed format bits and the number of wide channels can also be set.

* 3. **rate plugin**

The plugin converts the rate. The input and output formats must be linear, commonly used to play audio files that hardware does not support sample rate data width.

For example:

```
pcm.rate_16000 {

    type rate

    slave {

        pcm "hw:0,0"

        rate 16000

        format S8

    }

}
 ```

The plugin supports conversion of sampling rate and bit width, but not channel.

* 4. **plug plugin**

This plug-in can convert channels, rates and formats according to requirements. It has more functions of channel conversion than rate plug-in.

For example:

```
pcm.rate_48000 {

    type plug

    slave {

        pcm "hw:0,0"

        rate 16000

        format S8

        channels 1

    }

}
 ```

* 5. **Soft Volume**

This plugin is used to adjust software volume.

For example:

```
pcm.amic_record {

    type softvol

    slave {

        pcm "hw:0,0"

    }

    control {

        name "amic volume"

    }

    min_dB -51.0

    max_dB 30.0

}
```

The corresponding control will only be generated when using the softvol plugin for playback for the first time, that is, you need to execute "arecord -D amic_record" (for example as defined above) before adjusting volume through amixer.

### View playback device

```
# aplay -l

**** List of PLAYBACK Hardware Devices ****

card 0: x2660 [x2660], device 0: i2s-icodec 10021000.icodec-0 []

Subdevices: 1/1

Subdevice #0: subdevice #0
```

### View recording devices

```
# arecord -l

**** List of CAPTURE Hardware Devices ****

card 0: x2660 [x2660], device 0: i2s-icodec 10021000.icodec-0 []

  Subdevices: 1/1

  Subdevice #0: subdevice #0

card 0: x2660 [x2660], device 1: i2s-tloop dump_pcm_codec-1 []

  Subdevices: 1/1

  Subdevice #0: subdevice #0

card 0: x2660 [x2660], device 2: dmic 134c0000.dmic-2 []

  Subdevices: 1/1

  Subdevice #0: subdevice #0
```

### View audio regulators

```
# amixer contents

numid=2,iface=MIXER,name='Capture Volume'

  ; type=INTEGER,access=rw---R--,values=1,min=0,max=255,step=0

  : values=196

  | dBscale-min=-95.50dB,step=0.50dB,mute=0

numid=1,iface=MIXER,name='Playback Volume'

  ; type=INTEGER,access=rw---R--,values=1,min=0,max=255,step=0

  : values=226

  | dBscale-min=-121.00dB,step=0.50dB,mute=0

numid=3,iface=MIXER,name='DMIC Capture Volume'

  ; type=INTEGER,access=rw---R--,values=1,min=0,max=31,step=0

  : values=4

  | dBscale-min=0.00dB,step=3.00dB,mute=0

numid=4,iface=MIXER,name='DMIC High Pass Filter1 Switch'

  ; type=BOOLEAN,access=rw------,values=1

  : values=on

numid=5,iface=MIXER,name='DMIC High Pass Filter2 Switch'

  ; type=BOOLEAN,access=rw------,values=1

  : values=on

numid=6,iface=MIXER,name='DMIC Low Pass Filter Switch'

  ; type=BOOLEAN,access=rw------,values=1

  : values=on

numid=7,iface=MIXER,name='DMIC SW_LR Switch'

  ; type=BOOLEAN,access=rw------,values=1

  : values=off
```

### Adjust icodec recording and playback gain

```
amixer cset name='Capture Volume' 200

amixer cset name='Playback Volume' 200
```

### Adjust dmic recording gain

```
amixer cset name='DMIC Capture Volume' 20
```

### icodec recording

```
arecord -D hw:0,0 -c 1 -f S16_LE -r 16000 -d 10 icdc_16000-16-2.wav

arecord -D hw:0,0 -c 1 -f S16_LE -r 48000 -d 10 icdc_48000-16-4.wav
```

### dmic recording

```
arecord -D hw:0,2 -c 2 -f S16_LE -r 16000 -d 10 dmic_16000-16-2.wav

arecord -D hw:0,2 -c 2 -f S16_LE -r 48000 -d 10 dmic_48000-16-4.wav
```

### icodec playback

```
aplay -D hw:0,0 -c 1 -f S16_LE -r 16000 -d 10 aplay.wav
```

## Notes on use

For x2660 halley v1.0/v1.1 development board, DMIC recording pin and Bluetooth serial port UART1 share PC04, PC05 pins. The GPIO functions used by both are different, so they cannot be used at the same time.

For x2670 halley v1.0 development board, DMIC recording pin shares PC04 and PC05 pins with Bluetooth UART1; it also shares PC06 pin with WIFI VDDIO enable pin. The GPIO functions used by these three are different, so they cannot be used simultaneously.

For x2600e halley v1.0 development board, DMIC recording pin and WIFI VDDIO enable pin share PC06 pin. The GPIO functions used by them are different, so they cannot be used at the same time.

The X2660 only supports 1 and 2 channel dmic, while the X2670 and X2600E support 1, 2 and 4 channel dmic.
