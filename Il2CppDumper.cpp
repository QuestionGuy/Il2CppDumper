// Il2CppDumper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "il2cpp.h"
#include "elf_structures.h"
#include "elf_utils.h"
#include "il2cpp_utils.h"
#include <map>
#include <string.h>

elf_header* pLibIl2Cpp = 0;

char* szTypeString[] =
{
	"END",
	"void",
	"bool",
	"char",
	"sbyte",// I1
	"byte",// U1
	"short",// I2
	"ushort",// U2
	"int",// I4
	"uint",// U4
	"long",// I8
	"ulong",// U8
	"float",// R4
	"double",// R8
	"String",
	"Ptr",
	"REF",
	"ValueType",
	"Class",
	"var",
	"array",
	"Generic",
	"TypedREF",
	"int",// I
	"uint",// U
	"FuncPtr",
	"Object",
	"sArray",
	"mvar",
	"cmod_reqd",
	"cmod_opt",
	"internal_type",
	"modifier",
	"sentinel",
	"pinned",
	"enum"
};

char* get_type_name(Il2CppType* pType, bool* isClass = 0)
{
	char* ret = "";
	if (pType->type == IL2CPP_TYPE_CLASS || pType->type == IL2CPP_TYPE_VALUETYPE)
	{
		Il2CppTypeDefinition* klass = GetTypeDefFromIndex(pType->data.klassIndex);
		ret = GetString(klass->nameIndex);
		if (isClass)
			*isClass = true;
	}
	else if (pType->type == IL2CPP_TYPE_GENERICINST)
	{
		Il2CppTypeDefinition* pMainDef = GetTypeDefFromIndex(pType->data.generic_class->typeDefinitionIndex);
		char* szNameRet = GetString(pMainDef->nameIndex);
		size_t stBaseNamelen = strlen(szNameRet);

		std::vector<char*> typeNames = std::vector<char*>();
		Il2CppGenericInst* pOriInst = pType->data.generic_class->context.class_inst;
		pType->data.generic_class->context.class_inst = (Il2CppGenericInst*)MapVATR((uint32_t)pType->data.generic_class->context.class_inst, pLibIl2Cpp);
		Il2CppGenericInst* pInst = pType->data.generic_class->context.class_inst;
		Il2CppType** pOriArgv = pInst->type_argv;
		pInst->type_argv = (Il2CppType**)MapVATR((uint32_t)pInst->type_argv, pLibIl2Cpp);
		for (int i = 0; i < pInst->type_argc; ++i)
		{
			Il2CppType* pOriType = pInst->type_argv[i];
			pInst->type_argv[i] = (Il2CppType*)MapVATR((uint32_t)pInst->type_argv[i], pLibIl2Cpp);
			typeNames.push_back(get_type_name(pInst->type_argv[i]));
			pInst->type_argv[i] = pOriType;
		}
		pInst->type_argv = pOriArgv;

		size_t stRetLen = stBaseNamelen;

		for (int i = 0; i < typeNames.size(); ++i)
			stRetLen += strlen(typeNames[i]) + 2;

		char* szRet = (char*)malloc(stRetLen + 1);
		memset(szRet, 0, stRetLen + 1);
		memcpy(szRet, szNameRet, stBaseNamelen);
		
		uint32_t uiOffset = stBaseNamelen + 1;
		for (int i = 0; i < typeNames.size(); ++i)
		{
			size_t stLen = strlen(typeNames[i]);
			memcpy(&szRet[uiOffset], typeNames[i], stLen);
			uiOffset += stLen;
			szRet[uiOffset++] = ',';
			szRet[uiOffset++] = ' ';
		}

		szRet[stBaseNamelen] = '<';
		szRet[stRetLen - 1] = '>';
		szRet[stRetLen] = 0;
		
		ret = szRet;
		if (isClass)
			*isClass = true;

		// So you're probably wondering Jumbo/r3ndom wtf is this?
		// Well the answer is caching is hard and I'm lazy so I unfix my fixes to fix them later so that I don't accidentally fix them twice.
		pType->data.generic_class->context.class_inst = pOriInst;
	}
	else
	{
		ret = szTypeString[pType->type];
		if (isClass)
			*isClass = false;
	}
	return ret;
}

