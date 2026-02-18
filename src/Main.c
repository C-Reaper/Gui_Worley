#if defined __linux__
#include "/home/codeleaded/System/Static/Library/WindowEngine1.0.h"
#include "/home/codeleaded/System/Static/Library/Worley.h"
#elif defined _WINE
#include "/home/codeleaded/System/Static/Library/WindowEngine1.0.h"
#include "/home/codeleaded/System/Static/Library/Worley.h"
#elif defined _WIN32
#include "F:/home/codeleaded/System/Static/Library/WindowEngine1.0.h"
#include "F:/home/codeleaded/System/Static/Library/Worley.h"
#elif defined(__APPLE__)
#error "Apple not supported!"
#else
#error "Platform not supported!"
#endif


Worley wy;

void Setup(AlxWindow* w){
	RGA_Set(Time_Nano());
	wy = Worley_New(10.0f,10.0f,10);
}
void Update(AlxWindow* w){
	TransformedView_HandlePanZoom(&wy.tv,w->Strokes,GetMouse());

	Clear(BLACK);

	Worley_Render(&wy,WINDOW_STD_ARGS);
}
void Delete(AlxWindow* w){
	Worley_Free(&wy);
}

int main(){
    if(Create("Worley",1900,1000,1,1,Setup,Update,Delete))
        Start();
    return 0;
}