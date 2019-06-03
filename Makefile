CC = g++

output: elf_utils.o il2cpp_utils.o il2CppDumper.o
	g++ elf_utils.o il2cpp_utils.o il2CppDumper.o -o dumper
	
elf_utils.o: elf_utils.cpp il2cpp.h
	$(CC) -c elf_utils.cpp -fpermissive

il2cpp_utils.o: il2cpp_utils.cpp il2cpp_utils.h il2cpp.h
	$(CC) -c il2cpp_utils.cpp

il2CppDumper.o: il2CppDumper.cpp il2cpp.h
	$(CC) -c il2CppDumper.cpp

clean:
	rm *.o dumper