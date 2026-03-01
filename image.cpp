//---------------------------------------------------------------------------
#include "stdafx.h"

#include <cassert>
#include <cmath> // n.b., needed on Linux at least

//#define TIMING

#ifdef TIMING
#include <ctime> // for performance testing
#endif

#include <algorithm>
using std::min;
using std::max;

#include "image.h"
#include "utils.h"
//---------------------------------------------------------------------------

SDL_Surface *my_IMG_LoadLBM_RW(SDL_IOStream *src);

inline void CreateMask( Uint32& rmask, Uint32& gmask, Uint32& bmask, Uint32& amask) {
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#elif defined(__APPLE__) && defined(__MACH__)
	rmask = 0x00ff0000;
	gmask = 0x0000ff00;
	bmask = 0x000000ff;
	amask = 0xff000000;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
// TEST:
	/*rmask = 0x00ff0000;
	gmask = 0x0000ff00;
	bmask = 0x000000ff;
	amask = 0xff000000;*/
#endif
}

extern const bool DEBUG;
extern const int DEBUGLEVEL;

SDL_Renderer *Gigalomania::Image::sdlRenderer = NULL;

Gigalomania::Image::Image() {
	this->data = NULL;
	this->need_to_free_data = false;
	this->surface = NULL;
	this->texture = NULL;
	this->scale_x = 1;
	this->scale_y = 1;
	this->offset_x = 0;
	this->offset_y = 0;
}

Gigalomania::Image::~Image() {
	free();
}

void Gigalomania::Image::free() {
	if( this->surface != NULL ) {
		SDL_DestroySurface(this->surface);
		this->surface = NULL;
	}
	if( this->texture != NULL ) {
		SDL_DestroyTexture(this->texture);
		this->texture = NULL;
	}
	if( need_to_free_data && this->data != NULL ) {
		delete [] this->data;
		this->data = NULL;
	}
}

void Gigalomania::Image::draw(int x, int y) const {
	x += offset_x;
	y += offset_y;
	x = (int)(x * scale_x);
	y = (int)(y * scale_y);
	SDL_FRect dstrect;
	dstrect.x = (float)x;
	dstrect.y = (float)y;
	dstrect.w = (float)this->getWidth();
	dstrect.h = (float)this->getHeight();
	SDL_RenderTexture(sdlRenderer, texture, NULL, &dstrect);
}

void Gigalomania::Image::draw(int x, int y, int sw, int sh) const {
	x += offset_x;
	y += offset_y;
	x = (int)(x * scale_x);
	y = (int)(y * scale_y);
	sw = (int)(sw * scale_x);
	sh = (int)(sh * scale_y);
	SDL_FRect srcrect;
	srcrect.x = 0; srcrect.y = 0; srcrect.w = (float)sw; srcrect.h = (float)sh;
	SDL_FRect dstrect;
	dstrect.x = (float)x; dstrect.y = (float)y; dstrect.w = (float)sw; dstrect.h = (float)sh;
	SDL_RenderTexture(sdlRenderer, texture, &srcrect, &dstrect);
}

void Gigalomania::Image::draw(int x, int y, float scale_w, float scale_h) const {
	x = (int)(x * scale_x);
	y = (int)(y * scale_y);
	SDL_FRect dstrect;
	dstrect.x = (float)x; dstrect.y = (float)y;
	dstrect.w = (float)(this->getWidth()*scale_w);
	dstrect.h = (float)(this->getHeight()*scale_h);
	SDL_RenderTexture(sdlRenderer, texture, NULL, &dstrect);
}

void Gigalomania::Image::drawWithAlpha(int x, int y, unsigned char alpha) const {
	// n.b., only works if the image doesn't have per-pixel alpha channel
	SDL_SetTextureAlphaMod(texture, alpha);
	this->draw(x, y);
}

int Gigalomania::Image::getWidth() const {
	return this->surface->w;
}

int Gigalomania::Image::getHeight() const {
	return this->surface->h;
}

void Gigalomania::Image::setScale(float scale_x,float scale_y) {
	this->scale_x = scale_x;
	this->scale_y = scale_y;
}

bool Gigalomania::Image::isPaletted() const {
	return SDL_GetSurfacePalette(this->surface) != NULL;
}

int Gigalomania::Image::getNColors() const {
	return SDL_GetSurfacePalette(this->surface)->ncolors;
}

unsigned char Gigalomania::Image::getPixelIndex(int x,int y) const {
	if( !isPaletted() )
		return 0;

	SDL_LockSurface(this->surface);
	unsigned char c = ((unsigned char *)this->surface->pixels)[ y * this->surface->pitch + x ];
	SDL_UnlockSurface(this->surface);
	return c;
}

bool Gigalomania::Image::setPixelIndex(int x,int y,unsigned char c) {
	if( !isPaletted() )
		return false;

	SDL_LockSurface(this->surface);
	((unsigned char *)this->surface->pixels)[ y * this->surface->pitch + x ] = c;
	SDL_UnlockSurface(this->surface);
	return true;
}

bool Gigalomania::Image::setColor(int index,unsigned char r,unsigned char g,unsigned char b) {
	if( !isPaletted() )
		return false;

	SDL_LockSurface(this->surface);
	SDL_Color *col = &SDL_GetSurfacePalette(this->surface)->colors[index];
	col->r = r;
	col->g = g;
	col->b = b;
	SDL_UnlockSurface(this->surface);
	return true;
}

/*
* Return the pixel value at (x, y)
* NOTE: The surface must be locked before calling this!
*/
Uint32 getpixel(SDL_Surface *surface, int x, int y) {
	int bpp = SDL_BYTESPERPIXEL(surface->format);
	/* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		return *p;

	case 2:
		return *reinterpret_cast<const Uint16*>(p);

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			return p[0] << 16 | p[1] << 8 | p[2];
		else
			return p[0] | p[1] << 8 | p[2] << 16;

	case 4:
		return *reinterpret_cast<const Uint32*>(p);

	default:
		return 0;       /* shouldn't happen, but avoids warnings */
	}
}

/*
* Set the pixel at (x, y) to the given value
* NOTE: The surface must be locked before calling this!
*/
void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = SDL_BYTESPERPIXEL(surface->format);
	/* Here p is the address to the pixel we want to set */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

	switch(bpp) {
	case 1:
		*p = pixel;
		break;

	case 2:
		*reinterpret_cast<Uint16*>(p) = static_cast<Uint16>(pixel);
		break;

	case 3:
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			p[0] = (pixel >> 16) & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = pixel & 0xff;
		} else {
			p[0] = pixel & 0xff;
			p[1] = (pixel >> 8) & 0xff;
			p[2] = (pixel >> 16) & 0xff;
		}
		break;

	case 4:
		*reinterpret_cast<Uint32*>(p) = pixel;
		break;
	}
}

