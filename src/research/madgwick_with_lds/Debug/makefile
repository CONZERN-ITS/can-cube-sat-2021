################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include App/Src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
madgwick_with_lds.elf \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
madgwick_with_lds.list \

OBJCOPY_BIN += \
madgwick_with_lds.bin \


# All Target
all: madgwick_with_lds.elf secondary-outputs

# Tool invocations
madgwick_with_lds.elf: $(OBJS) $(USER_OBJS) /home/sereshotes/STM32CubeIDE/workspace_1.2.0/madgwick_with_lds/STM32F405RGTX_FLASH.ld
	arm-none-eabi-gcc -o "madgwick_with_lds.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"/home/sereshotes/STM32CubeIDE/workspace_1.2.0/madgwick_with_lds/STM32F405RGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="madgwick_with_lds.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

madgwick_with_lds.list: $(EXECUTABLES)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "madgwick_with_lds.list"
	@echo 'Finished building: $@'
	@echo ' '

madgwick_with_lds.bin: $(EXECUTABLES)
	arm-none-eabi-objcopy  -O binary $(EXECUTABLES) "madgwick_with_lds.bin"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) *
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST) $(OBJCOPY_BIN)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
