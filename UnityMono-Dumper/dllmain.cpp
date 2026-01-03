#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>

//--Mono Enums--//
enum MonoMethodAttribute {
    MONO_METHOD_ATTR_STATIC = 0x0010,
    MONO_METHOD_ATTR_FINAL = 0x0020,
    MONO_METHOD_ATTR_VIRTUAL = 0x0040,
    MONO_METHOD_ATTR_ABSTRACT = 0x0400,
    MONO_METHOD_ATTR_PINVOKE_IMPL = 0x2000,
};


enum MonoFieldAttribute {
    MONO_FIELD_ATTR_STATIC = 0x0010,
    MONO_FIELD_ATTR_INIT_ONLY = 0x0020,

    MONO_FIELD_ATTR_LITERAL = 0x0040,

};

//--API structs--//
typedef struct _MonoDomain MonoDomain;
typedef struct _MonoAssembly MonoAssembly;
typedef struct _MonoImage MonoImage;
typedef struct _MonoClass MonoClass;
typedef struct _MonoMethod MonoMethod;
typedef struct _MonoType MonoType;
typedef struct _MonoProperty MonoProperty;
typedef struct _MonoClassField MonoClassField;
typedef struct _MonoVTable MonoVTable;
typedef struct _MonoMethodSignature MonoMethodSignature;

//--API typedefs--//
typedef MonoDomain* (*mono_get_root_domain_t)();
typedef MonoDomain* (*mono_domain_get_t)();
typedef void* (*mono_thread_attach_t)(MonoDomain*);
typedef void* (*mono_assembly_foreach_t)(void(*)(MonoAssembly*, void*), void*);
typedef MonoImage* (*mono_assembly_get_image_t)(MonoAssembly*);
typedef const char* (*mono_image_get_name_t)(MonoImage*);
typedef int (*mono_image_get_table_rows_t)(MonoImage*, int);
typedef MonoClass* (*mono_class_get_t)(MonoImage*, unsigned int);
typedef const char* (*mono_class_get_name_t)(MonoClass*);
typedef const char* (*mono_class_get_namespace_t)(MonoClass*);
typedef MonoMethod* (*mono_class_get_methods_t)(MonoClass*, void**);
typedef const char* (*mono_method_get_name_t)(MonoMethod*);
typedef MonoMethodSignature* (*mono_method_signature_t)(MonoMethod*);
typedef MonoType* (*mono_signature_get_return_type_t)(MonoMethodSignature*);
typedef MonoType* (*mono_signature_get_params_t)(MonoMethodSignature*, void**);
typedef unsigned int (*mono_signature_get_param_count_t)(MonoMethodSignature*);
typedef char* (*mono_type_get_name_t)(MonoType*);
typedef char* (*mono_method_full_name_t)(MonoMethod*, int);
typedef MonoClassField* (*mono_class_get_fields_t)(MonoClass*, void**);
typedef const char* (*mono_field_get_name_t)(MonoClassField*);
typedef MonoType* (*mono_field_get_type_t)(MonoClassField*);
typedef unsigned int (*mono_field_get_flags_t)(MonoClassField*);
typedef int (*mono_field_get_offset_t)(MonoClassField*);
typedef unsigned int (*mono_method_get_flags_t)(MonoMethod*, unsigned int*);
typedef void* (*mono_compile_method_t)(MonoMethod*);
typedef MonoClass* (*mono_class_get_parent_t)(MonoClass*);
typedef int (*mono_type_get_type_t)(MonoType*);
typedef void (*mono_free_t)(void*);
typedef MonoVTable* (*mono_class_vtable_t)(MonoDomain*, MonoClass*);
typedef void* (*mono_vtable_get_static_field_data_t)(MonoVTable*);
typedef MonoType* (*mono_class_get_type_t)(MonoClass*);
typedef MonoProperty* (*mono_class_get_properties_t)(MonoClass*, void**);
typedef const char* (*mono_property_get_name_t)(MonoProperty*);
typedef MonoMethod* (*mono_property_get_get_method_t)(MonoProperty*);
typedef MonoMethod* (*mono_property_get_set_method_t)(MonoProperty*);
typedef MonoClass* (*mono_class_get_interfaces_t)(MonoClass*, void**);