// Creates an alpha from the mask; also adds in shadow effect based on supplied ar/ag/ab colour
bool Gigalomania::Image::createAlphaForColor(bool mask, unsigned char mr, unsigned char mg, unsigned char mb, unsigned char ar, unsigned char ag, unsigned char ab, unsigned char alpha) {
	int w = this->getWidth();
	int h = this->getHeight();

	this->convertToHiColor(true);

#ifdef TIMING
	int time_s = clock();
#endif

	SDL_LockSurface(this->surface);
	// faster to read in x direction! (caching?)
	for(int cy=0;cy<h;cy++) {
		for(int cx=0;cx<w;cx++) {
			Uint32 pixel = getpixel(this->surface, cx, cy);
			Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_GetRGB(pixel, SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), &r, &g, &b);
			if( r == ar && g == ag && b == ab ) {
				r = 0;
				g = 0;
				b = 0;
				a = alpha;
				pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), r, g, b, a);
				putpixel(this->surface, cx, cy, pixel);
			}
			else if( mask && r == mr && g == mg && b == mb ) {
				r = 0;
				g = 0;
				b = 0;
				a = 0;
				pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), r, g, b, a);
				putpixel(this->surface, cx, cy, pixel);
			}
		}
	}

	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image createAlphaForColor time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image createAlphaForColor total %d\n", total);
#endif
	return true;
}

void Gigalomania::Image::scaleAlpha(float scale) {
	int bpp = SDL_BITSPERPIXEL(this->surface->format);
	if( bpp != 32 )
		return;

#ifdef TIMING
	int time_s = clock();
#endif

	int w = this->getWidth();
	int h = this->getHeight();
	SDL_LockSurface(this->surface);
	// faster to read in x direction! (caching?)
	for(int cy=0;cy<h;cy++) {
		for(int cx=0;cx<w;cx++) {
			Uint32 pixel = getpixel(this->surface, cx, cy);
			Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), &r, &g, &b, &a);
			//a *= scale;
			a = (Uint8)(a*scale);
			pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), r, g, b, a);
			putpixel(this->surface, cx, cy, pixel);
		}
	}
	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image scaleAlpha time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image scaleAlpha total %d\n", total);
#endif
}

bool Gigalomania::Image::convertToHiColor(bool alpha) {
#ifdef TIMING
	int time_s = clock();
#endif
	// todo: repeated conversions don't seem to work? seems to be due to repeated blitting not working
	int bpp = SDL_BITSPERPIXEL(this->surface->format);
	if( bpp == 32 )
		return false;
	Uint32 rmask, gmask, bmask, amask;
	CreateMask(rmask, gmask, bmask, amask);

	int depth = alpha ? 32 : 24;
	SDL_Surface *new_surf = SDL_CreateSurface(this->getWidth(), this->getHeight(), SDL_GetPixelFormatForMasks(depth, rmask, gmask, bmask, amask));
	SDL_BlitSurface(this->surface, NULL, new_surf, NULL);
	free();
	this->surface = new_surf;
	this->need_to_free_data = false;

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image convert time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image convert total %d\n", total);
#endif
	return true;
}

bool Gigalomania::Image::convertToDisplayFormat() {
	{
	texture = SDL_CreateTextureFromSurface(sdlRenderer, surface);
	if( texture == NULL ) {
		LOG("SDL_CreateTextureFromSurface failed\n");
		return false;
	}
	/*{
		SDL_BlendMode blendMode = SDL_BLENDMODE_NONE;
		SDL_GetTextureBlendMode(texture, &blendMode);
		LOG("texture blend mode: %d\n", blendMode);
	}*/
	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND); // seems to be default, but just in case
	}
	return true;
}

bool Gigalomania::Image::copyPalette(const Gigalomania::Image *image) {
	SDL_Palette *src_palette = SDL_GetSurfacePalette(image->surface);
	SDL_Palette *dst_palette = SDL_GetSurfacePalette(this->surface);
	if( src_palette == NULL || dst_palette == NULL )
		return false;

	SDL_SetPaletteColors(dst_palette, src_palette->colors, 0, dst_palette->ncolors);
	return true;
}

