#-------------------------------------------------
# FileName : install.sh
# Author   : Pengcheng Liu(Lpc-Win32)
# Date     : 2017-06-13
#------------------------------------------------

#!/bin/sh

set -x

cd `dirname $0`

VERSION=`git tag | tail -1`
LIBPCDIR=`pwd`
LIBPCLIBPATH="/usr/local/lib"
LIBPCINCLUDEPATH="/usr/local/include/libpc"

# Delete old libpc head files
rm -f $LIBPCINCLUDEPATH/*.h

mkdir -p $LIBPCDIR/build
cd $LIBPCDIR/build

# TODO (This build func is too low) Build libpc
gcc -c -o pc_ctx_swap.o $LIBPCDIR/src/pc_ctx_swap.S
g++ -g -std=c++11 -Wall -c -o pc_epoll.o $LIBPCDIR/src/pc_epoll.cpp
g++ -g -std=c++11 -Wall -c -o pc_routine.o $LIBPCDIR/src/pc_routine.cpp
g++ -g -std=c++11 -Wall -c -o pc_ctx.o $LIBPCDIR/src/pc_ctx.cpp
g++ -g -std=c++11 -Wall -c -o pc_hook_sys_call.o $LIBPCDIR/src/pc_hook_sys_call.cpp
g++ -g -std=c++11 -Wall -c -o pc_pool.o $LIBPCDIR/src/pc_pool.cpp
g++ -g -std=c++11 -Wall -c -o pc_schedule.o $LIBPCDIR/src/pc_schedule.cpp
g++ -g -std=c++11 -Wall -c -o pc_logger.o $LIBPCDIR/src/pc_logger.cpp
g++ -g -std=c++11 -Wall -c -o pc.o $LIBPCDIR/src/pc.cpp
ar -rc libpc.a pc_epoll.o pc_routine.o pc_hook_sys_call.o pc_ctx_swap.o pc_ctx.o pc_pool.o pc_schedule.o pc_logger.o pc.o

mkdir -p $LIBPCINCLUDEPATH
cp -f $LIBPCDIR/src/*.h $LIBPCINCLUDEPATH
cp -f $LIBPCDIR/build/libpc.a $LIBPCLIBPATH
ldconfig

cd $LIBPCDIR
rm -rf build

# Record version of libpc
echo "#!/bin/sh" > libpc_version
echo "echo ${VERSION}" >> libpc_version
chmod +x libpc_version
mv -f libpc_version /usr/local/bin/libpc_version

echo "libpc install ok"
