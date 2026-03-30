NAME=libk.a

ROOT_DIR=../..

KERNEL_INCLUDE_DIR = -I$(ROOT_DIR)/include

CFLAGS ?= -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -Wall -Wextra -Werror
CFLAGS += $(KERNEL_INCLUDE_DIR)

OBJ=\
	ft_atoi.o 		\
	ft_isalpha.o 	\
	ft_isdigit.o 	\
	ft_isalnum.o 	\
	ft_isascii.o 	\
	ft_isprint.o 	\
	ft_toupper.o 	\
	ft_tolower.o 	\
	ft_bzero.o 		\
	ft_memcmp.o 	\
	ft_memcpy.o 	\
	ft_memmove.o 	\
	ft_memchr.o 	\
	ft_memset.o 	\
	ft_strlen.o 	\
	ft_strchr.o 	\
	ft_strncmp.o 	\
	ft_strlcat.o 	\
	ft_strnstr.o 	\
	ft_strlcpy.o 	\
	ft_striteri.o 	\
	ft_strequ.o		\
	ft_strrchr.o

.PHONY: all
all: $(NAME)

$(NAME): $(OBJ)
	$(AR) rcs $@ $(OBJ)

.PHONY: clean
clean:
	@$(RM) -v $(OBJ)

.PHONY: fclean
fclean: clean
	@$(RM) -v $(NAME)

.PHONY: re
re: fclean all

.NOTPARALLEL: re fclean