// side-effect: also converts images with < 256 colours to have 256 colours, unless scaling is 1.0
void Gigalomania::Image::scale(float sx,float sy) {
	if( sx == 1.0f && sy == 1.0f ) {
		//LOG("no need to scale\n");
		return;
	}
	//LOG("having to scale %f x %f\n", sx, sy);
	// only supported for either reducing or englarging the size - this is all we need, and is easier to optimise for performance
	bool enlarging = false;
	if( sx > 1.0f || sy > 1.0f ) {
		// making larger
		ASSERT( sx > 1.0f );
		ASSERT( sy > 1.0f );
		enlarging = true;
	}
#ifdef TIMING
	int time_s = clock();
#endif
	int w = this->getWidth();
	int h = this->getHeight();
	SDL_LockSurface(this->surface);
	unsigned char *src_data = (unsigned char *)this->surface->pixels;
	int bytesperpixel = SDL_BYTESPERPIXEL(this->surface->format);
	int new_width = (int)(w * sx);
	int new_height = (int)(h * sy);
	int new_size = (int)(new_width * new_height * bytesperpixel);
	bool is_paletted = this->isPaletted();
	unsigned char *new_data = NULL;
	int *new_data_nonpaletted = NULL;
	int *count = NULL;
	if( is_paletted || enlarging ) {
		new_data = new unsigned char[new_size];
	}
	else {
		new_data_nonpaletted = new int[new_size];
		count = new int[new_size];
		for(int i=0;i<new_size;i++) {
			new_data_nonpaletted[i] = 0;
			count[i] = 0;
		}
	}
	// faster to read in x direction! (caching?)
	if( enlarging ) {
		for(int cy=0;cy<h;cy++) {
			int src_indx = cy * this->surface->pitch;
			for(int cx=0;cx<w;cx++) {
				for(int i=0;i<bytesperpixel;i++, src_indx++) {
					T_ASSERT(src_indx >= 0 && src_indx < this->surface->pitch*h );
					unsigned char pt = src_data[ src_indx ];
					for(int y=0;y<sy;y++) {
						for(int x=0;x<sx;x++) {
							int dx = (int)(cx * sx + x);
							int dy = (int)(cy * sy + y);
							T_ASSERT( dx >= 0 && dx < new_width );
							T_ASSERT( dy >= 0 && dy < new_height );
							int dst_indx = (int)(dy * new_width * bytesperpixel + dx * bytesperpixel + i);
							T_ASSERT(dst_indx >= 0 && dst_indx < new_width*new_height*bytesperpixel );
							new_data[ dst_indx ] = pt;
						}
					}
				}
			}
		}
	}
	else {
		for(int cy=0;cy<h;cy++) {
			int src_indx = cy * this->surface->pitch;
			for(int cx=0;cx<w;cx++) {
				for(int i=0;i<bytesperpixel;i++, src_indx++) {
					T_ASSERT(src_indx >= 0 && src_indx < this->surface->pitch*h );
					unsigned char pt = src_data[ src_indx ];

					int dx = (int)(cx * sx);
					int dy = (int)(cy * sy);
					if( dx >= new_width || dy >= new_height ) {
						// ignore leftover part
						continue;
					}
					T_ASSERT( dx >= 0 && dx < new_width );
					T_ASSERT( dy >= 0 && dy < new_height );
					int dst_indx = (int)(dy * new_width * bytesperpixel + dx * bytesperpixel + i);
					T_ASSERT(dst_indx >= 0 && dst_indx < new_width*new_height*bytesperpixel );
					if( is_paletted )
						new_data[ dst_indx ] = pt;
					else {
						new_data_nonpaletted[ dst_indx ] += pt;
						count[ dst_indx ]++;
					}
				}
			}
		}
	}
	SDL_UnlockSurface(this->surface);

	{
		w = (int)(w * sx);
		h = (int)(h * sy);
		SDL_PixelFormat fmt = this->surface->format;
		int pitch = SDL_BYTESPERPIXEL(this->surface->format) * w;
		if( !(is_paletted || enlarging) ) {
			new_data = new unsigned char[new_size];
			for(int i=0;i<new_size;i++) {
				new_data[i] = (unsigned char)(new_data_nonpaletted[i] / count[i]);
			}
			delete [] new_data_nonpaletted;
			new_data_nonpaletted = NULL;
			delete [] count;
			count = NULL;
		}
		SDL_Surface *new_surf = SDL_CreateSurfaceFrom(w, h, fmt, new_data, pitch);
		{
			SDL_Palette *src_pal = SDL_GetSurfacePalette(this->surface);
			SDL_Palette *dst_pal = SDL_GetSurfacePalette(new_surf);
			if( src_pal != NULL && dst_pal != NULL ) {
				SDL_SetPaletteColors(dst_pal, src_pal->colors, 0, src_pal->ncolors);
			}
		}
		free();
		this->surface = new_surf;
		this->data = new_data;
		this->need_to_free_data = true;
	}

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image scale time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image scale total %d\n", total);
#endif
}

bool Gigalomania::Image::scaleTo(int n_w) {
	float scale_factor = ((float)n_w) / (float)this->getWidth();
	this->scale(scale_factor, scale_factor); // nb, still scale if scale_factor==1, as this is a way of converting to 8bit
	return true;
}

void Gigalomania::Image::remap(unsigned char sr,unsigned char sg,unsigned char sb,unsigned char rr,unsigned char rg,unsigned char rb) {
	if( SDL_BITSPERPIXEL(this->surface->format) != 24 && SDL_BITSPERPIXEL(this->surface->format) != 32 ) {
		return;
	}
	if( rr == sr && rg == sg && rb == sb ) {
		return;
	}
#ifdef TIMING
	int time_s = clock();
#endif
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	/*int isr = (int)sr;
	int isg = (int)sg;
	int isb = (int)sb;
	int mag_s = (int)sqrt( (float)(isr*isr + isg*isg + isb*isb) ); // *255
	const int threshold = 0.9f * mag_s; // *255
	int irr = (int)rr;
	int irg = (int)rg;
	int irb = (int)rb;
	int mag_r = (int)sqrt( (float)(irr*irr + irg*irg + irb*irb) ); // *255*/
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 r = 0, g = 0, b = 0, a = 0;
			SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), &r, &g, &b, &a);
			if( r == sr && g == sg && b == sb ) {
				pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), SDL_GetSurfacePalette(surface), rr, rg, rb, a);
				putpixel(this->surface, x, y, pixel);
			}
			/*if( r == 0 && g == 0 && b == 0 ) {
				continue;
			}
			int ir = (int)r;
			int ig = (int)g;
			int ib = (int)b;
			int mag = (int)sqrt( (float)(ir*ir + ig*ig + ib*ib) ); // *255
			int dot = ( sr*ir + sg*ig + sb*ib ); // *255*255
			if( dot >= threshold*mag ) {
				float cos_angle = ((float)dot) / (float)( mag_s * mag ); // *1
				int proj_mag = mag * cos_angle; // *255
				ir = proj_mag * rr / mag_r;
				ig = proj_mag * rg / mag_r;
				ib = proj_mag * rb / mag_r;
				pixel = SDL_MapRGBA(surface->format, ir, ig, ib, a);
				putpixel(this->surface, x, y, pixel);
			}*/
		}
	}

	SDL_UnlockSurface(this->surface);

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image remap time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image remap total %d\n", total);
#endif
}

void Gigalomania::Image::reshadeRGB(int from, bool to_r, bool to_g, bool to_b) {
	ASSERT(from >= 0 && from < 3);
	if( SDL_BITSPERPIXEL(this->surface->format) != 24 && SDL_BITSPERPIXEL(this->surface->format) != 32 ) {
		return;
	}
	bool to[3] = {to_r, to_g, to_b};
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 rgba[] = {0, 0, 0, 0};
			SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			int val = rgba[from];
			int t_diff = 0;
			int n = 0;
			for(int j=0;j<3;j++) {
				if( to[j] && j != from ) {
					int val2 = rgba[j];
					int diff = val2 - val;
					t_diff += diff;
					n++;
					rgba[j] = val;
				}
			}
			if( n > 0 && !to[from] ) {
				t_diff /= n;
				val += t_diff;
				ASSERT(val >=0 && val < 256);
				rgba[from] = val;
			}
			pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), SDL_GetSurfacePalette(surface), rgba[0], rgba[1], rgba[2], rgba[3]);
			putpixel(this->surface, x, y, pixel);
		}
	}

	SDL_UnlockSurface(this->surface);
}

