/*
HUD_RENDERER_LUA.CPP

    Copyright (C) 2009 by Jeremiah Morris and the Aleph One developers

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    This license is contained in the file "COPYING",
    which is included with this source code; it is available online at
    http://www.gnu.org/licenses/gpl.html

    Implements HUD helper class for Lua HUD themes
*/
#include "config.h"
#ifdef HAVE_LUA

#include "HUDRenderer_Lua.h"

#include "FontHandler.h"
#include "Image_Blitter.h"
#include "Shape_Blitter.h"

#include "lua_hud_script.h"
#include "shell.h"
#include "screen.h"

#ifdef HAVE_OPENGL
# if defined (__APPLE__) && defined (__MACH__)
#   include <OpenGL/gl.h>
# else
#   include <GL/gl.h>
# endif
#endif

#include "OGL_Setup.h" // for SglColor*

#include <math.h>

#if defined(__WIN32__) || defined(__MINGW32__)
#undef DrawText
#endif

extern bool MotionSensorActive;


// Rendering object
static HUD_Lua_Class HUD_Lua;


HUD_Lua_Class *Lua_HUDInstance()
{
	return &HUD_Lua;
}

void Lua_DrawHUD(short time_elapsed)
{
	HUD_Lua.update_motion_sensor(time_elapsed);
	HUD_Lua.start_draw();
	L_Call_HUDDraw();
	HUD_Lua.end_draw();
}

/*
 *  Update motion sensor
 */

void HUD_Lua_Class::update_motion_sensor(short time_elapsed)
{
	if (!(GET_GAME_OPTIONS() & _motion_sensor_does_not_work) && MotionSensorActive) {
		if (time_elapsed == NONE)
			reset_motion_sensor(current_player_index);
		motion_sensor_scan(time_elapsed);
	}
}

void HUD_Lua_Class::clear_entity_blips(void)
{
	m_blips.clear();
}

void HUD_Lua_Class::add_entity_blip(short mtype, short intensity, short x, short y)
{
	world_point2d origin, target;
	origin.x = origin.y = 0;
	target.x = x;
	target.y = y;
	
	blip_info info;
	info.mtype = mtype;
	info.intensity = intensity;
	info.distance = distance2d(&origin, &target);
	info.direction = arctangent(x, y);
	
	m_blips.push_back(info);
}

size_t HUD_Lua_Class::entity_blip_count(void)
{
	return m_blips.size();
}

blip_info HUD_Lua_Class::entity_blip(size_t index)
{
	return m_blips[index];
}

void HUD_Lua_Class::start_draw(void)
{
	alephone::Screen *scr = alephone::Screen::instance();
    m_wr = scr->window_rect();
	m_opengl = (get_screen_mode()->acceleration == _opengl_acceleration);
	
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_ALPHA_TEST);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_FOG);
        
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glTranslatef(m_wr.x, m_wr.y, 0.0);
		
		m_surface = NULL;
	}
	else
#endif
	{
		if (m_surface &&
				(m_surface->w != SDL_GetVideoSurface()->w ||
				 m_surface->h != SDL_GetVideoSurface()->h))
		{
			SDL_FreeSurface(m_surface);
			m_surface = NULL;
		}
		if (!m_surface)
		{
			m_surface = SDL_DisplayFormatAlpha(SDL_GetVideoSurface());
			SDL_SetAlpha(m_surface, SDL_SRCALPHA, 0);
		}
		SDL_SetClipRect(m_surface, NULL);
		SDL_FillRect(m_surface, NULL, SDL_MapRGBA(m_surface->format, 0, 0, 0, 0));
		
		SDL_SetAlpha(SDL_GetVideoSurface(), SDL_SRCALPHA, 0xff);
	}
	
	
	m_drawing = true;
}

void HUD_Lua_Class::end_draw(void)
{
	m_drawing = false;
	
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
		glPopAttrib();
	}
	else
