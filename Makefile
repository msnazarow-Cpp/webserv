
.PHONY: all clean fclean re main bonus directories
export CXX CPPFLAGS MAKEFLAGS LDFLAGS INCLUDES
MKDIR_P 	:= mkdir -p
OUT_DIR 	:= obj
CXX 		:= clang++ -std=c++98 -O2
# $$ because $ is special Makefile symbol
IP 			:= $(shell\
				if command -v ifconfig > /dev/null; then \
                    ifconfig | grep 192 | cut -d' ' -f2; \
                else \
                  ip address | grep 192 | awk '{print $$2}' | cut -d'/' -f1; \
                fi)
CPPFLAGS 	:= -c -MMD -Wall -Wextra -Werror -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -Wno-unknown-pragmas -D IP=\"$(IP)\" # -Wno-unused-result
LDFLAGS 	:= -MMD -Wall -Wextra -Werror # -Wno-unused-result
DFLAGS 		:= '-O0 -g3'
ASFLAGS 	:= -fsanitize=address
ifeq ($(CXX), g++)
	HFLAGS		= '-pedantic -Wshadow -Wformat=2 -Wfloat-equal\
	-Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -Wcast-qual -Wcast-align\
	-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2\
	-fsanitize=undefined -fno-sanitize-recover -fstack-protector\
	-Wno-pointer-arith -Wno-cast-qual -Wimplicit-fallthrough'
else
	HFLAGS		= '-pedantic -Wshadow -Wformat=2 -Wfloat-equal\
	-Wshift-overflow -Wcast-qual -Wcast-align\
	-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2\
	-fsanitize=undefined -fno-sanitize-recover=all -fstack-protector\
	-Wno-pointer-arith -Wno-cast-qual -Wimplicit-fallthrough'
endif
MAKEFLAGS	= --no-print-directory
OBJ 		:= main.o Parser.o ServerBlock.o Location.o IndexHtmlMaker.o Client.o Server.o Exception.o File.o Port.o TextHolder.o
OBJ 		:= $(addprefix obj/,$(OBJ))
D_FILES 	= $(OBJ:.o=.d)
NAME 		= webserv
INCLUDES 	= -I$(PWD) -I$(PWD)/src
UNAME 		= $(shell uname)

ifeq ($(UNAME), Linux)
	CFLAGS += -D LINUX=1
endif

all: directories $(NAME)

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}
$(NAME): $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) $(INCLUDES) -o $(NAME)
obj/%.o : src/%.cpp
	$(CXX) $(CPPFLAGS) -c $(INCLUDES) $< -o $@
debug :
	@make CPPFLAGS+=$(DFLAGS) LDFLAGS+=$(DFLAGS)
debugas :
	@make debug CPPFLAGS+=$(ASFLAGS) LDFLAGS+=$(ASFLAGS)
debugh :
	@make debugas CPPFLAGS+=$(HFLAGS) LDFLAGS+=$(HFLAGS)
clean:
	rm -f *.o *.d
	rm -rf obj
fclean: clean
	rm -f $(NAME)
re%: fclean
	make $(patsubst re%, %, $@)
re: fclean all

-include $(D_FILES)