void Gigalomania::Image::brighten(float sr, float sg, float sb) {
	if( SDL_BITSPERPIXEL(this->surface->format) != 24 && SDL_BITSPERPIXEL(this->surface->format) != 32 ) {
		return;
	}
#ifdef TIMING
	int time_s = clock();
#endif
	float scale[3] = {sr, sg, sb};
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 rgba[] = {0, 0, 0, 0};
			SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			for(int j=0;j<3;j++) {
				float col = (float)rgba[j];
				col *= scale[j];
				if( col < 0 )
					col = 0;
				else if( col > 255 )
					col = 255;
				rgba[j] = (unsigned char)col;
			}
			pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), SDL_GetSurfacePalette(surface), rgba[0], rgba[1], rgba[2], rgba[3]);
			putpixel(this->surface, x, y, pixel);
		}
	}
	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image brighten time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image brighten total %d\n", total);
#endif
}

void Gigalomania::Image::fadeAlpha(bool x_dir, bool fwd) {
	if( SDL_BITSPERPIXEL(this->surface->format) != 24 && SDL_BITSPERPIXEL(this->surface->format) != 32 ) {
		return;
	}
#ifdef TIMING
	int time_s = clock();
#endif
	SDL_LockSurface(this->surface);
	int w = getWidth();
	int h = getHeight();
	// faster to read in x direction! (caching?)
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			Uint32 pixel = getpixel(this->surface, x, y);
			Uint8 rgba[] = {0, 0, 0, 0};
			SDL_GetRGBA(pixel, SDL_GetPixelFormatDetails(this->surface->format), SDL_GetSurfacePalette(this->surface), &rgba[0], &rgba[1], &rgba[2], &rgba[3]);
			float frac = 0.0f;
			//float perp_frac = 0.0f;
			if( x_dir ) {
				frac = ((float)x)/(float)(w-1.0f);
				//perp_frac = ((float)y)/(float)(h-1.0f);
			}
			else {
				frac = ((float)y)/(float)(h-1.0f);
				//perp_frac = ((float)x)/(float)(w-1.0f);
			}
			if( !fwd ) {
				frac = 1.0f - frac;
			}
			float cutoff = 0.75f;
			float alpha = 0.0f;
			if( frac >= cutoff ) {
				alpha = (frac-cutoff) / (1.0f-cutoff);
			}
			rgba[3] = (Uint8)(rgba[3] * alpha);
			pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(surface->format), SDL_GetSurfacePalette(surface), rgba[0], rgba[1], rgba[2], rgba[3]);
			putpixel(this->surface, x, y, pixel);
		}
	}
	SDL_UnlockSurface(this->surface);
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image linearFade time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image linearFade total %d\n", total);
#endif
}


Gigalomania::Image *Gigalomania::Image::copy(int x, int y, int w, int h) const {
	//LOG("Image::copy(%d,%d,%d,%d)\n",x,y,w,h);
#ifdef TIMING
	int time_s = clock();
#endif

	x = (int)(x * scale_x);
	y = (int)(y * scale_y);
	w = (int)(w * scale_x);
	h = (int)(h * scale_y);

	Gigalomania::Image *copy_image = NULL;
	{
		SDL_Surface *surface = SDL_CreateSurface(w, h, this->surface->format);
		copy_image = new Image();
		copy_image->surface = surface;
		copy_image->data = (unsigned char *)copy_image->surface->pixels;
		copy_image->need_to_free_data = false;
	}

	copy_image->copyPalette(this); // must be called before SDL_BlitSurface

	SDL_Rect src_rect;
	src_rect.x = x;
	src_rect.y = y;
	src_rect.w = w;
	src_rect.h = h;
	SDL_SetSurfaceBlendMode(this->surface, SDL_BLENDMODE_NONE);
	if( !SDL_BlitSurface(this->surface, &src_rect, copy_image->surface, NULL) ) {
		LOG("SDL_BlitSurface failed: %s\n", SDL_GetError());
	}
	/*unsigned char *src_data = (unsigned char *)this->surface->pixels;
	unsigned char *dst_data = (unsigned char *)copy_image->surface->pixels;
	SDL_LockSurface(this->surface);
	int bytesperpixel = this->surface->format->BytesPerPixel;
	// faster to read in x direction! (caching?)
	for(int cy=0;cy<h;cy++) {
		for(int cx=0;cx<w;cx++) {
			for(int i=0;i<bytesperpixel;i++) {
				int src_indx = ( y + cy ) * this->surface->pitch + ( x + cx ) * bytesperpixel + i;
				int dst_indx = cy * copy_image->surface->pitch + cx * bytesperpixel + i;
				ASSERT( src_indx >= 0 && src_indx < this->surface->pitch * this->surface->h * bytesperpixel );
				ASSERT( dst_indx >= 0 && dst_indx < copy_image->surface->pitch * copy_image->surface->h * copy_image->surface->format->BytesPerPixel );
				//if( dst_data[ dst_indx ] != src_data[ src_indx ] ) {
				//	LOG("not equal: %d, %d, %d: %d vs %d\n", cx, cy, i, src_data[ src_indx ], dst_data[ dst_indx ]);
				//}
				dst_data[ dst_indx ] = src_data[ src_indx ];
			}
		}
	}
	SDL_UnlockSurface(this->surface);*/

	copy_image->scale_x = scale_x;
	copy_image->scale_y = scale_y;

#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image copy time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image copy total %d\n", total);
#endif

	return copy_image;
}

Gigalomania::Image *Gigalomania::Image::loadImage(const char *filename) {
#ifdef TIMING
	int time_s = clock();
#endif
	//LOG("Image::loadImage(\"%s\")\n",filename); // disabled logging to improve performance on startup
	SDL_IOStream *src = SDL_IOFromFile(filename, "rb");
	if( src == NULL ) {
		LOG("failed to load: %s\n", filename);
		LOG("SDL_IOFromFile failed: %s\n", SDL_GetError());
		return NULL;
	}
	Gigalomania::Image *image = new Image();

	// workaround: IMG_Load_IO fails on IFF files with 2 bitplanes (bug in SDL2_image 2.0.8, still present in SDL3_image)
	if( strstr(filename, ".") == NULL ) {
		LOG("load as IFF, using local function\n");
		image->surface = my_IMG_LoadLBM_RW(src);
		SDL_CloseIO(src);
	}
	if( image->surface == NULL ) {
		image->surface = IMG_Load_IO(src, 1);
	}

	if( image->surface == NULL ) {
		LOG("failed to load: %s\n", filename);
		LOG("IMG_Load_IO failed: %s\n", SDL_GetError());
		delete image;
		image = NULL;
	}
#ifdef TIMING
	int time_taken = clock() - time_s;
	LOG("    image load time %d\n", time_taken);
	static int total = 0;
	total += time_taken;
	LOG("    image load total %d\n", total);
#endif


	return image;
}

