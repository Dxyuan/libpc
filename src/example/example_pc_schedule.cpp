/*
 * FileName : example_pc_schedule.cpp
 * Author   : Pengcheng Liu(Lpc-Win32)
 * Date     : Thu 29 Jun 2017 11:05:17 PM CST   Created
*/

#include <libpc/pc.h>
#include <iostream>

int main(int argc, char **argv)
{
    int ret = PcSchedule::get_instance()->init(16);
    if (ret == -1) {
        std::cout << "pc schedule init error" << std::endl;
        return -1;
    }

    PC []() {
        for (int i = 0; i < 100; i ++) {
            PC []() {
                std::cout << "pc run ok" << std::endl;
            };
        }
    };

    std::cout << "over PC" << std::endl;

    ret = PcSchedule::get_instance()->start();
    if (ret == -1) {
        std::cout << "start error" << std::endl;
        return -1;
    }

    return 0;
}
