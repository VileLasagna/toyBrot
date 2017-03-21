
#define NOMINMAX


#include <Windows.h>

#include "SDL.h"
#include "SDL_opengl.h"

#include <algorithm>
#include <assert.h>
#include <sstream>
#include <array>
#include <future>
#include <chrono>


struct region{long double Imin,Imax,Rmin,Rmax;}; //This struct delimits a region in the Argand-Gauss Plane (R X I)
bool operator==(const region& r1, const region& r2){return ( (r1.Imax == r2.Imax) && (r1.Imin == r2.Imin) && (r1.Rmax == r2.Rmax) && (r1.Rmin == r2.Rmin) );}

region reg;
SDL_Window* mainwindow;
SDL_Renderer* render;
SDL_Surface* screen;
SDL_Texture* texture;
SDL_Surface* frac;
SDL_Surface* highlight;
bool endProgram;
unsigned int iteration_factor = 100;
unsigned int max_iteration = 256 * iteration_factor;
long double Bailout = 2;
long double power = 2;
int w =	1280;
int h = 720;
int bpp = 32;
float aspect = (float)w/(float)h;
bool drawRect = false;
int rectX = 0;
int rectY = 0;
int MouseX = 0;
int MouseY = 0;
int lastI = 0;
bool creating = false;
bool ColourScheme = false;
auto genTime (std::chrono::high_resolution_clock::now());
std::stringstream stream;

constexpr int numDivs = 8;

std::array<std::future<bool>, numDivs> tasks;



void createHighlight() noexcept
{
    highlight = SDL_CreateRGBSurface(0,w,h,bpp,0,0,0,0);
	SDL_SetSurfaceBlendMode(highlight, SDL_BLENDMODE_BLEND);
	void* pix = highlight->pixels;
	for(int i = 0; i < frac->h; i++)
	{
		for(int j = 0; j< frac->w; j++)
		{
			
			Uint8* p = (Uint8*)pix + (i * highlight->pitch) + j*highlight->format->BytesPerPixel;
			*(Uint32*) p = SDL_MapRGB(frac->format, 255, 255, 255);
		}
	}
    SDL_SetSurfaceAlphaMod(highlight,128);
}

void DrawHL() noexcept
{
	SDL_Rect r;
	r.x = (rectX<MouseX?rectX:MouseX);
	r.y = (rectY<MouseY?rectY:MouseY);
	r.w = abs(MouseX - rectX);
	r.h = abs(MouseY - rectY);
	SDL_BlitSurface(highlight,&r,screen,&r);
}



Uint32 getColour(unsigned int it/*, double x*/) noexcept
{
	

	if (ColourScheme)
	{
		//HELL YEAH EXPENSIVE MATHS!!
		//Aproximate range: From 0.3 to 1018 and then infinity (O.o)
//		long double index = it + (log(2*(log(Bailout))) - (log(log(std::abs(x)))))/log(power);
//		return SDL_MapRGB(frac->format, (sin(index))*255, (sin(index+50))*255, (sin(index+100))*255);
        return SDL_MapRGB(frac->format, 128+ sin((float)it + 1)*128, 128 + sin((float)it)*128 ,  cos((float)it+1.5)*255);
	}
	else
	{
        //std::cout<< it <<std::endl;

        //return SDL_MapRGB(frac->format, 128+ sin((float)it + 1)*128, 128 + sin((float)it)*128 ,  cos((float)it+1.5)*255);
        if(it == max_iteration)
        {
            return SDL_MapRGB(frac->format, 0, 0 , 0);
        }
        else
        {
			//auto b = std::min(it,255u);
            return SDL_MapRGB(frac->format, std::min(it,255u) , 0, std::min(it,255u) );
            //return SDL_MapRGB(frac->format, 128+ sin((float)it + 1)*128, 128 + sin((float)it)*128 ,  cos((float)it+1.5)*255);
        }
	}

}