Gigalomania::Image *Gigalomania::Image::createBlankImage(int width,int height, int bpp) {
	Uint32 rmask, gmask, bmask, amask;
	CreateMask(rmask, gmask, bmask, amask);

	SDL_Surface *surface = SDL_CreateSurface(width, height, SDL_GetPixelFormatForMasks(bpp, rmask, gmask, bmask, amask));

	Gigalomania::Image *image = new Image();
	image->surface = surface;
	image->data = (unsigned char *)image->surface->pixels;
	image->need_to_free_data = false;

	return image;
}

Gigalomania::Image *Gigalomania::Image::createNoise(int w,int h,float scale_u,float scale_v,const unsigned char filter_max[3],const unsigned char filter_min[3],NOISEMODE_t noisemode,int n_iterations) {
	Gigalomania::Image *image = Gigalomania::Image::createBlankImage(w, h, 32);
	SDL_LockSurface(image->surface);
	float fvec[2] = {0.0f, 0.0f};
	for(int y=0;y<h;y++) {
		fvec[0] = scale_v * ((float)y) / ((float)h - 1.0f);
		for(int x=0;x<w;x++) {
			fvec[1] = scale_u * ((float)x) / ((float)w - 1.0f);
			float h = 0.0f;
			float max_val = 0.0f;
			float mult = 1.0f;
			for(int j=0;j<n_iterations;j++,mult*=2.0f) {
				float this_fvec[2];
				this_fvec[0] = fvec[0] * mult;
				this_fvec[1] = fvec[1] * mult;
				float this_h = perlin_noise2(this_fvec) / mult;
				if( noisemode == NOISEMODE_PATCHY || noisemode == NOISEMODE_MARBLE )
					this_h = abs(this_h);
				h += this_h;
				max_val += 1.0f / mult;
			}
			if( noisemode == NOISEMODE_PATCHY ) {
				h /= max_val;
			}
			else if( noisemode == NOISEMODE_MARBLE ) {
				h = sin(scale_u * ((float)x) / ((float)w - 1.0f) + h);
				h = 0.5f + 0.5f * h;
			}
			else {
				h /= max_val;
				h = 0.5f + 0.5f * h;
			}

			if( noisemode == NOISEMODE_CLOUDS ) {
				//const float offset = 0.4f;
				//const float offset = 0.3f;
				const float offset = 0.2f;
				h = offset - h * h;
				h = max(h, 0.0f);
				h /= offset;
			}
			// h is now in range [0, 1]
			if( h < 0.0 || h > 1.0 ) {
				LOG("h value is out of bounds\n");
				ASSERT(false);
			}
			if( noisemode == NOISEMODE_WOOD ) {
				h = 20 * h;
				h = h - floor(h);
			}
			Uint8 r = (Uint8)((filter_max[0] - filter_min[0]) * h + filter_min[0]);
			Uint8 g = (Uint8)((filter_max[1] - filter_min[1]) * h + filter_min[1]);
			Uint8 b = (Uint8)((filter_max[2] - filter_min[2]) * h + filter_min[2]);
			Uint8 a = 255;
			Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(image->surface->format), SDL_GetSurfacePalette(image->surface), r, g, b, a);
			putpixel(image->surface, x, y, pixel);
		}
	}
	SDL_UnlockSurface(image->surface);

	return image;
}

Gigalomania::Image * Gigalomania::Image::createRadial(int w,int h,float alpha_scale) {
	return createRadial(w, h, alpha_scale, 255, 255, 255);
}

Gigalomania::Image * Gigalomania::Image::createRadial(int w,int h,float alpha_scale, Uint8 r, Uint8 g, Uint8 b) {
	Gigalomania::Image *image = Gigalomania::Image::createBlankImage(w, h, 32);
	SDL_LockSurface(image->surface);
	int radius = min(w/2, h/2);
	for(int y=0;y<h;y++) {
		int dy = abs(y - h/2);
		for(int x=0;x<w;x++) {
			int dx = abs(x - w/2);
			float dist = sqrt((float)(dx*dx + dy*dy));
			dist /= (float)radius;
			if( dist >= 1.0f )
				dist = 1.0f;
			dist = 1.0f - dist;
			dist *= alpha_scale;
			unsigned char v = (int)(255.0f * dist);
			Uint8 a = v;
			Uint32 pixel = SDL_MapRGBA(SDL_GetPixelFormatDetails(image->surface->format), SDL_GetSurfacePalette(image->surface), r, g, b, a);
			putpixel(image->surface, x, y, pixel);
		}
	}

	SDL_UnlockSurface(image->surface);
	return image;
}

void Gigalomania::Image::setGraphicsOutput(SDL_Renderer *sdlRenderer) {
	Gigalomania::Image::sdlRenderer = sdlRenderer;
}

void Gigalomania::Image::writeNumbers(int x,int y,Gigalomania::Image *images[10],int number,Justify justify) {
	char buffer[16] = "";
	snprintf(buffer, sizeof(buffer), "%d",number);
	int len = (int)strlen(buffer);
	int w = images[0]->getScaledWidth();
	int sx = 0;
	if( justify == JUSTIFY_LEFT )
		sx = x;
	else if( justify == JUSTIFY_CENTRE )
		sx = x - ( w * len ) / 2;
	else if( justify == JUSTIFY_RIGHT )
		sx = x - w * len;

	for(int i=0;i<len;i++) {
		images[ buffer[i] - '0' ]->draw(sx, y);
		sx += w;
	}
}

void Gigalomania::Image::write(int x,int y,Gigalomania::Image *images[n_font_chars_c],const char *text,Justify justify) {
	writeMixedCase(x, y, images, images, NULL, text, justify);
}