void dump_method(MethodIndex idx)
{
	Il2CppMethodDefinition* pDef = GetMethodDefinition(idx);
	fprintf(stdout, "\t");
	Il2CppType* pReturnType = GetTypeFromTypeIndex(pDef->returnType);

	if ((pDef->flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK) == METHOD_ATTRIBUTE_PRIVATE)
		fprintf(stdout, "private ");
	if ((pDef->flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK) == METHOD_ATTRIBUTE_PUBLIC)
		fprintf(stdout, "public ");
	if (pDef->flags & METHOD_ATTRIBUTE_VIRTUAL)
		fprintf(stdout, "virtual ");
	//if (pDef->flags & METHOD_ATTRIBUTE_FINAL) // No idea what METHOD_ATTRIBUTE_FINAL means but I only see it on standard lib functions so ignoring for now.
	//	fprintf(stdout, "final ");
	if (pDef->flags & METHOD_ATTRIBUTE_STATIC)
		fprintf(stdout, "static ");

	fprintf(stdout, "%s %s(", get_type_name(pReturnType), GetString(pDef->nameIndex));
	for (int i = 0; i < pDef->parameterCount; ++i)
	{
		Il2CppParameterDefinition* pParam = GetParameterFromIndex(pDef->parameterStart + i);
		char* szParamName = GetString(pParam->nameIndex);
		Il2CppType* pType = GetTypeFromTypeIndex(pParam->typeIndex);
		char* szTypeName = get_type_name(pType);
		if (pType->attrs & PARAM_ATTRIBUTE_OPTIONAL)
			fprintf(stdout, "optional ");
		if (pType->attrs & PARAM_ATTRIBUTE_OUT)
			fprintf(stdout, "out ");
		if (i != pDef->parameterCount - 1)
		{
			fprintf(stdout, "%s %s, ", szTypeName, szParamName);
		}
		else
		{
			fprintf(stdout, "%s %s", szTypeName, szParamName);
		}
	}
	fprintf(stdout, "); // %x - %d\n", pCodeRegistration->methodPointers[pDef->methodIndex], pDef->methodIndex);
}

union MultiType
{
	float f;
	double d;
	int i;
	long long l;
	char* szString;
};

bool ReadMemBlob(Il2CppTypeEnum type, char* pBlob, MultiType* pOut)
{
	try
	{
		switch (type)
		{
		case IL2CPP_TYPE_BOOLEAN:
		case IL2CPP_TYPE_U1:
		case IL2CPP_TYPE_I1:
			pOut->i = *pBlob;
			break;
		case IL2CPP_TYPE_CHAR:
		case IL2CPP_TYPE_U2:
		case IL2CPP_TYPE_I2:
			pOut->i = *(uint16_t*)pBlob;
			break;
		case IL2CPP_TYPE_U4:
		case IL2CPP_TYPE_I4:
		case IL2CPP_TYPE_VALUETYPE:
			pOut->i = *(uint32_t*)pBlob;
			break;
		case IL2CPP_TYPE_U8:
		case IL2CPP_TYPE_I8:
			pOut->l = *(uint64_t*)pBlob;
			break;
		case IL2CPP_TYPE_R4:
			pOut->f = *(float*)pBlob;
			break;
		case IL2CPP_TYPE_R8:
			pOut->d = *(double*)pBlob;
			break;
		case IL2CPP_TYPE_STRING:
		{
			uint32_t uiLen = *(uint32_t*)pBlob;
			if (uiLen > 256) // To avoid dealing with an error in some early modules that have a super long string (202050057 chars) presumably an error.
				return false;
			char* pRes = (char*)malloc(uiLen * 2);
			memset(pRes, 0, uiLen * 2);
			memcpy(pRes, pBlob + sizeof(uint32_t), uiLen);
			pOut->szString = pRes;
			break;
		}
		default:
			return false;
		}
	}
	catch(void *e)
	{
		return false;
	}
	return true;
}

