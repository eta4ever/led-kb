################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
ASM_SRCS += \
../usbdrv/usbdrvasm.asm 

C_SRCS += \
../usbdrv/oddebug.c \
../usbdrv/usbdrv.c 

S_UPPER_SRCS += \
../usbdrv/usbdrvasm.S 

OBJS += \
./usbdrv/oddebug.o \
./usbdrv/usbdrv.o \
./usbdrv/usbdrvasm.o 

ASM_DEPS += \
./usbdrv/usbdrvasm.d 

S_UPPER_DEPS += \
./usbdrv/usbdrvasm.d 

C_DEPS += \
./usbdrv/oddebug.d \
./usbdrv/usbdrv.d 


# Each subdirectory must supply rules for building sources it contributes
usbdrv/%.o: ../usbdrv/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Compiler'
	avr-gcc -I"C:\workspace\midi\usbdrv" -I/usr/lib/avr/include -Wall -g2 -gstabs -O0 -fpack-struct -fshort-enums -ffunction-sections -fdata-sections -std=gnu99 -funsigned-char -funsigned-bitfields -mmcu=atmega8 -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

usbdrv/%.o: ../usbdrv/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Assembler'
	avr-gcc -x assembler-with-cpp -I"C:\workspace\midi\usbdrv" -I/usr/lib/avr/include -g2 -gstabs -mmcu=atmega8 -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

usbdrv/%.o: ../usbdrv/%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: AVR Assembler'
	avr-gcc -x assembler-with-cpp -I"C:\workspace\midi\usbdrv" -I/usr/lib/avr/include -g2 -gstabs -mmcu=atmega8 -DF_CPU=16000000UL -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