#endif
	if (m_surface)
	{
//		SDL_BlitSurface(m_surface, NULL, SDL_GetVideoSurface(), NULL);
		SDL_SetAlpha(SDL_GetVideoSurface(), 0, 0xff);
		SDL_SetClipRect(SDL_GetVideoSurface(), 0);
	}
}

void HUD_Lua_Class::apply_clip(void)
{
	alephone::Screen *scr = alephone::Screen::instance();
	
    SDL_Rect r;
    r.x = m_wr.x + scr->lua_clip_rect.x;
    r.y = m_wr.y + scr->lua_clip_rect.y;
    r.w = MIN(scr->lua_clip_rect.w, m_wr.w - scr->lua_clip_rect.x);
    r.h = MIN(scr->lua_clip_rect.h, m_wr.h - scr->lua_clip_rect.y);
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
        glEnable(GL_SCISSOR_TEST);
        glScissor(r.x, scr->height() - r.y - r.h, r.w, r.h);
	}
	else
#endif
	if (m_surface)
	{
		SDL_SetClipRect(SDL_GetVideoSurface(), &r);
	}
}

void HUD_Lua_Class::fill_rect(float x, float y, float w, float h,
															float r, float g, float b, float a)
{
	if (!m_drawing)
		return;
	
	apply_clip();
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		if (Using_sRGB)
			glDisable(GL_FRAMEBUFFER_SRGB_EXT);
		glColor4f(r, g, b, a);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);

		glVertex2f(x,     y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + h);
		glVertex2f(x,     y + h);

		glEnd();
		glEnable(GL_TEXTURE_2D);
		if (Using_sRGB)
			glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	}
	else
#endif
	if (m_surface)
	{
		SDL_Rect rect;
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.y = static_cast<Sint16>(y) + m_wr.y;
		rect.w = static_cast<Uint16>(w);
		rect.h = static_cast<Uint16>(h);
		SDL_FillRect(m_surface, &rect,
								 SDL_MapRGBA(m_surface->format, static_cast<unsigned char>(r * 255), static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255), static_cast<unsigned char>(a * 255)));
		SDL_BlitSurface(m_surface, &rect, SDL_GetVideoSurface(), &rect);
	}
}	

void HUD_Lua_Class::frame_rect(float x, float y, float w, float h,
												 			 float r, float g, float b, float a,
															 float t)
{
	if (!m_drawing)
		return;
		
	apply_clip();
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		if (Using_sRGB)
			glDisable(GL_FRAMEBUFFER_SRGB_EXT);
		glColor4f(r, g, b, a);
		glDisable(GL_TEXTURE_2D);
		glBegin(GL_QUADS);
		
		glVertex2f(x,     y);
		glVertex2f(x + w, y);
		glVertex2f(x + w, y + t);
		glVertex2f(x,     y + t);
		
		glVertex2f(x,     y + h - t);
		glVertex2f(x + w, y + h - t);
		glVertex2f(x + w, y + h);
		glVertex2f(x,     y + h);
		
		glVertex2f(x,     y + t);
		glVertex2f(x + t, y + t);
		glVertex2f(x + t, y + h - t);
		glVertex2f(x,     y + h - t);
		
		glVertex2f(x + w - t, y + t);
		glVertex2f(x + w,     y + t);
		glVertex2f(x + w,     y + h - t);
		glVertex2f(x + w - t, y + h - t);
		
		glEnd();
		glEnable(GL_TEXTURE_2D);
		if (Using_sRGB)
			glEnable(GL_FRAMEBUFFER_SRGB_EXT);
	}
	else