void dump_field(FieldIndex idx)
{
	Il2CppFieldDefinition* pField = GetFieldDefFromIndex(idx);
	Il2CppType* pType = GetTypeFromTypeIndex(pField->typeIndex);
	Il2CppFieldDefaultValue* pDefault = GetFieldDefaultFromIndex(idx);

	fprintf(stdout, "\t");
	if ((pType->attrs & FIELD_ATTRIBUTE_PRIVATE) == FIELD_ATTRIBUTE_PRIVATE)
		fprintf(stdout, "private ");
	if ((pType->attrs & FIELD_ATTRIBUTE_PUBLIC) == FIELD_ATTRIBUTE_PUBLIC)
		fprintf(stdout, "public ");
	if (pType->attrs & FIELD_ATTRIBUTE_STATIC)
		fprintf(stdout, "static ");
	//if (pType->attrs & FIELD_ATTRIBUTE_INIT_ONLY) // This works just not very useful information.
	//	fprintf(stdout, "readonly ");
	fprintf(stdout, "%s %s", get_type_name(pType), GetString(pField->nameIndex));
	if (pDefault && pDefault->dataIndex != -1)
	{
		char* pDef = GetDefaultValueFromIndex(pDefault->dataIndex);
		Il2CppType* pDefValType = GetTypeFromTypeIndex(pDefault->typeIndex);
		Il2CppType* pTypeToUse = pDefValType != 0 ? pDefValType : pType;
		MultiType multi;
		multi.l = 0;
		if (pDef)
		{
			if (ReadMemBlob(pTypeToUse->type, pDef, &multi))
			{
				switch (pTypeToUse->type)
				{
				case IL2CPP_TYPE_BOOLEAN:
					fprintf(stdout, " = %s", multi.i ? "true" : "false");
					break;
				case IL2CPP_TYPE_I1:
				case IL2CPP_TYPE_I2:
				case IL2CPP_TYPE_I4:
				case IL2CPP_TYPE_I8:
					fprintf(stdout, " = %ld", multi.l);
					break;
				case IL2CPP_TYPE_U1:
				case IL2CPP_TYPE_U2:
				case IL2CPP_TYPE_U4:
				case IL2CPP_TYPE_U8:
					fprintf(stdout, " = %lu", multi.l);
					break;
				case IL2CPP_TYPE_R4:
					fprintf(stdout, " = %f", multi.f);
					break;
				case IL2CPP_TYPE_R8:
					fprintf(stdout, " = %g", multi.d);
					break;
				case IL2CPP_TYPE_STRING:
					fprintf(stdout, " = @\"%s\"", multi.szString);
					free(multi.szString);
					break;
				default:
					break;
				}
			}
		}
	}
	if (!(pType->attrs & FIELD_ATTRIBUTE_STATIC))
		fprintf(stdout, "; // 0x%x\n", GetFieldOffsetFromIndex(idx));
	else
		fprintf(stdout, ";\n");
}

void dump_class(TypeDefinitionIndex idx)
{
	Il2CppTypeDefinition* pDef = GetTypeDefFromIndex(idx);
	fprintf(stdout, "// Namespace: %s\n", GetString(pDef->namespaceIndex));
	
	if (pDef->flags & TYPE_ATTRIBUTE_SERIALIZABLE)
		fprintf(stdout, "[Serializable]\n");

	if ((pDef->flags & TYPE_ATTRIBUTE_VISIBILITY_MASK) == TYPE_ATTRIBUTE_PUBLIC)
		fprintf(stdout, "public ");
	if (pDef->flags & TYPE_ATTRIBUTE_ABSTRACT)
		fprintf(stdout, "abstract ");
	if (pDef->flags & TYPE_ATTRIBUTE_SEALED)
		fprintf(stdout, "sealed ");
	
	if (pDef->flags & TYPE_ATTRIBUTE_INTERFACE)
		fprintf(stdout, "interface ");
	else
		fprintf(stdout, "class ");
	fprintf(stdout, "%s // TypeDefIndex: %d\n{\n", GetString(pDef->nameIndex), idx);

	
	fprintf(stdout, "\t// Fields\n");
	for (FieldIndex i = pDef->fieldStart; i < (pDef->fieldStart + pDef->field_count); ++i)
	{
		dump_field(i);
	}

	fprintf(stdout, "\t// Methods\n");
	for (MethodIndex i = pDef->methodStart; i < (pDef->methodStart + pDef->method_count); ++i)
	{
		dump_method(i);
	}

	fputs("}\n", stdout);
}

