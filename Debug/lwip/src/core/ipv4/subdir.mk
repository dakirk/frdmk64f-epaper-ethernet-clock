################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/src/core/ipv4/autoip.c \
../lwip/src/core/ipv4/dhcp.c \
../lwip/src/core/ipv4/etharp.c \
../lwip/src/core/ipv4/icmp.c \
../lwip/src/core/ipv4/igmp.c \
../lwip/src/core/ipv4/ip4.c \
../lwip/src/core/ipv4/ip4_addr.c \
../lwip/src/core/ipv4/ip4_frag.c 

OBJS += \
./lwip/src/core/ipv4/autoip.o \
./lwip/src/core/ipv4/dhcp.o \
./lwip/src/core/ipv4/etharp.o \
./lwip/src/core/ipv4/icmp.o \
./lwip/src/core/ipv4/igmp.o \
./lwip/src/core/ipv4/ip4.o \
./lwip/src/core/ipv4/ip4_addr.o \
./lwip/src/core/ipv4/ip4_frag.o 

C_DEPS += \
./lwip/src/core/ipv4/autoip.d \
./lwip/src/core/ipv4/dhcp.d \
./lwip/src/core/ipv4/etharp.d \
./lwip/src/core/ipv4/icmp.d \
./lwip/src/core/ipv4/igmp.d \
./lwip/src/core/ipv4/ip4.d \
./lwip/src/core/ipv4/ip4_addr.d \
./lwip/src/core/ipv4/ip4_frag.d 


# Each subdirectory must supply rules for building sources it contributes
lwip/src/core/ipv4/%.o: ../lwip/src/core/ipv4/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -DFRDM_K64F -DFREEDOM -DSERIAL_PORT_TYPE_UART=1 -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -DFSL_RTOS_BM -DSDK_OS_BAREMETAL -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/CMSIS" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/utilities" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/drivers" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/board" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/component/lists" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip/apps" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/port" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/source" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/component/serial_manager" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/port/arch" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix/arpa" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix/net" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix/sys" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/stdc" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip/priv" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip/prot" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/netif" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/netif/ppp" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/netif/ppp/polarssl" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/device" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/component/uart" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/apps" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/apps" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/component/lists" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/CMSIS" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/drivers" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/component/uart" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/device" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/utilities" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/component/serial_manager" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/board" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/source" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/port/arch" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix/arpa" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix/net" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/posix/sys" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/compat/stdc" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip/priv" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip/prot" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/netif" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/netif/ppp" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/netif/ppp/polarssl" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/port" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include/lwip/apps" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm/lwip/src/include" -I"/Users/David/Desktop/College_Stuff/EC544_Projects/Workspace/frdmk64f_lwip_dhcp_bm" -O0 -fno-common -g3 -Wall -c  -ffunction-sections  -fdata-sections  -ffreestanding  -fno-builtin -fmerge-constants -fmacro-prefix-map="../$(@D)/"=. -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


