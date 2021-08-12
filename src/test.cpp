#include "Parser.h"

int main(int argc, char *argv[])
{
    if (argv[1])
    {
        Parser::parse(argv[1]);
    }
}