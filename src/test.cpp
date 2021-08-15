#include "Parser.hpp"
#include <tr1/regex>
int main(int argc, char *argv[])
{
    std::vector<ServerBlock> out;
    if (argv[1])
    {
       Parser parser(argv[1]);
       std::cout << parser.getfilename("www.example.org",80,"/1.html") << std::endl;
    }
    return (0);
}