struct MonoAPI {
    mono_get_root_domain_t get_root_domain;
    mono_domain_get_t domain_get;
    mono_thread_attach_t thread_attach;
    mono_assembly_foreach_t assembly_foreach;
    mono_assembly_get_image_t assembly_get_image;
    mono_image_get_name_t image_get_name;
    mono_image_get_table_rows_t image_get_table_rows;
    mono_class_get_t class_get;
    mono_class_get_name_t class_get_name;
    mono_class_get_namespace_t class_get_namespace;
    mono_class_get_methods_t class_get_methods;
    mono_method_get_name_t method_get_name;
    mono_method_signature_t method_signature;
    mono_signature_get_return_type_t signature_get_return_type;
    mono_signature_get_params_t signature_get_params;
    mono_signature_get_param_count_t signature_get_param_count;
    mono_type_get_name_t type_get_name;
    mono_method_full_name_t method_full_name;
    mono_class_get_fields_t class_get_fields;
    mono_field_get_name_t field_get_name;
    mono_field_get_type_t field_get_type;
    mono_field_get_flags_t field_get_flags;
    mono_field_get_offset_t field_get_offset;
    mono_method_get_flags_t method_get_flags;
    mono_compile_method_t compile_method;
    mono_class_get_parent_t class_get_parent;
    mono_type_get_type_t type_get_type;
    mono_free_t mono_free;
    mono_class_vtable_t class_vtable;
    mono_vtable_get_static_field_data_t vtable_get_static_field_data;
    mono_class_get_type_t class_get_type;
    mono_class_get_properties_t class_get_properties;
    mono_property_get_name_t property_get_name;
    mono_property_get_get_method_t property_get_get_method;
    mono_property_get_set_method_t property_get_set_method;
    mono_class_get_interfaces_t class_get_interfaces;
};

MonoAPI g_mono;
HMODULE g_monoModule = nullptr;
FILE* g_console = nullptr;

std::string IntToHex(uintptr_t value) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << value;
    return ss.str();
}

std::string addDllExtension(const std::string& name) {
    if (name.length() < 4 || (name.substr(name.length() - 4) != ".dll" && name.substr(name.length() - 4) != ".DLL")) {
        return name + ".dll";
    }
    return name;
}

bool InitMonoAPI() {
    const char* monoLibs[] = { "mono.dll", "mono-2.0-bdwgc.dll", "mono-2.0-sgen.dll", "mono-2.0.dll" }; //add the mono dlls here
    for (const char* lib : monoLibs) {
        g_monoModule = GetModuleHandleA(lib);
        if (g_monoModule) break;
    }
    if (!g_monoModule) return false;

#define LOAD_FUNC(name) g_mono.name = (mono_##name##_t)GetProcAddress(g_monoModule, "mono_" #name)

    LOAD_FUNC(get_root_domain);
    LOAD_FUNC(domain_get);
    LOAD_FUNC(thread_attach);
    LOAD_FUNC(assembly_foreach);
    LOAD_FUNC(assembly_get_image);
    LOAD_FUNC(image_get_name);
    LOAD_FUNC(image_get_table_rows);
    LOAD_FUNC(class_get);
    LOAD_FUNC(class_get_name);
    LOAD_FUNC(class_get_namespace);
    LOAD_FUNC(class_get_methods);
    LOAD_FUNC(method_get_name);
    LOAD_FUNC(method_signature);
    LOAD_FUNC(signature_get_return_type);
    LOAD_FUNC(signature_get_params);
    LOAD_FUNC(signature_get_param_count);
    LOAD_FUNC(type_get_name);
    LOAD_FUNC(method_full_name);
    LOAD_FUNC(class_get_fields);
    LOAD_FUNC(field_get_name);
    LOAD_FUNC(field_get_type);
    LOAD_FUNC(field_get_flags);
    LOAD_FUNC(field_get_offset);
    LOAD_FUNC(method_get_flags);
    LOAD_FUNC(compile_method);
    LOAD_FUNC(class_get_parent);
    LOAD_FUNC(type_get_type);
    LOAD_FUNC(class_vtable);
    LOAD_FUNC(vtable_get_static_field_data);
    LOAD_FUNC(class_get_type);
    LOAD_FUNC(class_get_properties);
    LOAD_FUNC(property_get_name);
    LOAD_FUNC(property_get_get_method);
    LOAD_FUNC(property_get_set_method);
    LOAD_FUNC(class_get_interfaces);
    g_mono.mono_free = (mono_free_t)GetProcAddress(g_monoModule, "mono_free");

    return g_mono.get_root_domain != nullptr;
}