void FixTypeAddresses(Il2CppType* pType)
{
	if (pType->type == IL2CPP_TYPE_ARRAY
		|| pType->type == IL2CPP_TYPE_GENERICINST
		|| pType->type == IL2CPP_TYPE_BYREF
		|| pType->type == IL2CPP_TYPE_PTR
		|| pType->type == IL2CPP_TYPE_FNPTR)
	{
		pType->data.dummy = MapVATR((uint32_t)pType->data.dummy, pLibIl2Cpp);
	}
}

void LoadIl2CppLib(char* szFile)
{
	FILE* pFile = fopen(szFile, "rb");
	fseek(pFile, 0, SEEK_END);
	uint32_t uiSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	pLibIl2Cpp = (elf_header*)malloc(uiSize);
	fread((void*)pLibIl2Cpp, uiSize, 1, pFile);
	fclose(pFile);

	if (pLibIl2Cpp->m_dwFormat != 0x464c457f)
	{
		fprintf(stderr, "ERROR: il2cpp lib provided is not a valid ELF file.\n");
		exit(1);
	}
	
	// Someone asked me about these two constants so I thought I should document what they are.
	// These two pointers are the first two arguments passed to il2cpp::vm::MetadataCache::Register in the libil2cpp.so binary.
	// Updating them manually should be fairly trivial, just find where il2cpp::vm::MetadataCache::Register is called and use the first two args for \
	  code and metadata respectively.
	pCodeRegistration = (Il2CppCodeRegistration*)MapVATR(0x14860F8, pLibIl2Cpp);
	pMetadataRegistration = (Il2CppMetadataRegistration*)MapVATR(0x14A7F78, pLibIl2Cpp);

	// Fixes so that the code in il2cpp_utils.cpp and dumping funcstions above works...
	pCodeRegistration->methodPointers = (uint32_t*)MapVATR((uint32_t)pCodeRegistration->methodPointers, pLibIl2Cpp);

	pMetadataRegistration->fieldOffsets = (int32_t*)MapVATR((uint32_t)pMetadataRegistration->fieldOffsets, pLibIl2Cpp);
	pMetadataRegistration->typeDefinitionsSizes = (Il2CppTypeDefinitionSizes*)MapVATR((uint32_t)pMetadataRegistration->typeDefinitionsSizes, pLibIl2Cpp);
	pMetadataRegistration->types = (Il2CppType**)MapVATR((uint32_t)pMetadataRegistration->types, pLibIl2Cpp);
	for (int i = 0; i < pMetadataRegistration->typesCount; ++i)
	{
		pMetadataRegistration->types[i] = (Il2CppType*)MapVATR((uint32_t)pMetadataRegistration->types[i], pLibIl2Cpp);
		FixTypeAddresses(pMetadataRegistration->types[i]);
	}
}

int main(uint16_t argc, char** argv)
{
	// Hardcoded cause I'm lazy
	LoadMetadata("global-metadata.dat");
	LoadIl2CppLib("libil2cpp.so");

	//for (uint32_t i = 0; i < pCodeRegistration->methodPointersCount; ++i)
	//	dump_method(i);

	for (uint32_t i = 0; i < uiNumTypes; ++i)
		dump_class(i);

    return 0;
}