void CreateFractal(region r) noexcept
{
	//this legacy unused function is here for reference
	if(creating == false)
	{
		lastI = 0;
		creating = true;
	}
	SDL_LockSurface(frac);
	Uint32* pix = (Uint32*)frac->pixels;
	long double incX = std::abs((r.Rmax - r.Rmin)/frac->w);
	long double incY = std::abs((r.Imax - r.Imin)/frac->h);
	for(int i = lastI; i < (lastI+10); i++)
	{
		if (i == frac->h)
		{
			break;
		}
		for(int j = 0; j< frac->w; j++)
		{
			
			Uint8* p = (Uint8*)pix + (i * frac->pitch) + j*frac->format->BytesPerPixel;//Don't ask u.u
			long double x = r.Rmin+(j*incX);
			long double y = r.Imax-(i*incY);
			long double x0 = x;
			long double y0 = y;

			unsigned int iteration = 0;

			while ( (x*x + y*y <= 4)  &&  (iteration < max_iteration) ) 
			{
				long double xtemp = x*x - y*y + x0;
				y = 2*x*y + y0;

				x = xtemp;

				iteration++;
			}

			*(Uint32*) p = getColour(iteration/*, x*/);
		}
	}
	lastI +=10;
	if (lastI == frac->h)
	{
		creating = false;
	}
	SDL_UnlockSurface(frac);
	
}

void paint() noexcept
{



	SDL_BlitSurface(frac,0,screen,0);
	if(drawRect)
	{
		DrawHL();
	}

	SDL_UpdateTexture(texture, NULL, screen->pixels, screen->pitch);
	SDL_RenderClear(render);
	SDL_RenderCopy(render, texture, NULL, NULL);
	SDL_RenderPresent(render);

}

auto fracGen = [](region r,int index, Uint32* pix, int h0) noexcept
{
    //std::cout << "tid: " << std::this_thread::get_id() << std::endl;

	//Uint32* pix = (Uint32*)frac->pixels;
	long double incX = std::abs((r.Rmax - r.Rmin)/frac->w);
	long double incY = std::abs((r.Imax - r.Imin)/frac->h);
	for(int i = h0;i < h0+10; i++)
	{
		if (i == frac->h)
		{
			return true;
		}
        //Initially intuitive/illustrative division
        //for(int j = (index%numDivs)*(frac->w/numDivs); j< ((index%numDivs)+1)*(frac->w/numDivs); j++)
        //Newer prefetcher-friendly version
        for(int j = 0 + index; j< frac->w; j+=numDivs)
		{

			Uint8* p = (Uint8*)pix + (i * frac->pitch) + j*frac->format->BytesPerPixel;//Set initial pixel
			long double x = r.Rmin+(j*incX);
			long double y = r.Imax-(i*incY);
			long double x0 = x;
			long double y0 = y;

			unsigned int iteration = 0;

			while ( (x*x + y*y <= 4)  &&  (iteration < max_iteration) )
			{
				long double xtemp = x*x - y*y + x0;
				y = 2*x*y + y0;

				x = xtemp;

				iteration++;
			}

			*(Uint32*) p = getColour(iteration/*, x*/);
		}
	}
	return false;
};

void spawnTasks(region reg) noexcept
{
	creating = true;
	static std::atomic<int> h {0};

	SDL_LockSurface(frac);
    for(unsigned int i = 0; i < tasks.size(); i++)
	{
		tasks[i] = std::async(std::launch::async, fracGen,reg, i, /*tasks.size(),*/ (Uint32*)frac->pixels,h.load());
	}

    h+= 10;

    for(unsigned int i = 0; i < tasks.size(); i++)
	{
		if(tasks[i].get())
		{
			h.store(0);
			creating = false;
			auto d = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - genTime).count();

			stream << "Fractal Generation Took " << d << " milliseconds";
			SDL_SetWindowTitle(mainwindow, stream.str().c_str());
			stream.str("");
		}
	}
	SDL_UnlockSurface(frac);




}


void onKeyboardEvent(const SDL_KeyboardEvent& ) noexcept
{


}

void onMouseMotionEvent(const SDL_MouseMotionEvent& e) noexcept
{
	MouseX = e.x;
	MouseY = e.y;
	int rw = MouseX - rectX;
	int rh = MouseY - rectY;
	if (rh == 0)
	{
		return;
	}
	float ra = abs(rw/rh);
	if (ra == aspect)
	{
		return;
	}
	if (ra < aspect)
	{

		MouseX = rectX + rh*aspect;

	}
	else
	{
		MouseY = rectY + rw/aspect;
	}
}

