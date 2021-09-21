## Using same Makefile given for Exercise 3

## [M1: point 1]
#  Explain following in here
#  name of the module is proj2
MODULE	 = proj2

## [M2: point 1]
#  Explain following in here
#  create an object file with proj2.o as file name
obj-m += $(MODULE).o

## [M3: point 1]
#  Explain following in here
#  get the kernel directory. shell command gives 5.13.0 on my system
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

## [M4: point 1]
#  Explain following in here
#  get the current working directory. In our case where the Makefile is located
PWD := $(shell pwd)

## [M5: point 1]
#  Explain following in here
#  get all the executables
all: $(MODULE)


## [M6: point 1]
#  Explain following in here
#  compile with gcc. $< is the first prereq and $@ is the name of the target
%.o: %.c
	@echo "  CC      $<"
	@$(CC) -c $< -o $@

## [M7: point 1]
#  Explain following in here
#  make the current working dir with kernel dir modules
$(MODULE):
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

## [M8: point 1]
#  Explain following in here
#  clean the modules after making them in the previous rule.
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
