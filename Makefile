ifneq ($(MAKEBUILDTYPE),Release)
MAKEBUILDTYPE=Debug
endif

BASE_BUILD_DIR=build

ifeq ($(MAKEBUILDTYPE),Release)
BUILDDIR=$(BASE_BUILD_DIR)/release
else
BUILDDIR=$(BASE_BUILD_DIR)/debug
endif

BINDIR=$(BUILDDIR)/bin
ISODIR=$(BUILDDIR)/iso

SCRIPTS_DIR=scripts

TOOLSDIR=tools

AS=i686-linux-gnu-as
ASFLAGS=

CC=i686-linux-gnu-gcc
CFLAGS=-ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -nostdinc
CFLAGS+=-Wall -Wextra

ifeq ($(MAKEBUILDTYPE),Release)
CFLAGS+=-Werror -DNDEBUG
endif

CFLAGS+=-I./include -I./lib/libk -I./lib/data_structs -I./lib/libutils

AR=i686-linux-gnu-ar

LD=$(CC)
LDFLAGS=-z noexecstack -nostdlib -nodefaultlibs -static

ifeq ($(MAKEBUILDTYPE),Release)
LDFLAGS+=-s
endif

LDLIBS=-L./lib/libk -lk -L./lib/data_structs -lds -L./lib/libutils -lutils

QEMU=qemu-system-i386
QEMUFLAGS=-m 4G -smp 4 -cpu host -enable-kvm -net nic -net user -s -daemonize

DOCKERIMAGENAME=noalexan/cross-compiler
DOCKERIMAGETAG=ubuntu

OBJ=$(patsubst src/%,$(BINDIR)/%,$(shell find src -regex '.*\(\.c\|\.cpp\|\.s\)' -not -path "src/generated/*" | sed 's/\(\.c\|\.cpp\|\.s\)/.o/g'))
OBJ+=$(BINDIR)/generated/syscall_table.o

$(BINDIR)/%.o: src/%.s
	@mkdir -pv $(@D)
	$(AS) $(ASFLAGS) -o $@ $<

$(BINDIR)/%.o: src/%.c
	@mkdir -pv $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

define docker_run
	docker run --rm -t --user $(shell id -u):$(shell id -g) -v .:/kfs -e IN_DOCKER=1 -e MAKEBUILDTYPE="$(MAKEBUILDTYPE)" $(DOCKERIMAGENAME):$(DOCKERIMAGETAG) $(1)
endef

ifeq ($(IN_DOCKER),1)

.PHONY: all
all: $(BUILDDIR)/boot.iso

.PHONY: format
format:
	@clang-format --verbose --Werror -i $(shell find src include -regex '.*\.\(c\|h\|cpp\|hpp\)' -not \( -path 'src/generated/*' -o -path 'include/generated/*' \))

.PHONY: gen_systable
gen_systable:
	python3 $(SCRIPTS_DIR)/generate_table.py

.PHONY: clean
clean:
	$(RM) -r $(BUILDDIR)
	$(RM) -r src/generated
	$(RM) -r include/generated
	@make -C lib/libk clean
	@make -C lib/data_structs clean
	@make -C lib/libutils clean

$(BUILDDIR)/boot.iso: $(ISODIR)/boot/kernel $(ISODIR)/boot/grub/grub.cfg
	@mkdir -pv $(@D)
	grub-mkrescue -o $@ $(ISODIR)

$(ISODIR)/boot/grub/grub.cfg: grub.cfg
	@mkdir -pv $(@D)
	cp grub.cfg $@

$(ISODIR)/boot/kernel: gen_systable $(OBJ) linker.ld | libs
	@mkdir -pv $(@D)
	$(LD) -T linker.ld $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

.PHONY: libs
libs:
	@make -C lib/libk CC=$(CC) AR=$(AR)
	@make -C lib/data_structs CC=$(CC) AR=$(AR)
	@make -C lib/libutils CC=$(CC) AR=$(AR)

else

.PHONY: all
all:
	$(call docker_run, all)

.PHONY: format
format:
	$(call docker_run, format)

.PHONY: clean
clean:
	$(call docker_run, clean)

endif

# did not work.
# Todo: rework.
.PHONY: setup-dev
setup-dev:
	@mkdir -vp $(TOOLSDIR)
	@pip install --target $(TOOLSDIR) pre-commit
	@PYTHONPATH="$(TOOLSDIR):$$PYTHONPATH" python -m pre_commit install

.PHONY: doxy
doxy:
	doxygen Doxyfile

.PHONY: run
run: all
	$(QEMU) $(QEMUFLAGS) -cdrom $(BUILDDIR)/boot.iso

.PHONY: re
re: clean all

.NOTPARALLEL: all clean format
