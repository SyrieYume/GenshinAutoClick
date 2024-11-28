# GenshinAutoClick
原神后台自动点剧情工具 v1 版本   
推荐大家前往使用更方便的 v2 版本：https://github.com/SyrieYume/GenshinAutoV2

## 注意事项
1. 在第一次运行程序前，请先修改配置文件 `config.ini`。
2. 在使用时，除了游戏进程外，不要运行其它名为 "YuanShen.exe" 或者 "GenshinImpact.exe" 的进程。
3. 开启自动点剧情后，游戏内鼠标将无法点击游戏中其它区域，关闭后即可恢复正常（默认快捷键为 G 键）。

## 如何编译本项目
使用的编译器为 **MinGW** (gcc version 14.2.0).
### 1. 将 Minhook 编译为静态链接库
```bash
mkdir ./Minhook/lib
gcc -c ./Minhook/src/*.c ./Minhook/src/hde/*.c -O2
ar rcs ./Minhook/lib/minhook.lib ./*.o
rm *.o
```


### 2. 编译 dinput8.dll
```bash
gcc ./dllmain.c -shared -o ./build/dinput8.dll -lminhook -L./Minhook/lib -O2
```


### 3. 编译 Genshin AutoClick.exe
```bash
windres ./res/res.rc res.o
gcc ./main.c res.o -o "./build/Genshin AutoClick.exe" -O2
rm res.o
```
