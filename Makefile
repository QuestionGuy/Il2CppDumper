CC = g++
OBJS = $(CC) -c -fpermissive -g

output: elf_utils.o il2cpp_utils.o Il2CppDumper.o
	g++ elf_utils.o il2cpp_utils.o Il2CppDumper.o -o dumper
	
elf_utils.o: elf_utils.cpp il2cpp.h
	$(OBJS) elf_utils.cpp

il2cpp_utils.o: il2cpp_utils.cpp il2cpp_utils.h il2cpp.h
	$(OBJS) -c il2cpp_utils.cpp

Il2CppDumper.o: Il2CppDumper.cpp il2cpp.h
	$(OBJS) -c Il2CppDumper.cpp -fpermissive

clean:
	rm *.o dumper