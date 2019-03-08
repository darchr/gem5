#include "VeriilatorMemBlackBox.hh"
#include "base/Logging.hh"

//need to rewrite after changing memory.scala
VerilatorMemBlackBox::VerilatorMemBlackBox(
        VerilatorMemBlackBoxParams *params) :
    MemObject(params),
    instData(params -> name + ".instPort", this),
    dataPort(params -> name + ".dataPort", this)
{
}

