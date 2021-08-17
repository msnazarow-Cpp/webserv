
.PHONY: all clean fclean re main bonus
export CXX CPPFLAGS MAKEFLAGS LDFLAGS INCLUDES BONUS WITH_BONUS make
make 		= make
CXX 		= clang++ -std=c++98 -O2
CPPFLAGS 		= -c -MMD -Wall -Wextra -Werror -Wno-unused-result -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -Wno-unknown-pragmas
BFLAGS		= '-D BONUS=1'
LDFLAGS 	= -MMD -Wall -Wextra -Werror -Wno-unused-result
DFLAGS 		= '-O0 -g3'
ASFLAGS 	= -fsanitize=address
ifeq ($(CC), gcc)
HFLAGS		= '-pedantic -Wshadow -Wformat=2 -Wfloat-equal\
	-Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -Wcast-qual -Wcast-align\
	-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2\
	-fsanitize=undefined -fno-sanitize-recover -fstack-protector\
	-Wno-pointer-arith -Wno-cast-qual -Wno-unused-result'
else
HFLAGS		= '-pedantic -Wshadow -Wformat=2 -Wfloat-equal\
	-Wshift-overflow -Wcast-qual -Wcast-align\
	-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2\
	-fsanitize=undefined -fno-sanitize-recover=all -fstack-protector\
	-Wno-pointer-arith -Wno-cast-qual -Wno-unused-result'
endif
MAKEFLAGS	= --no-print-directory
OBJ 		:= main.o Parser.o ServerBlock.o Location.o
OBJ 		:= $(addprefix obj/,$(OBJ))
D_FILES 	= $(OBJ:.o=.d)
NAME 		= webserv
INCLUDES 	= -I$(PWD) -I$(PWD)/src
UNAME 		= $(shell uname)

ifeq ($(UNAME), Linux)
	CFLAGS += -D LINUX=1
endif

bonus:
	make make='make bonus' CPPFLAGS+=$(BFLAGS) LDFLAGS+=$(BFLAGS) all

all: DIR $(NAME)

DIR : 
	mkdir -p obj

$(NAME): $(OBJ) 
	$(CXX) $(LDFLAGS) $(OBJ) $(INCLUDES) -o $(NAME)
obj/%.o : src/%.cpp
	$(CXX) $(CPPFLAGS) -c $(INCLUDES) $< -o $@

debug :
	make bonus CPPFLAGS+=$(DFLAGS) LDFLAGS+=$(DFLAGS)
debugas :
	make debug CPPFLAGS+=$(ASFLAGS) LDFLAGS+=$(ASFLAGS)
debugh :
	make debugas CPPFLAGS+=$(HFLAGS) LDFLAGS+=$(HFLAGS)
clean:
	rm -f *.o *.d
	rm -rf obj

fclean: clean
	rm -f $(NAME)
re%: fclean
	make $(patsubst re%, %, $@)
re: fclean all
-include $(D_FILES)