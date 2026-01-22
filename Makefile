NAME = ircserv

CPP_FILES = main.cpp\
			server.cpp\

CFLAGS = -Wall -Werror -Wextra -std=c++98 -g
# CFLAGS = clang -fsanitize=address -g -Wall -Werror -Wextra -std=c++98 -g

C_TYPE = c++

OBJ = $(CPP_FILES:.cpp=.o)

all: ${NAME}

${NAME}: ${OBJ}
	${C_TYPE} ${CFLAGS} ${OBJ} -o ${NAME}

%.o: %.cpp
	$(C_TYPE) $(CFLAGS) -c $< -o $@

clean:
	rm -f ${OBJ}

fclean:	clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re%