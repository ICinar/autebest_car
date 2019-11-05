# syscalls_gen.sh
#
# Generate syscall stubs, headers and tables
#
# azuepke, 2013-05-24: initial

cd ../libsys/
../scripts/generate_syscall_stubs.sh ../kernel/syscalls.lst 32
cd ../kernel/include/
../../scripts/generate_syscall_header.sh ../syscalls.lst
cd ../src/
../../scripts/generate_syscall_table.sh ../syscalls.lst
cd ..
hg add ../libsys/sys_*.S
