#include "Error.h"

Error::Error( string cmnt, int mo, int smo, int errn, int prio):module(mo),submodule(smo),errornumber(errn),priority(prio),comment(cmnt)
{
    time(&ntime);
    stime = ctime(&ntime);
}

Error::~Error()
{
    //dtor
}

void Error::what() const noexcept
{
    std::cout<<"error occured at: "<<stime<<std::endl;
    std::cout<<comment<<std::endl;
}
