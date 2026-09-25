NAME=libds.a

ROOT_DIR=../..

KERNEL_INCLUDE_DIR = -I$(ROOT_DIR)/include

CFLAGS ?= -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -Wall -Wextra -Werror
CFLAGS += $(KERNEL_INCLUDE_DIR) -I$(ROOT_DIR)/lib/libk

SRC=$(shell find . -name "*.c")
OBJ=$(SRC:.c=.o)

.PHONY: all
all: $(NAME)

$(NAME): $(OBJ)
	$(AR) rcs $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean
clean:
	@$(RM) $(OBJ)

.PHONY: fclean
fclean: clean
	@$(RM) $(NAME)

.PHONY: re
re: fclean all

.NOTPARALLEL: re fclean
