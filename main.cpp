#include <raylib.h>
#include "UIComponents.cpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
using namespace std;

int main(){
    InitWindow(800,600,"UI Test");
    SetTargetFPS(60);

    font = LoadFontEx("res/RedHatMono-Medium.ttf",20,0,250);
    if(font.texture.id == 0){
        cout << "Error: Font could not be loaded" << endl;
        return 1;
    }
}
