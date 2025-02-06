#include "stdafx.h"

struct fontsizes {
    int s12;
    int s10;
    int s8;
};

struct fontsizes fs;

char path[MAX_PATH];


void MoveFont(const char* fontName, int origsize, int newsize, bool bold)
{
    char on[128];
    char nn[128];

    
    sprintf(nn, "%s\\Pack\\Pack\\Disc1\\%s_%d%s.font", path, fontName, origsize, bold ? "_bold" : "");
    char* fn2 = strdup(fontName);
    for (int i = 0; i < strlen(fn2); i++)
    {
        if (fn2[i] == ' ')
            fn2[i] = '_';
    }
    sprintf(on, "%s\\%s_%d%s.font", path, fn2, newsize, bold ? "_bold" : "");
    bool ret = MoveFileExA(on, nn, MOVEFILE_REPLACE_EXISTING);
    free(fn2);
}

template<uintptr_t addr>
void LoadLuaScriptHook()
{

    // Hook script loading
    using func_hook = injector::function_hooker<addr, int(void* a, const char* b)>;
    injector::make_static_hook<func_hook>([](func_hook::func_type loadLuaScript, void* a, const char* b)
        {
            
            if (!strcmp(b, "_boot.lua"))
            {
                // Load our hooked script that generates fonts
                return loadLuaScript(a, "_boot_hook.lua");
            }
            else if (!strcmp(b, "CSI3Startup.lua"))
            {
                // Move fonts
                MoveFont("Arial", 12, fs.s12, 1);
                MoveFont("Arial", 12, fs.s12, 0);
                MoveFont("Arial", 10, fs.s10, 1);
                MoveFont("Arial", 8, fs.s8, 1);
                MoveFont("Comic Sans MS", 12, fs.s12, 0);
                MoveFont("Lucida Sans", 12, fs.s12, 0);
                MoveFont("Lucida Sans", 10, fs.s10, 1);
                MoveFont("Lucida Sans", 8, fs.s8, 1);
                MoveFont("Lucida Console", 10, fs.s10, 1);
            }
            return loadLuaScript(a, b);
        });
}


typedef void (__thiscall* FloatPropertySetter)(void* a, float b);

int WINAPI GetDeviceCapsHook(HDC hdc, int index)
{
    // The game does a calculating with the LOGPIXELSY to get the font size, on HighDPI it will be too big.
    return 96;
}

template<class T>
inline T ReadAndWriteMemory(injector::memory_pointer_tr addr, T value, bool vp = false)
{
    T original = injector::ReadMemory<T>(addr, vp);
    injector::WriteMemory<T>(addr, value, vp);
    return original;
}

float scale;
float text_size_scale;

FloatPropertySetter originalTextShadow;



#pragma runtime_checks( "", off )
void __stdcall ScaledTextShadowHeightSetter(float b)
{
    void* that;
    _asm mov that, ecx

    return originalTextShadow(that, b*scale);
}

#pragma runtime_checks( "", restore )

void InitT3MLL()
{
    GetModuleFileNameA(NULL, path, MAX_PATH);
    *strrchr(path, '\\') = '\0';

    CIniReader iniReader("");

    int ResX = iniReader.ReadInteger("MAIN", "ResX", 0);
    int ResY = iniReader.ReadInteger("MAIN", "ResY", 0);

    if (!ResX || !ResY)
        std::tie(ResX, ResY) = GetDesktopRes();

    scale = (float)ResY / 600;
    text_size_scale = 100.0 * ResY / 600;

    // Patch Fullscreen resolution
    auto pattern = hook::module_pattern(GetModuleHandle(L"T3.MLL"), "BA 20 03 00 00 89 15 ? ? ? ? B8 58 02 00 00");
    auto match = pattern.get_one();
    injector::WriteMemory(match.get<uint32_t*>(1), ResX, true);
    injector::WriteMemory(match.get<uint32_t*>(12), ResY, true);

    // Patch Windowed resolution
    pattern = hook::module_pattern(GetModuleHandle(L"T3.MLL"), "C7 44 24 44 20 03 00 00 C7 44 24 48 58 02 00 00");
    match = pattern.get_one();
    injector::WriteMemory(match.get<uint32_t*>(4), ResX, true);
    injector::WriteMemory(match.get<uint32_t*>(12), ResY, true);

    // Make font-generating bitmap larger as 512x512 is not enough for bigger font sizes
    pattern = hook::module_pattern(GetModuleHandle(L"T3.MLL"), "6A 00 6A 20 6A 01 68 00 02 00 00 68 00 02 00 00");
    match = pattern.get_one();
    injector::WriteMemory(match.get<uint32_t*>(7), 2048, true); //height
    //injector::WriteMemory(match.get<uint32_t*>(12), 512, true); // width

    // Make font-generating save in DXT3 instead of DXT5 as DXT5 creates crazy artefacts in the alpha channel
    pattern = hook::module_pattern(GetModuleHandle(L"T3.MLL"), "B9 44 58 54 31 EB 05 B9 44 58 54 35");
    match = pattern.get_one();
    injector::WriteMemory(match.get<uint32_t*>(8), 0x33545844, true); //D3DFMT_DXT3

    // Modify "Text Width", "Text Min Width" and "Text Min Height" property multipliers to properly scale them up
    injector::WriteMemory(0x1006F894 + 2, &text_size_scale, true); //Text Width
    injector::WriteMemory(0x1006F77B + 2, &text_size_scale, true); //Text Min Width
    injector::WriteMemory(0x1006F79B + 2, &text_size_scale, true); //Text Min Height
    
    // Hook "ScaledTextShadowHeightSetter" to scale it up
    originalTextShadow = ReadAndWriteMemory(0x100731F8 + 1, (FloatPropertySetter)ScaledTextShadowHeightSetter, true);

    
    // Calculate font sizes
    fs.s12 = (12 * ResY) / 600;
    fs.s10 = (10 * ResY) / 600;
    fs.s8 = (8 * ResY) / 600;

    // High DPI fix
    injector::MakeRangedNOP(0x10051427, 0x1005142D);
    injector::MakeCALL(0x10051427, GetDeviceCapsHook);

    char boothookpath[MAX_PATH];
    sprintf(boothookpath, "%s\\Pack\\Pack\\Disc1\\_boot_hook.lua", path);

    // Start by generating fonts before returning control to CSI3Startup
    FILE* ha = fopen(boothookpath, "w");
    fprintf(ha, "FontCreate(\"Arial\",%d,1);\n" //arial 12 bold
        "FontCreate(\"Arial\",%d,0);\n" // arial 12
        "FontCreate(\"Arial\",%d,1);\n" // arial 10 bold
        "FontCreate(\"Arial\", %d, 1);\n" //arial 8 bold
        "FontCreate(\"Comic Sans MS\", %d, 0);\n" //comic sans 12
        "FontCreate(\"Lucida Sans\", %d, 0);\n" // lucida sans 12
        "FontCreate(\"Lucida Sans\", %d, 1);\n" // lucida sans 10 bold
        "FontCreate(\"Lucida Sans\", %d, 1);\n" // lucida sans 8
        "FontCreate(\"Lucida Console\", %d, 1);\n" // lucida console 10 bold
        "LoadScript(\"CSI3Startup.lua\");\n", fs.s12, fs.s12, fs.s10, fs.s8, fs.s12, fs.s12, fs.s10, fs.s8, fs.s10);
    fclose(ha);

    // Hook lua script loading
    LoadLuaScriptHook<(0x102721BC)>();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(InitT3MLL);   
     });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}