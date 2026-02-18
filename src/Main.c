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


Worley wrp;

void Setup(AlxWindow* w){
	RGA_Set(Time_Nano());
	wrp = Worley_New(10.0f,10.0f,10);
}
void Update(AlxWindow* w){
	Clear(BLACK);

	Worley_Render(&wrp,WINDOW_STD_ARGS);
}
void Delete(AlxWindow* w){
	Worley_Free(&wrp);
}

int main(){
    if(Create("Worley",1900,1000,1,1,Setup,Update,Delete))
        Start();
    return 0;
}