void onMouseButtonEvent(const SDL_MouseButtonEvent& e ) noexcept
{
	if (creating)
	{
		//ignore
		return;
	}
	if (e.button == 4)
	{
		//M_WHEEL_UP
		//std::cout<< "button 4" <<std::endl;
		return;
	}
	if (e.button == 5)
	{
		//M_WHEEL_DOWN
		//std::cout<< "button 5" <<std::endl;
		return;
	}
	if (e.button == 2)
	{
		//Middle Button
		ColourScheme = !ColourScheme;
		//CreateFractal(reg);
        genTime = std::chrono::high_resolution_clock::now();
		spawnTasks(reg);
		return;
	}
	if(e.button == 3)
	{
		//Right Button
		reg.Imax = 1.5;
		reg.Imin = -1.5;
		reg.Rmax = 1;
		reg.Rmin = -2;
		//CreateFractal(reg);
        genTime = std::chrono::high_resolution_clock::now();
		spawnTasks(reg);
		return;
	}
	if(e.type == SDL_MOUSEBUTTONDOWN)
	{
		rectX = e.x;
		rectY = e.y;
		drawRect = true;
	}
	else
	{
		int rx = MouseX;
		int ry = MouseY;
//		int rw = std::abs(MouseX - rectX);
//		int rh = std::abs(MouseY - rectY);
		
	


		double x0 = reg.Rmin + ((reg.Rmax - reg.Rmin)/w) * rectX;
		double x1 = reg.Rmin + ((reg.Rmax - reg.Rmin)/w) * rx;
		
		double y0 = reg.Imax - ((reg.Imax - reg.Imin)/h) * rectY;
		double y1 = reg.Imax - ((reg.Imax - reg.Imin)/h) * ry;

		reg.Rmax = (x0>x1?x0:x1);
		reg.Rmin = (x0>x1?x1:x0);

		
		reg.Imax = (y0>y1?y0:y1);
		reg.Imin = (y0>y1?y1:y0);

		drawRect = false;
		//CreateFractal(reg);
        genTime = std::chrono::high_resolution_clock::now();
		spawnTasks(reg);
	}

}

void capture() noexcept
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			onKeyboardEvent(event.key);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			onMouseButtonEvent(event.button);
			break;

		case SDL_QUIT:
			endProgram = true;
			break;

		case SDL_MOUSEMOTION:
			onMouseMotionEvent(event.motion);
			break;

		case SDL_JOYAXISMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYHATMOTION:
		case SDL_JOYBALLMOTION:
//		case SDL_ACTIVEEVENT:
//		case SDL_VIDEOEXPOSE:
//		case SDL_VIDEORESIZE:
			break;

		default:
			// Unexpected event type!
			//assert(0);
			break;
		}
	}
}




int main (int , char**) noexcept
{
	endProgram = false;
	SDL_Init(SDL_INIT_EVERYTHING);
//	screen = SDL_SetVideoMode(w,h,bpp, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ASYNCBLIT);
	mainwindow = SDL_CreateWindow("Mandelbrot Fractal Explorer - Use Mouse1 to zoom in and Mouse2 to zoom out. Press Mouse 3 to change colouring scheme",
							  SDL_WINDOWPOS_UNDEFINED,
							  SDL_WINDOWPOS_UNDEFINED,
							  w, h,
							  SDL_WINDOW_SHOWN);

	render = SDL_CreateRenderer(mainwindow, -1, 0);

	screen = SDL_CreateRGBSurface(0, w, h, bpp,
									0x00FF0000,
									0x0000FF00,
									0x000000FF,
									0xFF000000);

	texture = SDL_CreateTexture(render,
								SDL_PIXELFORMAT_ARGB8888,
								SDL_TEXTUREACCESS_STREAMING,
								w, h);
	assert(screen);
//	SDL_WM_SetCaption("Mandelbrot Fractal Explorer - Use Mouse1 to zoom in and Mouse2 to zoom out. Press Mouse 3 to change colouring scheme",0);
	SDL_SetWindowTitle(mainwindow, "Mandelbrot Fractal Explorer - Use Mouse1 to zoom in and Mouse2 to zoom out. Press Mouse 3 to change colouring scheme");

	//frac =	SDL_CreateRGBSurface(SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_ASYNCBLIT,w,h,bpp,0,0,0,0);
	frac = SDL_CreateRGBSurface(0, w, h, bpp,
								0x00FF0000,
								0x0000FF00,
								0x000000FF,
								0xFF000000);

	
	reg.Imax = 1.5;
	reg.Imin = -1.5;
	reg.Rmax = 1;
	reg.Rmin = -2;


	//CreateFractal(reg);
    genTime = std::chrono::high_resolution_clock::now();
	spawnTasks(reg);
	createHighlight();

	while(!endProgram)
	{
		if(creating)
		{
			spawnTasks(reg);
		}

		capture();
		paint();
	}

	return 0;
}

