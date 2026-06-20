NAME=libutils.a

ROOT_DIR=../..

KERNEL_INCLUDE_DIR = -I$(ROOT_DIR)/include

CFLAGS ?= -ffreestanding -fno-builtin -fno-exceptions -fno-stack-protector -Wall -Wextra -Werror
CFLAGS += $(KERNEL_INCLUDE_DIR)

OBJ=\
	ft_calloc.o			\
	ft_free_strs.o		\
	ft_join_and_free.o	\
	ft_strjoin.o		\
	ft_substr.o			\
	ft_itoa_base.o		\
	ft_split.o  		\
	ft_strmapi.o		\
	ft_itoa.o			\
	ft_strdup.o  		\
	ft_strtrim.o		

.PHONY: all
all: $(NAME)

$(NAME): $(OBJ)
	$(AR) rcs $@ $(OBJ)

.PHONY: clean
clean:
	@$(RM) $(OBJ)

.PHONY: fclean
fclean: clean
	@$(RM) $(NAME)

.PHONY: re
re: fclean all

.NOTPARALLEL: re fclean