void Gigalomania::Image::writeMixedCase(int x,int y,Gigalomania::Image *large[n_font_chars_c],Gigalomania::Image *little[n_font_chars_c],Gigalomania::Image *numbers[10],const char *text,Justify justify) {
	int len = (int)strlen(text);
	int n_lines = 0;
	int s_w = little[0]->getScaledWidth();
	int l_w = large[0]->getScaledWidth();
	int max_wid = 0;
	textLines(&n_lines, &max_wid, text, s_w, l_w);
	int n_h = 0;
	if( numbers != NULL )
		n_h = numbers[0]->getScaledHeight();
	int s_h = little[0]->getScaledHeight();
	int l_h = large[0]->getScaledHeight();
	int sx = 0;
	if( justify == JUSTIFY_LEFT )
		sx = x;
	else if( justify == JUSTIFY_CENTRE )
		sx = x - max_wid / 2;
	else if( justify == JUSTIFY_RIGHT )
		sx = x - max_wid;
	int cx = sx;

	for(int i=0;i<len;i++) {
		char ch = text[i];
		bool was_large = false;
		if( numbers == NULL && ch == '0' ) {
			ch = 'O'; // hack for 0 (we don't spell it O, due to alphabetical ordering)
		}
		if( ch == '\n' ) {
			// newline
			cx = sx;
			y += l_h + 2;
			continue; // don't increase cx
		}
		else if( isspace( ch ) )
			; // do nothing
		else if( ch >= '0' && ch <= '9' ) {
			ASSERT( numbers != NULL );
			int indx = ch - '0';
			numbers[indx]->draw(cx, y + l_h - n_h);
		}
		else if( isupper( ch ) ) {
			int indx = ch - 'A';
			large[indx]->draw(cx, y);
			was_large = true;
		}
		else if( islower( ch ) ) {
			little[ ch - 'a' ]->draw(cx, y + l_h - s_h);
		}
		else if( ch == '.' ) {
			if( little[font_index_period_c] != NULL )
				little[font_index_period_c]->draw(cx, y + l_h - s_h);
			else if( large[font_index_period_c] != NULL )
				large[font_index_period_c]->draw(cx, y);
		}
		else if( ch == ',' ) {
			if( little[font_index_comma_c] != NULL )
				little[font_index_comma_c]->draw(cx, y + l_h - s_h);
			else if( large[font_index_comma_c] != NULL )
				large[font_index_comma_c]->draw(cx, y);
		}
		else if( ch == '\'' ) {
			if( little[font_index_apostrophe_c] != NULL )
				little[font_index_apostrophe_c]->draw(cx, y + l_h - s_h);
			else if( large[font_index_apostrophe_c] != NULL )
				large[font_index_apostrophe_c]->draw(cx, y);
		}
		else if( ch == '!' ) {
			if( little[font_index_exclamation_c] != NULL )
				little[font_index_exclamation_c]->draw(cx, y + l_h - s_h);
			else if( large[font_index_exclamation_c] != NULL )
				large[font_index_exclamation_c]->draw(cx, y);
		}
		else if( ch == '?' ) {
			if( little[font_index_question_c] != NULL )
				little[font_index_question_c]->draw(cx, y + l_h - s_h);
			else if( large[font_index_question_c] != NULL )
				large[font_index_question_c]->draw(cx, y);
		}
		else if( ch == '-' ) {
			if( little[font_index_dash_c] != NULL )
				little[font_index_dash_c]->draw(cx, y + l_h - s_h);
			else if( large[font_index_dash_c] != NULL )
				large[font_index_dash_c]->draw(cx, y);
		}
		else {
			continue; // don't increase cx
		}
		cx += was_large ? l_w : s_w;
	}
}

void Gigalomania::Image::smooth() {
	if( SDL_BITSPERPIXEL(this->surface->format) != 24 && SDL_BITSPERPIXEL(this->surface->format) != 32 ) {
		return;
	}
	int w = getWidth();
	int h = getHeight();
	unsigned char *src_data = (unsigned char *)this->surface->pixels;
	int bytesperpixel = SDL_BYTESPERPIXEL(this->surface->format);
	int pitch = this->surface->pitch;
	unsigned char *new_data = new unsigned char[w * h * bytesperpixel];

	SDL_LockSurface(this->surface);
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			for(int i=0;i<bytesperpixel;i++) {
				Uint32 col = 0;
				if(	x > 0 && x < w-1 && y > 0 && y < h-1 ) {
						Uint32 sq[9];
						int indx = 0;
						for(int sx=x-1;sx<=x+1;sx++) {
							for(int sy=y-1;sy<=y+1;sy++) {
								sq[indx++] = src_data[ sy * pitch + sx * bytesperpixel + i ];
							}
						}
						col = ( sq[0] + 2 * sq[1] + sq[2] + 2 * sq[3] + 4 * sq[4] + 2 * sq[5] + sq[6] + 2 * sq[7] + sq[8] ) / 16;
						//col = ( sq[1] + sq[3] + sq[5] + sq[7] + 12 * sq[4] ) / 16;
				}
				else
					col = src_data[ y * pitch + x * bytesperpixel + i ];
				new_data[ y * w * bytesperpixel + x * bytesperpixel + i] = (unsigned char)col;
			}
		}
	}
	for(int y=0;y<h;y++) {
		for(int x=0;x<w;x++) {
			for(int i=0;i<bytesperpixel;i++) {
				src_data[ y * pitch + x * bytesperpixel + i ] = new_data[ y * w * bytesperpixel + x * bytesperpixel + i];
			}
		}
	}
	delete [] new_data;
	SDL_UnlockSurface(this->surface);
}

/* Locally patched version of IMG_LoadLBM_RW from IMG_lbm.c from SDL_image 2.0.8, to fix problem where
   IFF images with 2 bit planes are no longer supported.

   Original copyright/licence message follows.
*/

/*
  SDL_image:  An example image loading library for use with SDL
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#define MAXCOLORS 256

/* Structure for an IFF picture ( BMHD = Bitmap Header ) */

typedef struct
{
	Uint16 w, h;        /* width & height of the bitmap in pixels */
	Sint16 x, y;        /* screen coordinates of the bitmap */
	Uint8 planes;       /* number of planes of the bitmap */
	Uint8 mask;         /* mask type ( 0 => no mask ) */
	Uint8 tcomp;        /* compression type */
	Uint8 pad1;         /* dummy value, for padding */
	Uint16 tcolor;      /* transparent color */
	Uint8 xAspect,      /* pixel aspect ratio */
		yAspect;
	Sint16  Lpage;      /* width of the screen in pixels */
	Sint16  Hpage;      /* height of the screen in pixels */
} BMHD;

