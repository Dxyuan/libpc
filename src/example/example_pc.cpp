/*
 * FileName : example_pc.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-14
*/

#include <libpc/pc.h>
#include <iostream>
#include <unistd.h>

void pc_func()
{
    poll(NULL, 0, 1000);
    std::cout << "pc_func() run ok..." << std::endl;
}

int main(void)
{
    PC pc_func;

    PC []() {
        std::cout << "lambda func() run ok..." << std::endl;
    };

    pc_eventloop(pc_get_epoll_ct(), 0, 0);

    return 0;
}