std::string SanitizeString(const char* str) {
    if (!str) return "";
    std::string result = str;
    for (char& c : result) {
        if ((c < '0' || c > '9') && (c < 'a' || c > 'z') && (c < 'A' || c > 'Z') &&
            c != '_' && c != '.' && c != '<' && c != '>' && c != '[' && c != ']' && c != '`') {
            c = '_';
        }
    }
    return result;
}

std::string GetTypeName(MonoType* type) {
    if (!type || !g_mono.type_get_name) return "void";
    char* name = g_mono.type_get_name(type);
    if (!name) return "void";
    std::string result = name;
    if (g_mono.mono_free) g_mono.mono_free(name);

    if (result == "System.Void") return "void";
    if (result == "System.Int32") return "int";
    if (result == "System.UInt32") return "uint";
    if (result == "System.Int16") return "short";
    if (result == "System.UInt16") return "ushort";
    if (result == "System.Int64") return "long";
    if (result == "System.UInt64") return "ulong";
    if (result == "System.Single") return "float";
    if (result == "System.Double") return "double";
    if (result == "System.Boolean") return "bool";
    if (result == "System.Byte") return "byte";
    if (result == "System.SByte") return "sbyte";
    if (result == "System.Char") return "char";
    if (result == "System.String") return "string";
    if (result == "System.Object") return "object";
    if (result == "System.Decimal") return "decimal";

    size_t backtick = result.find('`');
    if (backtick != std::string::npos) {
        result = result.substr(0, backtick);
    }
    return SanitizeString(result.c_str());
}

std::string GetMethodSignature(MonoMethod* method) {
    if (!method || !g_mono.method_signature) return "()";
    MonoMethodSignature* sig = g_mono.method_signature(method);
    if (!sig) return "()";

    std::stringstream ss;
    ss << "(";
    if (g_mono.signature_get_param_count && g_mono.signature_get_params) {
        unsigned int paramCount = g_mono.signature_get_param_count(sig);
        void* iter = nullptr;
        for (unsigned int i = 0; i < paramCount; i++) {
            if (i > 0) ss << ", ";
            MonoType* paramType = g_mono.signature_get_params(sig, &iter);
            ss << GetTypeName(paramType) << " arg" << i;
        }
    }
    ss << ")";
    return ss.str();
}

uintptr_t CalculateVA(uintptr_t address) {
    if (!g_monoModule) return 0;
    return address - (uintptr_t)g_monoModule;
}

