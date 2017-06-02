cmd_drivers/hello/hello.ko := or1k-elf-ld -r  -T ./scripts/module-common.lds --build-id  -o drivers/hello/hello.ko drivers/hello/hello.o drivers/hello/hello.mod.o