SDL_Surface *my_IMG_LoadLBM_RW( SDL_IOStream *src )
{
    Sint64 start;
    SDL_Surface *Image;
    Uint8       id[4], pbm, colormap[MAXCOLORS*3], *MiniBuf, *ptr, count, color, msk;
    Uint32      size, bytesloaded, nbcolors;
    Uint32      i, j, bytesperline, nbplanes, stencil, plane, h;
    Uint32      remainingbytes;
    Uint32      width;
    BMHD          bmhd;
    const char  *error;
    Uint8       flagHAM,flagEHB;

    Image   = NULL;
    error   = NULL;
    MiniBuf = NULL;

    if ( !src ) {
        /* The error message has been set in SDL_IOFromFile */
        return NULL;
    }
    start = SDL_TellIO(src);

    if ( SDL_ReadIO( src, id, 4 ) != 4 )
    {
        error="error reading IFF chunk";
        goto done;
    }

    /* Should be the size of the file minus 4+4 ( 'FORM'+size ) */
    if ( SDL_ReadIO( src, &size, 4 ) != 4 )
    {
        error="error reading IFF chunk size";
        goto done;
    }

    /* As size is not used here, no need to swap it */

    if ( SDL_memcmp( id, "FORM", 4 ) != 0 )
    {
        error="not a IFF file";
        goto done;
    }

    if ( SDL_ReadIO( src, id, 4 ) != 4 )
    {
        error="error reading IFF chunk";
        goto done;
    }

    pbm = 0;

    /* File format : PBM=Packed Bitmap, ILBM=Interleaved Bitmap */
    if ( !SDL_memcmp( id, "PBM ", 4 ) ) pbm = 1;
    else if ( SDL_memcmp( id, "ILBM", 4 ) )
    {
        error="not a IFF picture";
        goto done;
    }

    nbcolors = 0;

    SDL_memset( &bmhd, 0, sizeof( BMHD ) );
    flagHAM = 0;
    flagEHB = 0;

    while ( SDL_memcmp( id, "BODY", 4 ) != 0 )
    {
        if ( SDL_ReadIO( src, id, 4 ) != 4 )
        {
            error="error reading IFF chunk";
            goto done;
        }

        if ( SDL_ReadIO( src, &size, 4 ) != 4 )
        {
            error="error reading IFF chunk size";
            goto done;
        }

        bytesloaded = 0;

        size = SDL_Swap32BE( size );

        if ( !SDL_memcmp( id, "BMHD", 4 ) ) /* Bitmap header */
        {
            if ( SDL_ReadIO( src, &bmhd, sizeof( BMHD ) ) != sizeof( BMHD ) )
            {
                error="error reading BMHD chunk";
                goto done;
            }

            bytesloaded = sizeof( BMHD );

            bmhd.w      = SDL_Swap16BE( bmhd.w );
            bmhd.h      = SDL_Swap16BE( bmhd.h );
            bmhd.x      = SDL_Swap16BE( bmhd.x );
            bmhd.y      = SDL_Swap16BE( bmhd.y );
            bmhd.tcolor = SDL_Swap16BE( bmhd.tcolor );
            bmhd.Lpage  = SDL_Swap16BE( bmhd.Lpage );
            bmhd.Hpage  = SDL_Swap16BE( bmhd.Hpage );
        }

        if ( !SDL_memcmp( id, "CMAP", 4 ) ) /* palette ( Color Map ) */
        {
            if (size > sizeof (colormap)) {
                error="colormap size is too large";
                goto done;
            }

            if ( SDL_ReadIO( src, &colormap, size ) != size )
            {
                error="error reading CMAP chunk";
                goto done;
            }

            bytesloaded = size;
            nbcolors = size / 3;
        }

        if ( !SDL_memcmp( id, "CAMG", 4 ) ) /* Amiga ViewMode  */
        {
            Uint32 viewmodes;
            if ( SDL_ReadIO( src, &viewmodes, sizeof(viewmodes) ) != sizeof(viewmodes) )
            {
                error="error reading CAMG chunk";
                goto done;
            }

            bytesloaded = size;
            viewmodes = SDL_Swap32BE( viewmodes );
            if ( viewmodes & 0x0800 )
                flagHAM = 1;
            if ( viewmodes & 0x0080 )
                flagEHB = 1;
        }

        if ( SDL_memcmp( id, "BODY", 4 ) )
        {
            if ( size & 1 ) ++size;     /* padding ! */
            size -= bytesloaded;
            /* skip the remaining bytes of this chunk */
            if ( size ) SDL_SeekIO( src, size, SDL_IO_SEEK_CUR );
        }
    }

    /* compute some usefull values, based on the bitmap header */

    width = ( bmhd.w + 15 ) & 0xFFFFFFF0;  /* Width in pixels modulo 16 */

    bytesperline = ( ( bmhd.w + 15 ) / 16 ) * 2;

    nbplanes = bmhd.planes;

    if ( pbm )                         /* File format : 'Packed Bitmap' */
    {
        bytesperline *= 8;
        nbplanes = 1;
    }

    if ((nbplanes != 1) && (nbplanes != 2) && (nbplanes != 4) && (nbplanes != 8) && (nbplanes != 24))
    {
        error="unsupported number of color planes";
        goto done;
    }

    stencil = (bmhd.mask & 1);   /* There is a mask ( 'stencil' ) */

    /* Allocate memory for a temporary buffer ( used for
           decompression/deinterleaving ) */

    MiniBuf = (Uint8 *)SDL_malloc( bytesperline * (nbplanes + stencil) );
    if ( MiniBuf == NULL )
    {
        error="not enough memory for temporary buffer";
        goto done;
    }

    if ( ( Image = SDL_CreateSurface( width, bmhd.h, (nbplanes==24 || flagHAM==1) ? SDL_PIXELFORMAT_RGB24 : SDL_PIXELFORMAT_INDEX8 ) ) == NULL )
       goto done;

    if ( bmhd.mask & 2 )               /* There is a transparent color */
        SDL_SetSurfaceColorKey( Image, true, bmhd.tcolor );

    /* Update palette informations */

    /* There is no palette in 24 bits ILBM file */
    if ( nbcolors>0 && flagHAM==0 )
    {
        /* FIXME: Should this include the stencil? See comment below */
        int nbrcolorsfinal = 1 << (nbplanes + stencil);
        ptr = &colormap[0];

        for ( i=0; i<nbcolors; i++ )
        {
            SDL_GetSurfacePalette(Image)->colors[i].r = *ptr++;
            SDL_GetSurfacePalette(Image)->colors[i].g = *ptr++;
            SDL_GetSurfacePalette(Image)->colors[i].b = *ptr++;
        }

        /* Amiga EHB mode (Extra-Half-Bright) */
        /* 6 bitplanes mode with a 32 colors palette */
        /* The 32 last colors are the same but divided by 2 */
        /* Some Amiga pictures save 64 colors with 32 last wrong colors, */
        /* they shouldn't !, and here we overwrite these 32 bad colors. */
        if ( (nbcolors==32 || flagEHB ) && (1<<nbplanes)==64 )
        {
            nbcolors = 64;
            ptr = &colormap[0];
            for ( i=32; i<64; i++ )
            {
                SDL_GetSurfacePalette(Image)->colors[i].r = (*ptr++)/2;
                SDL_GetSurfacePalette(Image)->colors[i].g = (*ptr++)/2;
                SDL_GetSurfacePalette(Image)->colors[i].b = (*ptr++)/2;
            }
        }

        /* If nbcolors < 2^nbplanes, repeat the colormap */
        /* This happens when pictures have a stencil mask */
        if ( nbrcolorsfinal > (1<<nbplanes) ) {
            nbrcolorsfinal = (1<<nbplanes);
        }
        for ( i=nbcolors; i < (Uint32)nbrcolorsfinal; i++ )
        {
            SDL_GetSurfacePalette(Image)->colors[i].r = SDL_GetSurfacePalette(Image)->colors[i%nbcolors].r;
            SDL_GetSurfacePalette(Image)->colors[i].g = SDL_GetSurfacePalette(Image)->colors[i%nbcolors].g;
            SDL_GetSurfacePalette(Image)->colors[i].b = SDL_GetSurfacePalette(Image)->colors[i%nbcolors].b;
        }
        if ( !pbm )
            SDL_GetSurfacePalette(Image)->ncolors = nbrcolorsfinal;
    }

    /* Get the bitmap */

    for ( h=0; h < bmhd.h; h++ )
    {
        /* uncompress the datas of each planes */

        for ( plane=0; plane < (nbplanes+stencil); plane++ )
        {
            ptr = MiniBuf + ( plane * bytesperline );

            remainingbytes = bytesperline;

            if ( bmhd.tcomp == 1 )      /* Datas are compressed */
            {
                do
                {
                    if ( SDL_ReadIO( src, &count, 1 ) != 1 )
                    {
                        error="error reading BODY chunk";
                        goto done;
                    }

                    if ( count & 0x80 )
                    {
                        count ^= 0xFF;
                        count += 2; /* now it */

                        if ( ( count > remainingbytes ) || SDL_ReadIO( src, &color, 1 ) != 1 )
                        {
                            error="error reading BODY chunk";
                            goto done;
                        }
                        SDL_memset( ptr, color, count );
                    }
                    else
                    {
                        ++count;

                        if ( ( count > remainingbytes ) || SDL_ReadIO( src, ptr, count ) != count )
                        {
                           error="error reading BODY chunk";
                            goto done;
                        }
                    }

                    ptr += count;
                    remainingbytes -= count;

                } while ( remainingbytes > 0 );
            }
            else
            {
                if ( SDL_ReadIO( src, ptr, bytesperline ) != bytesperline )
                {
                    error="error reading BODY chunk";
                    goto done;
                }
            }
        }

        /* One line has been read, store it ! */

        ptr = (Uint8 *)Image->pixels;
        if ( nbplanes==24 || flagHAM==1 )
            ptr += h * width * 3;
        else
            ptr += h * width;

        if ( pbm )                 /* File format : 'Packed Bitmap' */
        {
           SDL_memcpy( ptr, MiniBuf, width );
        }
        else        /* We have to un-interlace the bits ! */
        {
            if ( nbplanes!=24 && flagHAM==0 )
            {
                size = ( width + 7 ) / 8;

                for ( i=0; i < size; i++ )
                {
                    SDL_memset( ptr, 0, 8 );

                    for ( plane=0; plane < (nbplanes + stencil); plane++ )
                    {
                        color = *( MiniBuf + i + ( plane * bytesperline ) );
                        msk = 0x80;

                        for ( j=0; j<8; j++ )
                        {
                            if ( ( plane + j ) <= 7 ) ptr[j] |= (Uint8)( color & msk ) >> ( 7 - plane - j );
                            else                        ptr[j] |= (Uint8)( color & msk ) << ( plane + j - 7 );

                            msk >>= 1;
                        }
                    }
                    ptr += 8;
                }
            }
            else
            {
                Uint32 finalcolor = 0;
                size = ( width + 7 ) / 8;
                /* 24 bitplanes ILBM : R0...R7,G0...G7,B0...B7 */
                /* or HAM (6 bitplanes) or HAM8 (8 bitplanes) modes */
                for ( i=0; i<width; i=i+8 )
                {
                    Uint8 maskBit = 0x80;
                    for ( j=0; j<8; j++ )
                    {
                        Uint32 pixelcolor = 0;
                        Uint32 maskColor = 1;
                        Uint8 dataBody;
                        for ( plane=0; plane < nbplanes; plane++ )
                        {
                            dataBody = MiniBuf[ plane*size+i/8 ];
                            if ( dataBody&maskBit )
                                pixelcolor = pixelcolor | maskColor;
                            maskColor = maskColor<<1;
                        }
                        /* HAM : 12 bits RGB image (4 bits per color component) */
                        /* HAM8 : 18 bits RGB image (6 bits per color component) */
                        if ( flagHAM )
                        {
                            switch( pixelcolor>>(nbplanes-2) )
                            {
                                case 0: /* take direct color from palette */
                                    finalcolor = colormap[ pixelcolor*3 ] + (colormap[ pixelcolor*3+1 ]<<8) + (colormap[ pixelcolor*3+2 ]<<16);
                                    break;
                                case 1: /* modify only blue component */
                                    finalcolor = finalcolor&0x00FFFF;
                                    finalcolor = finalcolor | (pixelcolor<<(16+(10-nbplanes)));
                                    break;
                                case 2: /* modify only red component */
                                    finalcolor = finalcolor&0xFFFF00;
                                    finalcolor = finalcolor | pixelcolor<<(10-nbplanes);
                                    break;
                                case 3: /* modify only green component */
                                    finalcolor = finalcolor&0xFF00FF;
                                    finalcolor = finalcolor | (pixelcolor<<(8+(10-nbplanes)));
                                    break;
                            }
                        }
                        else
                        {
                            finalcolor = pixelcolor;
                        }
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
                            *ptr++ = (Uint8)(finalcolor>>16);
                            *ptr++ = (Uint8)(finalcolor>>8);
                            *ptr++ = (Uint8)(finalcolor);
#else
                            *ptr++ = (Uint8)(finalcolor);
                            *ptr++ = (Uint8)(finalcolor>>8);
                            *ptr++ = (Uint8)(finalcolor>>16);
#endif
                        maskBit = maskBit>>1;
                    }
                }
            }
        }
    }

done:

    if ( MiniBuf ) SDL_free( MiniBuf );

    if ( error )
    {
        SDL_SeekIO(src, start, SDL_IO_SEEK_SET);
        if ( Image ) {
            SDL_DestroySurface( Image );
            Image = NULL;
        }
        SDL_SetError( "%s", error );
    }

    return( Image );
}