void DumpClass(MonoClass* klass, std::ofstream& output, const std::string& assemblyName) {
    if (!klass) return;
    const char* rawName = g_mono.class_get_name ? g_mono.class_get_name(klass) : nullptr;
    const char* rawNs = g_mono.class_get_namespace ? g_mono.class_get_namespace(klass) : nullptr;

    if (!rawName) return;
    std::string name = SanitizeString(rawName);
    std::string ns = rawNs ? SanitizeString(rawNs) : "";

    if (name.find("<") != std::string::npos || name.find("$") != std::string::npos) return;

    output << "// Dll : " << addDllExtension(assemblyName) << "\n";
    std::string indent = "";
    if (!ns.empty()) {
        output << "namespace " << ns << "\n{\n";
        indent = "\t";
    }

    output << indent << "public class " << name;
    bool hasParent = false;

    if (g_mono.class_get_parent) {
        MonoClass* parent = g_mono.class_get_parent(klass);
        if (parent && g_mono.class_get_name) {
            std::string pName = SanitizeString(g_mono.class_get_name(parent));
            if (pName != "Object" && pName != "ValueType") {
                output << " : " << pName;
                hasParent = true;
            }
        }
    }

    if (g_mono.class_get_interfaces) {
        void* iter = nullptr;
        MonoClass* iface;
        bool firstIface = !hasParent;
        while ((iface = g_mono.class_get_interfaces(klass, &iter))) {
            const char* iName = g_mono.class_get_name(iface);
            if (iName) {
                output << (firstIface ? " : " : ", ") << SanitizeString(iName);
                firstIface = false;
            }
        }
    }

    output << "\n" << indent << "{\n";
    std::string innerIndent = indent + "\t";

    if (g_mono.class_get_fields) {
        void* iter = nullptr;
        MonoClassField* field;
        bool headerPrinted = false;
        while ((field = g_mono.class_get_fields(klass, &iter))) {
            const char* fNameRaw = g_mono.field_get_name ? g_mono.field_get_name(field) : nullptr;
            if (!fNameRaw) continue;

            std::string fName = SanitizeString(fNameRaw);
            if (fName.find("<") != std::string::npos) continue;

            if (!headerPrinted) { output << innerIndent << "// Fields\n"; headerPrinted = true; }
            unsigned int flags = g_mono.field_get_flags ? g_mono.field_get_flags(field) : 0;
            std::string mods = "public ";
            if (flags & MONO_FIELD_ATTR_STATIC) mods += "static ";
            if (flags & MONO_FIELD_ATTR_INIT_ONLY) mods += "readonly ";
            if (flags & MONO_FIELD_ATTR_LITERAL) mods += "const ";

            std::string typeName = GetTypeName(g_mono.field_get_type(field));
            int offset = g_mono.field_get_offset ? g_mono.field_get_offset(field) : -1;

            output << innerIndent << mods << typeName << " " << fName << "; // 0x" << std::hex << std::uppercase << offset << std::dec << "\n";
        }
        if (headerPrinted) output << "\n";
    }

    if (g_mono.class_get_properties) {
        void* iter = nullptr;
        MonoProperty* prop;
        bool headerPrinted = false;
        while ((prop = g_mono.class_get_properties(klass, &iter))) {
            const char* pNameRaw = g_mono.property_get_name(prop);
            if (!pNameRaw) continue;
            std::string pName = SanitizeString(pNameRaw);

            if (!headerPrinted) { output << innerIndent << "// Properties\n"; headerPrinted = true; }

            MonoMethod* getter = g_mono.property_get_get_method ? g_mono.property_get_get_method(prop) : nullptr;
            MonoMethod* setter = g_mono.property_get_set_method ? g_mono.property_get_set_method(prop) : nullptr;

            std::string typeName = "object";
            bool isStatic = false;

            if (getter) {
                MonoMethodSignature* sig = g_mono.method_signature(getter);
                if (sig) typeName = GetTypeName(g_mono.signature_get_return_type(sig));
                unsigned int flags = 0;
                if (g_mono.method_get_flags) {
                    g_mono.method_get_flags(getter, &flags);
                    if (flags & MONO_METHOD_ATTR_STATIC) isStatic = true;
                }
            }
            else if (setter) {
                MonoMethodSignature* sig = g_mono.method_signature(setter);
                if (sig && g_mono.signature_get_params) {
                    void* pIter = nullptr;
                    typeName = GetTypeName(g_mono.signature_get_params(sig, &pIter));
                }
            }

            output << innerIndent << "public " << (isStatic ? "static " : "") << typeName << " " << pName << " { ";
            if (getter) output << "get; ";
            if (setter) output << "set; ";
            output << "}\n";
        }
        if (headerPrinted) output << "\n";
    }

    if (g_mono.class_get_methods) {
        void* iter = nullptr;
        MonoMethod* method;
        bool headerPrinted = false;
        while ((method = g_mono.class_get_methods(klass, &iter))) {
            const char* mNameRaw = g_mono.method_get_name ? g_mono.method_get_name(method) : nullptr;
            if (!mNameRaw) continue;
            std::string mName = SanitizeString(mNameRaw);

            if (mName.find("<") != std::string::npos) continue;
            if (mName.find("get_") == 0 || mName.find("set_") == 0) continue;
            if (mName == ".cctor") continue;

            if (!headerPrinted) { output << innerIndent << "// Methods\n"; headerPrinted = true; }

            bool isCtor = (mName == ".ctor");
            if (isCtor) mName = name;

            unsigned int flags = 0;
            if (g_mono.method_get_flags) flags = g_mono.method_get_flags(method, nullptr);

            std::string modifiers = "public ";
            if (flags & MONO_METHOD_ATTR_STATIC) modifiers += "static ";
            if (flags & MONO_METHOD_ATTR_ABSTRACT) modifiers += "abstract ";
            else if (flags & MONO_METHOD_ATTR_VIRTUAL) modifiers += "virtual ";
            else if (flags & MONO_METHOD_ATTR_FINAL) modifiers += "sealed override ";

            std::string returnType = "void";
            MonoMethodSignature* sig = g_mono.method_signature(method);
            if (sig) returnType = GetTypeName(g_mono.signature_get_return_type(sig));

            uintptr_t addr = (uintptr_t)method;

            output << innerIndent << "// RVA: 0x" << IntToHex(addr) << " VA: 0x" << IntToHex(CalculateVA(addr)) << "\n";

            output << innerIndent << modifiers;
            if (!isCtor) output << returnType << " ";
            output << mName << GetMethodSignature(method) << " { }\n";
        }
    }

    output << indent << "}\n";
    if (!ns.empty()) output << "}\n";
    output << "\n";
}