#endif
	if (m_surface)
	{
		Uint32 color = SDL_MapRGBA(m_surface->format, static_cast<unsigned char>(r * 255), static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255), static_cast<unsigned char>(a * 255));
		SDL_Rect rect;
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.w = static_cast<Uint16>(w);
		rect.y = static_cast<Sint16>(y) + m_wr.y;
		rect.h = static_cast<Uint16>(t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, SDL_GetVideoSurface(), &rect);
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.w = static_cast<Uint16>(w);
		rect.y = static_cast<Sint16>(y + h - t) + m_wr.y;
		rect.h = static_cast<Uint16>(t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, SDL_GetVideoSurface(), &rect);
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.w = static_cast<Uint16>(t);
		rect.y = static_cast<Sint16>(y + t) + m_wr.y;
		rect.h = static_cast<Uint16>(h - t - t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, SDL_GetVideoSurface(), &rect);
		rect.x = static_cast<Sint16>(x + w - t) + m_wr.x;
		rect.w = static_cast<Uint16>(t);
		rect.y = static_cast<Sint16>(y + t) + m_wr.y;
		rect.h = static_cast<Uint16>(h - t - t);
		SDL_FillRect(m_surface, &rect, color);
		SDL_BlitSurface(m_surface, &rect, SDL_GetVideoSurface(), &rect);
	}
}	

void HUD_Lua_Class::draw_text(FontSpecifier *font, const char *text,
															float x, float y,
															float r, float g, float b, float a)
{
	if (!m_drawing)
		return;
	
	apply_clip();
#ifdef HAVE_OPENGL
	if (m_opengl)
	{
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(x, y + font->Height, 0);
		if (Using_sRGB)
			glDisable(GL_FRAMEBUFFER_SRGB_EXT);
		glColor4f(r, g, b, a);
		font->OGL_Render(text);
		glColor4f(1, 1, 1, 1);
		if (Using_sRGB)
			glEnable(GL_FRAMEBUFFER_SRGB_EXT);
		glPopMatrix();
	}
	else
#endif
	if (m_surface)
	{
		SDL_Rect rect;
		rect.x = static_cast<Sint16>(x) + m_wr.x;
		rect.y = static_cast<Sint16>(y) + m_wr.y;
		rect.w = font->TextWidth(text);
		rect.h = font->LineSpacing;
		SDL_BlitSurface(SDL_GetVideoSurface(), &rect, m_surface, &rect);
		font->Info->draw_text(m_surface, text, strlen(text),
												  static_cast<Sint16>(x) + m_wr.x, static_cast<Sint16>(y) + m_wr.y + font->Height,
												  SDL_MapRGBA(m_surface->format,
																		  static_cast<unsigned char>(r * 255), static_cast<unsigned char>(g * 255), static_cast<unsigned char>(b * 255), static_cast<unsigned char>(a * 255)),
												  font->Style);
	  SDL_BlitSurface(m_surface, &rect, SDL_GetVideoSurface(), &rect);
	}
}

void HUD_Lua_Class::draw_image(Image_Blitter *image, float x, float y)
{
	if (!m_drawing)
		return;
	
	SDL_Rect r;
	r.x = static_cast<Sint16>(x);
	r.y = static_cast<Sint16>(y);
	r.w = image->crop_rect.w;
	r.h = image->crop_rect.h;

	apply_clip();
    if (m_surface)
    {
        r.x += m_wr.x;
        r.y += m_wr.y;
    }
	image->Draw(SDL_GetVideoSurface(), r);
}

void HUD_Lua_Class::draw_shape(Shape_Blitter *shape, float x, float y)
{
	if (!m_drawing)
		return;
	
	SDL_Rect r;
	r.x = static_cast<Sint16>(x);
	r.y = static_cast<Sint16>(y);
	r.w = shape->crop_rect.w;
	r.h = shape->crop_rect.h;
    
	apply_clip();
#ifdef HAVE_OPENGL
    if (m_opengl)
    {
        shape->OGL_Draw(r);
    }
    else
#endif
    if (m_surface)
    {
        r.x += m_wr.x;
        r.y += m_wr.y;
        shape->SDL_Draw(SDL_GetVideoSurface(), r);
    }
}
#endif
