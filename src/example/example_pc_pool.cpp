/*
 * FileName : example_pc_pool.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : 2017-06-25    Created
*/

#include <libpc/pc.h>
#include <iostream>

int main(int argc, char **argv)
{
    PcPool *pc_pool_obj = PcPool::get_instance();
    pc_pool_obj->init(10);
    
    for (int i = 0; i < 15; i++) {
        PC []() {
            std::cout << "pc lambda ok..." << std::endl;
        };
    }

    std::cout << "pc used :" << PcPool::get_instance()->get_uesd() << std::endl;

    for (int i = 0; i < 10; i++) {
        PC []() {
            std::cout << "pc lambda ok..." << std::endl;
        };
    }

    pc_eventloop(pc_get_epoll_ct(), 0, 0);

    return 0;
}
