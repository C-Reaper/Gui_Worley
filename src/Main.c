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
Vec2* p;

void Setup(AlxWindow* w){
	RGA_Set(Time_Nano());
	wy = Worley_New(100.0f,100.0f,4);
	p = NULL;
}
void Update(AlxWindow* w){
	TransformedView_HandlePanZoom(&wy.tv,w->Strokes,GetMouse());
	const Vec2 m = TransformedView_ScreenWorldPos(&wy.tv,GetMouse());

	if(Stroke(ALX_MOUSE_L).PRESSED){
		for(int i = 0;i<wy.points.size;i++){
			Vec2* pp = (Vec2*)Vector_Get(&wy.points,i);
			if(Circle_Point((Circle[]){ Circle_New(*pp,1.0f) },m)){
				p = pp;
			}
		}
	}
	if(Stroke(ALX_MOUSE_L).DOWN){
		if(p){
			*p = m;
		}
	}
	if(Stroke(ALX_MOUSE_L).RELEASED){
		p = NULL;
	}

	Worley_Update(&wy);

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