struct AssemblyInfo { MonoImage* image; std::string name; };
std::vector<AssemblyInfo> g_assemblies;

void CollectAssembly(MonoAssembly* assembly, void* user_data) {
    if (!assembly) return;
    MonoImage* image = g_mono.assembly_get_image ? g_mono.assembly_get_image(assembly) : nullptr;
    if (!image) return;
    g_assemblies.push_back({ image, SanitizeString(g_mono.image_get_name(image)) });
}

void DumpImageList(std::ofstream& output) {
    output << "// Images:\n";
    for (size_t i = 0; i < g_assemblies.size(); i++) {
        output << "// " << i << ": " << addDllExtension(g_assemblies[i].name) << "\n";
    }
    output << "\n";
}

DWORD WINAPI MainThread(LPVOID param) {
    AllocConsole();
    freopen_s(&g_console, "CONOUT$", "w", stdout);

    printf("Initializing\n");

    if (!InitMonoAPI()) {
        printf("Failed to initialize Mono API\n");
        return 1;
    }

    MonoDomain* domain = g_mono.domain_get ? g_mono.domain_get() : nullptr;
    if (!domain && g_mono.get_root_domain) domain = g_mono.get_root_domain();
    if (domain && g_mono.thread_attach) g_mono.thread_attach(domain);

    std::ofstream output("dump.cs");
    if (!output.is_open()) return 1;

    g_assemblies.clear();
    if (g_mono.assembly_foreach) g_mono.assembly_foreach(CollectAssembly, nullptr);

    DumpImageList(output);

    output << "using System;\nusing System.Collections.Generic;\n\n";

    for (const auto& asmInfo : g_assemblies) {
        printf("Dumping %s\n", asmInfo.name.c_str());

        if (g_mono.image_get_table_rows && g_mono.class_get) {
            int rows = g_mono.image_get_table_rows(asmInfo.image, 2);

            for (int i = 1; i <= rows; i++) {
                MonoClass* klass = g_mono.class_get(asmInfo.image, 0x02000000 | i);
                DumpClass(klass, output, asmInfo.name);
            }
        }
    }

    output.close();
    printf("Dump Complete. Check the game directory for the dump.cs\n");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason, LPVOID lpReserved) {
    if (ul_reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}