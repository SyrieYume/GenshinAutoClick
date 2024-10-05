#include <Windows.h>
#include <stdio.h>
#include <locale.h>
#include <ShlObj.h>
#include <direct.h>

#define error(message, ...) {wprintf(L"\033[31m[Error]\033[0m " TEXT(message) L"\n", ##__VA_ARGS__); system("pause"); exit(-1); }
#define log(message, ...) wprintf(L"\033[32m[Info]\033[0m " TEXT(message) L"\n", ##__VA_ARGS__)

const wchar_t dllName[] = L"dinput8.dll";
const wchar_t configFileName[] = L"config.ini";

wchar_t targetName[MAX_PATH];
int clickPosX, clickPosY;
int triggerKey;



// 开启Windows的虚拟终端序列支持
BOOLEAN enableVTMode() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hOut, &dwMode))
        return FALSE;

    dwMode |= 0x0004;
    if (!SetConsoleMode(hOut, dwMode))
        return FALSE;

    return TRUE;
}



// 以管理员模式重新运行程序
void runAsAdmin(int argc, char* argv[]) {
    if (IsUserAnAdmin())
        return;

    char szFilePath[MAX_PATH];
    GetModuleFileNameA(NULL, szFilePath, MAX_PATH);

    char parameters[200] = {'\0'};
    for (int i = 1, len = 0; i < argc; ++i)
        len += sprintf_s(parameters + len, 200, "%s ", argv[i]);

    char dir[MAX_PATH];
    if (!_getcwd(dir, MAX_PATH))
        exit(-1);
        
    printf("Restart as Admin.\npath: %s\nparam: %s\ndir: %s\n",szFilePath, parameters, dir);
    ShellExecuteA(NULL, "runas", szFilePath, parameters, dir, SW_SHOWNORMAL);
    exit(0);
}


BOOLEAN loadConfigFile(){
    FILE *fp = _wfopen(configFileName, L"r");
    if(!fp) 
        return FALSE;

    char line[256];
    char key[128], value[128];

     while (fgets(line, 256, fp) != NULL) {
        if(*line == '#' || sscanf(line, "%[^ ] = %s", key, value) != 2)
            continue;

        if(strcmp(key, "target") == 0) {
            setlocale(LC_CTYPE, "en_US.UTF-8");
            size_t len = mbstowcs(targetName, value, MAX_PATH);
        }

        else if(strcmp(key, "clickPosX") == 0)
            clickPosX = atoi(value);

        else if(strcmp(key, "clickPosY") == 0)
            clickPosY = atoi(value);

        else if(strcmp(key, "triggerKey") == 0)
            triggerKey = *value;
     }

    log("Successfully load config File.");

    return TRUE;
}



int main(int argc, char* argv[]) {
    enableVTMode();
    runAsAdmin(argc, argv);
    loadConfigFile();


    log("target = '%ls'", targetName);
    log("clickPos = (%d, %d)", clickPosX, clickPosY);
    log("triggerKey = '%c'", triggerKey);


    HMODULE module;
    HHOOK hook;


    // Load DLL and hookProc.
    module = LoadLibraryW(dllName);
    if (!module)
        error("Faild to load '%ls'!", dllName);


    FARPROC fn = GetProcAddress(module, "hookProc");
    if (!fn)
        error("Can't find function 'hookProc' in module!");


    typedef void (WINAPI *SetTargetNameT)(wchar_t* _targetName);
    SetTargetNameT setTargetName = (SetTargetNameT)GetProcAddress(module, "setTargetName");
    if (!setTargetName)
        error("Can't find function 'setTargetName' in module!");

    setTargetName(targetName);


    typedef void (WINAPI *SetClickPosT)(int x, int y);
    SetClickPosT setClickPos = (SetClickPosT)GetProcAddress(module, "setClickPos");
    if (!setClickPos)
        error("Can't find function 'setClickPos' in module!");

    setClickPos(clickPosX, clickPosY);


    typedef void (WINAPI *SetTrigerKeyT)(int keycode);
    SetTrigerKeyT setTrigerKey = (SetTrigerKeyT)GetProcAddress(module, "setTrigerKey");
    if (!setTrigerKey)
        error("Can't find function 'setTrigerKey' in module!");

    setTrigerKey(triggerKey);
    

    log("Successfully load '%ls'.", dllName);


    // Set hook.
    hook = SetWindowsHookExW(WH_CBT, (HOOKPROC)fn, module, 0);
    if (!hook)
        error("Can't set windows hook!");

    log("Successfully set windows hook.");


    system("pause");
    UnhookWindowsHookEx(hook);
    FreeLibrary(module);
    
    return 0;
}