/*
LUA_HUD_OBJECTS.CPP

	Copyright (C) 2009 by Jeremiah Morris
 
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

    Implements Lua HUD objects and globals
*/
#include "config.h"
#ifdef HAVE_LUA

#include "lua_hud_objects.h"
#include "lua_objects.h"
#include "lua_map.h"
#include "lua_templates.h"

#include "items.h"
#include "player.h"
#include "motion_sensor.h"
#include "screen.h"
#include "shell.h"
#include "alephversion.h"
#include "HUDRenderer.h"
#include "HUDRenderer_Lua.h"
#include "network.h"
#include "FontHandler.h"
#include "render.h"
#include "Image_Blitter.h"
#include "OGL_Blitter.h"
#include "fades.h"
#include "OGL_Faders.h"
#include "Shape_Blitter.h"
#include "collection_definition.h"

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <algorithm>

#ifdef HAVE_LUA

extern struct view_data *world_view;

const float AngleConvert = 360/float(FULL_CIRCLE);

extern collection_definition *get_collection_definition(short);

static int Lua_Collection_Get_Bitmap_Count(lua_State *L)
{
	collection_definition *collection = get_collection_definition(Lua_Collection::Index(L, 1));
	lua_pushnumber(L, collection->bitmap_count);
	return 1;
}

const luaL_reg Lua_Collection_Get[] = {
{"bitmap_count", Lua_Collection_Get_Bitmap_Count},
{0, 0},
};


char Lua_InventorySection_Name[] = "inventory_section";
char Lua_InventorySections_Name[] = "InventorySections";

char Lua_RendererType_Name[] = "renderer_type";
char Lua_RendererTypes_Name[] = "RendererTypes";
#define NUMBER_OF_RENDERER_TYPES 2

char Lua_SensorBlipType_Name[] = "sensor_blip_type";
char Lua_SensorBlipTypes_Name[] = "SensorBlipTypes";

char Lua_TextureType_Name[] = "texture_type";
char Lua_TextureTypes_Name[] = "TextureTypes";

char Lua_SizePref_Name[] = "size_preference";
typedef L_Enum<Lua_SizePref_Name> Lua_SizePreference;

char Lua_SizePrefs_Name[] = "SizePreferences";
typedef L_EnumContainer<Lua_SizePrefs_Name, Lua_SizePreference> Lua_SizePreferences;
#define NUMBER_OF_SIZE_PREFERENCES 3

char Lua_FadeEffectType_Name[] = "fade_effect_type";
typedef L_Enum<Lua_FadeEffectType_Name> Lua_FadeEffectType;

char Lua_FadeEffectTypes_Name[] = "FadeEffectTypes";
typedef L_EnumContainer<Lua_FadeEffectTypes_Name, Lua_FadeEffectType> Lua_FadeEffectTypes;

extern char Lua_DifficultyType_Name[];
typedef L_Enum<Lua_DifficultyType_Name> Lua_DifficultyType;

extern char Lua_DifficultyTypes_Name[];
typedef L_EnumContainer<Lua_DifficultyTypes_Name, Lua_DifficultyType> Lua_DifficultyTypes;

extern char Lua_GameType_Name[];
typedef L_Enum<Lua_GameType_Name> Lua_GameType;

extern char Lua_GameTypes_Name[];
typedef L_EnumContainer<Lua_GameTypes_Name, Lua_GameType> Lua_GameTypes;

extern char Lua_ItemType_Name[];
static int Lua_ItemType_Get_Ball(lua_State *L)
{
	lua_pushboolean(L, (get_item_kind(Lua_ItemType::Index(L, 1)) == _ball));
	return 1;
}
const luaL_reg Lua_ItemType_Get[] = {
{"ball", Lua_ItemType_Get_Ball},
{0, 0}
};

extern char Lua_PlayerColor_Name[];
typedef L_Enum<Lua_PlayerColor_Name> Lua_PlayerColor;

extern char Lua_PlayerColors_Name[];
typedef L_EnumContainer<Lua_PlayerColors_Name, Lua_PlayerColor> Lua_PlayerColors;

extern char Lua_ScoringMode_Name[];
typedef L_Enum<Lua_ScoringMode_Name> Lua_ScoringMode;

extern char Lua_ScoringModes_Name[];
typedef L_Container<Lua_ScoringModes_Name, Lua_ScoringMode> Lua_ScoringModes;

extern char Lua_WeaponType_Name[];
typedef L_Enum<Lua_WeaponType_Name> Lua_WeaponType;

extern char Lua_WeaponTypes_Name[];
typedef L_EnumContainer<Lua_WeaponTypes_Name, Lua_WeaponType> Lua_WeaponTypes;


static float Lua_HUDColor_Lookup(lua_State *L, int pos, int idx, const char *name1, const char *name2)
{
	float ret = 1.0f;
	
	lua_pushstring(L, name1);
	lua_gettable(L, pos);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_pushstring(L, name2);
		lua_gettable(L, pos);
	}
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_pushinteger(L, idx);
		lua_gettable(L, pos);
	}
	if (!lua_isnil(L, -1))
		ret = static_cast<float>(lua_tonumber(L, -1));
	lua_pop(L, 1);
	return ret;
}

static float Lua_HUDColor_Get_R(lua_State *L, int pos)
{
	return Lua_HUDColor_Lookup(L, pos, 1, "r", "red");
}

static float Lua_HUDColor_Get_G(lua_State *L, int pos)
{
	return Lua_HUDColor_Lookup(L, pos, 2, "g", "green");
}

static float Lua_HUDColor_Get_B(lua_State *L, int pos)
{
	return Lua_HUDColor_Lookup(L, pos, 3, "b", "blue");
}

static float Lua_HUDColor_Get_A(lua_State *L, int pos)
{
	return Lua_HUDColor_Lookup(L, pos, 4, "a", "alpha");
}


char Lua_Image_Name[] = "image";
typedef L_ObjectClass<Lua_Image_Name, Image_Blitter *> Lua_Image;

char Lua_Image_Crop_Rect_Name[] = "image_crop_rect";
class Lua_Image_Crop_Rect : public L_Class<Lua_Image_Crop_Rect_Name>
{
public:
	static Image_Blitter *Object(lua_State *L, int index);
};

Image_Blitter *Lua_Image_Crop_Rect::Object(lua_State *L, int index)
{
	return Lua_Image::ObjectAtIndex(L, Lua_Image_Crop_Rect::Index(L, index));
}

static int Lua_Image_Crop_Rect_Get_X(lua_State *L)
{
	lua_pushnumber(L, Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.x);
	return 1;
}

static int Lua_Image_Crop_Rect_Get_Y(lua_State *L)
{
	lua_pushnumber(L, Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.y);
	return 1;
}

static int Lua_Image_Crop_Rect_Get_Width(lua_State *L)
{
	lua_pushnumber(L, Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.w);
	return 1;
}

static int Lua_Image_Crop_Rect_Get_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.h);
	return 1;
}

static int Lua_Image_Crop_Rect_Set_X(lua_State *L)
{
	Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.x = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Image_Crop_Rect_Set_Y(lua_State *L)
{
	Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.y = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Image_Crop_Rect_Set_Width(lua_State *L)
{
	Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.w = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Image_Crop_Rect_Set_Height(lua_State *L)
{
	Lua_Image_Crop_Rect::Object(L, 1)->crop_rect.h = lua_tointeger(L, 2);
  return 0;
}

const luaL_reg Lua_Image_Crop_Rect_Get[] = {
{"x", Lua_Image_Crop_Rect_Get_X},
{"y", Lua_Image_Crop_Rect_Get_Y},
{"width", Lua_Image_Crop_Rect_Get_Width},
{"height", Lua_Image_Crop_Rect_Get_Height},
{0, 0}
};

const luaL_reg Lua_Image_Crop_Rect_Set[] = {
{"x", Lua_Image_Crop_Rect_Set_X},
{"y", Lua_Image_Crop_Rect_Set_Y},
{"width", Lua_Image_Crop_Rect_Set_Width},
{"height", Lua_Image_Crop_Rect_Set_Height},
{0, 0}
};


static int Lua_Image_Get_Width(lua_State *L)
{
	lua_pushnumber(L, Lua_Image::Object(L, 1)->Width());
	return 1;
}

static int Lua_Image_Get_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Image::Object(L, 1)->Height());
	return 1;
}

static int Lua_Image_Get_Unscaled_Width(lua_State *L)
{
	lua_pushnumber(L, Lua_Image::Object(L, 1)->UnscaledWidth());
	return 1;
}

static int Lua_Image_Get_Unscaled_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Image::Object(L, 1)->UnscaledHeight());
	return 1;
}

static int Lua_Image_Get_Tint(lua_State *L)
{
	lua_newtable(L);
	lua_pushstring(L, "r");
	lua_pushnumber(L, Lua_Image::Object(L, 1)->tint_color_r);
	lua_settable(L, -3);
	lua_pushstring(L, "g");
	lua_pushnumber(L, Lua_Image::Object(L, 1)->tint_color_g);
	lua_settable(L, -3);
	lua_pushstring(L, "b");
	lua_pushnumber(L, Lua_Image::Object(L, 1)->tint_color_b);
	lua_settable(L, -3);
	lua_pushstring(L, "a");
	lua_pushnumber(L, Lua_Image::Object(L, 1)->tint_color_a);
	lua_settable(L, -3);
	return 1;
}


static int Lua_Image_Get_Rotation(lua_State *L)
{
	lua_pushnumber(L, Lua_Image::Object(L, 1)->rotation);
	return 1;
}

static int Lua_Image_Get_Crop_Rect(lua_State *L)
{
	Lua_Image_Crop_Rect::Push(L, Lua_Image::Index(L, 1));
	return 1;
}

int Lua_Image_Rescale(lua_State *L)
{
	Lua_Image::Object(L, 1)->Rescale(lua_tointeger(L, 2), lua_tointeger(L, 3));
	return 0;
}

int Lua_Image_Draw(lua_State *L)
{
	Lua_HUDInstance()->draw_image(Lua_Image::Object(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	return 0;
}

const luaL_reg Lua_Image_Get[] = {
{"width", Lua_Image_Get_Width},
{"height", Lua_Image_Get_Height},
{"unscaled_width", Lua_Image_Get_Unscaled_Width},
{"unscaled_height", Lua_Image_Get_Unscaled_Height},
{"tint_color", Lua_Image_Get_Tint},
{"rotation", Lua_Image_Get_Rotation},
{"crop_rect", Lua_Image_Get_Crop_Rect},
{"rescale", L_TableFunction<Lua_Image_Rescale>},
{"draw", L_TableFunction<Lua_Image_Draw>},
{0, 0}
};

static int Lua_Image_Set_Tint(lua_State *L)
{
	Lua_Image::Object(L, 1)->tint_color_r = Lua_HUDColor_Get_R(L, 2);
	Lua_Image::Object(L, 1)->tint_color_g = Lua_HUDColor_Get_G(L, 2);
	Lua_Image::Object(L, 1)->tint_color_b = Lua_HUDColor_Get_B(L, 2);
	Lua_Image::Object(L, 1)->tint_color_a = Lua_HUDColor_Get_A(L, 2);
	return 0;
}

static int Lua_Image_Set_Rotation(lua_State *L)
{
	Lua_Image::Object(L, 1)->rotation = lua_tonumber(L, 2);
	return 0;
}

const luaL_reg Lua_Image_Set[] = {
{"tint_color", Lua_Image_Set_Tint},
{"rotation", Lua_Image_Set_Rotation},
{0, 0}
};

static int Lua_Image_GC(lua_State *L)
{
	delete Lua_Image::Object(L, 1);
	Lua_Image::Invalidate(L, Lua_Image::Index(L, 1));
	return 0;
}

const luaL_reg Lua_Image_Metatable[] = {
{"__gc", Lua_Image_GC},
{0, 0}
};

char Lua_Images_Name[] = "Images";
typedef L_Class<Lua_Images_Name> Lua_Images;

int Lua_Images_New(lua_State *L)
{
    // read resource argument
    lua_pushstring(L, "resource");
    lua_gettable(L, 1);
    if (!lua_isnil(L, -1))
    {
        int resource_id = lua_tointeger(L, -1);

        // blitter from image
#ifdef HAVE_OPENGL
        Image_Blitter *blitter = (get_screen_mode()->acceleration == _opengl_acceleration) ? new OGL_Blitter() : new Image_Blitter();
#else
        Image_Blitter *blitter = new Image_Blitter();
#endif
        if (!blitter->Load(resource_id))
        {
            lua_pushnil(L);
            delete blitter;
            return 1;
        }
        Lua_Image::Push(L, blitter);
        return 1;
    }
    
	// read path argument
	char path[256] = "";
	lua_pushstring(L, "path");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		strncpy(path, lua_tostring(L, -1), 256);
		path[255] = 0;
	}
	lua_pop(L, 1);
	// path is required
	if (!strlen(path))
	{
		lua_pushnil(L);
		return 1;
	}
	
	// read mask argument
	char mask[256] = "";
	lua_pushstring(L, "mask");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		strncpy(mask, lua_tostring(L, -1), 256);
		path[255] = 0;
	}
	lua_pop(L, 1);
	
	// path into file spec
	FileSpecifier File;
	if (!File.SetNameWithPath(path))
	{
		lua_pushnil(L);
		return 1;
	}
	
	// image with file spec
	ImageDescriptor image;
	if (!image.LoadFromFile(File, ImageLoader_Colors, 0))
	{
		lua_pushnil(L);
		return 1;
	}
	
	// mask (we don't care if it fails)
	if (strlen(mask) && File.SetNameWithPath(mask))
		image.LoadFromFile(File, ImageLoader_Opacity, 0);
	
	// blitter from image
#ifdef HAVE_OPENGL
    Image_Blitter *blitter = (get_screen_mode()->acceleration == _opengl_acceleration) ? new OGL_Blitter() : new Image_Blitter();
#else
    Image_Blitter *blitter = new Image_Blitter();
#endif

	if (!blitter->Load(image))
	{
		lua_pushnil(L);
		delete blitter;
		return 1;
	}
	Lua_Image::Push(L, blitter);
	return 1;
}

const luaL_reg Lua_Images_Get[] = {
{"new", L_TableFunction<Lua_Images_New>},
{0, 0}
};


char Lua_Shape_Name[] = "shape";
typedef L_ObjectClass<Lua_Shape_Name, Shape_Blitter *> Lua_Shape;

char Lua_Shape_Crop_Rect_Name[] = "shape_crop_rect";
class Lua_Shape_Crop_Rect : public L_Class<Lua_Shape_Crop_Rect_Name>
{
public:
	static Shape_Blitter *Object(lua_State *L, int index);
};

Shape_Blitter *Lua_Shape_Crop_Rect::Object(lua_State *L, int index)
{
	return Lua_Shape::ObjectAtIndex(L, Lua_Shape_Crop_Rect::Index(L, index));
}

static int Lua_Shape_Crop_Rect_Get_X(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.x);
	return 1;
}

static int Lua_Shape_Crop_Rect_Get_Y(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.y);
	return 1;
}

static int Lua_Shape_Crop_Rect_Get_Width(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.w);
	return 1;
}

static int Lua_Shape_Crop_Rect_Get_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.h);
	return 1;
}

static int Lua_Shape_Crop_Rect_Set_X(lua_State *L)
{
	Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.x = lua_tointeger(L, 2);
    return 0;
}

static int Lua_Shape_Crop_Rect_Set_Y(lua_State *L)
{
	Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.y = lua_tointeger(L, 2);
    return 0;
}

static int Lua_Shape_Crop_Rect_Set_Width(lua_State *L)
{
	Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.w = lua_tointeger(L, 2);
    return 0;
}

static int Lua_Shape_Crop_Rect_Set_Height(lua_State *L)
{
	Lua_Shape_Crop_Rect::Object(L, 1)->crop_rect.h = lua_tointeger(L, 2);
    return 0;
}

const luaL_reg Lua_Shape_Crop_Rect_Get[] = {
{"x", Lua_Shape_Crop_Rect_Get_X},
{"y", Lua_Shape_Crop_Rect_Get_Y},
{"width", Lua_Shape_Crop_Rect_Get_Width},
{"height", Lua_Shape_Crop_Rect_Get_Height},
{0, 0}
};

const luaL_reg Lua_Shape_Crop_Rect_Set[] = {
{"x", Lua_Shape_Crop_Rect_Set_X},
{"y", Lua_Shape_Crop_Rect_Set_Y},
{"width", Lua_Shape_Crop_Rect_Set_Width},
{"height", Lua_Shape_Crop_Rect_Set_Height},
{0, 0}
};


static int Lua_Shape_Get_Width(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->Width());
	return 1;
}

static int Lua_Shape_Get_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->Height());
	return 1;
}

static int Lua_Shape_Get_Unscaled_Width(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->UnscaledWidth());
	return 1;
}

static int Lua_Shape_Get_Unscaled_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->UnscaledHeight());
	return 1;
}

static int Lua_Shape_Get_Tint(lua_State *L)
{
	lua_newtable(L);
	lua_pushstring(L, "r");
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->tint_color_r);
	lua_settable(L, -3);
	lua_pushstring(L, "g");
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->tint_color_g);
	lua_settable(L, -3);
	lua_pushstring(L, "b");
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->tint_color_b);
	lua_settable(L, -3);
	lua_pushstring(L, "a");
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->tint_color_a);
	lua_settable(L, -3);
	return 1;
}


static int Lua_Shape_Get_Rotation(lua_State *L)
{
	lua_pushnumber(L, Lua_Shape::Object(L, 1)->rotation);
	return 1;
}

static int Lua_Shape_Get_Crop_Rect(lua_State *L)
{
	Lua_Shape_Crop_Rect::Push(L, Lua_Shape::Index(L, 1));
	return 1;
}

int Lua_Shape_Rescale(lua_State *L)
{
	Lua_Shape::Object(L, 1)->Rescale(lua_tointeger(L, 2), lua_tointeger(L, 3));
	return 0;
}

int Lua_Shape_Draw(lua_State *L)
{
	Lua_HUDInstance()->draw_shape(Lua_Shape::Object(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	return 0;
}

const luaL_reg Lua_Shape_Get[] = {
{"width", Lua_Shape_Get_Width},
{"height", Lua_Shape_Get_Height},
{"unscaled_width", Lua_Shape_Get_Unscaled_Width},
{"unscaled_height", Lua_Shape_Get_Unscaled_Height},
{"tint_color", Lua_Shape_Get_Tint},
{"rotation", Lua_Shape_Get_Rotation},
{"crop_rect", Lua_Shape_Get_Crop_Rect},
{"rescale", L_TableFunction<Lua_Shape_Rescale>},
{"draw", L_TableFunction<Lua_Shape_Draw>},
{0, 0}
};

static int Lua_Shape_Set_Tint(lua_State *L)
{
	Lua_Shape::Object(L, 1)->tint_color_r = Lua_HUDColor_Get_R(L, 2);
	Lua_Shape::Object(L, 1)->tint_color_g = Lua_HUDColor_Get_G(L, 2);
	Lua_Shape::Object(L, 1)->tint_color_b = Lua_HUDColor_Get_B(L, 2);
	Lua_Shape::Object(L, 1)->tint_color_a = Lua_HUDColor_Get_A(L, 2);
	return 0;
}

static int Lua_Shape_Set_Rotation(lua_State *L)
{
	Lua_Shape::Object(L, 1)->rotation = lua_tonumber(L, 2);
	return 0;
}

const luaL_reg Lua_Shape_Set[] = {
{"tint_color", Lua_Shape_Set_Tint},
{"rotation", Lua_Shape_Set_Rotation},
{0, 0}
};

static int Lua_Shape_GC(lua_State *L)
{
	delete Lua_Shape::Object(L, 1);
	Lua_Shape::Invalidate(L, Lua_Shape::Index(L, 1));
	return 0;
}

const luaL_reg Lua_Shape_Metatable[] = {
{"__gc", Lua_Shape_GC},
{0, 0}
};

char Lua_Shapes_Name[] = "Shapes";
typedef L_Class<Lua_Shapes_Name> Lua_Shapes;

int Lua_Shapes_New(lua_State *L)
{
	// read collection argument
	short collection_index = NONE;
	lua_pushstring(L, "collection");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
        collection_index = Lua_Collection::ToIndex(L, -1);
	lua_pop(L, 1);
	
	// read texture_index argument
	short texture_index = NONE;
	lua_pushstring(L, "texture_index");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
    {
        texture_index = static_cast<short>(lua_tonumber(L, 2));
        if (texture_index < 0 || texture_index >= MAXIMUM_SHAPES_PER_COLLECTION)
            return luaL_error(L, "texture_index: invalid texture index");
    }
	lua_pop(L, 1);
	
    // read type argument
	short texture_type = 0;
	lua_pushstring(L, "type");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
        texture_type = Lua_TextureType::ToIndex(L, -1);
	lua_pop(L, 1);

    // read texture_index argument
	short clut_index = 0;
	lua_pushstring(L, "color_table");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
    {
        clut_index = static_cast<short>(lua_tonumber(L, 2));
        if (clut_index < 0 || clut_index >= MAXIMUM_CLUTS_PER_COLLECTION)
            return luaL_error(L, "color_table: invalid clut index");
    }
	lua_pop(L, 1);
    
	// blitter from shape info
	Shape_Blitter *blitter = new Shape_Blitter(collection_index, texture_index, texture_type, clut_index);
	if (!blitter->Width())
	{
		lua_pushnil(L);
		delete blitter;
		return 1;
	}
	Lua_Shape::Push(L, blitter);
	return 1;
}

const luaL_reg Lua_Shapes_Get[] = {
{"new", L_TableFunction<Lua_Shapes_New>},
{0, 0}
};


char Lua_Font_Name[] = "font"; // "font"
typedef L_ObjectClass<Lua_Font_Name, FontSpecifier *> Lua_Font;

int Lua_Font_Measure_Text(lua_State *L)
{
	lua_pushnumber(L, Lua_Font::Object(L, 1)->TextWidth(lua_tostring(L, 2)));
	lua_pushnumber(L, Lua_Font::Object(L, 1)->LineSpacing);
	return 2;
}

int Lua_Font_Draw_Text(lua_State *L)
{
	const char *str = lua_tostring(L, 2);
	float x = static_cast<float>(lua_tonumber(L, 3));
	float y = static_cast<float>(lua_tonumber(L, 4));
	float r = Lua_HUDColor_Get_R(L, 5);
	float g = Lua_HUDColor_Get_G(L, 5);
	float b = Lua_HUDColor_Get_B(L, 5);
	float a = Lua_HUDColor_Get_A(L, 5);
	
	Lua_HUDInstance()->draw_text(Lua_Font::Object(L, 1),
															 str,
															 x, y, r, g, b, a);
	return 0;
}

static int Lua_Font_Get_Size(lua_State *L)
{
	lua_pushnumber(L, Lua_Font::Object(L, 1)->Size);
	return 1;
}

static int Lua_Font_Get_Style(lua_State *L)
{
	lua_pushnumber(L, Lua_Font::Object(L, 1)->Style);
	return 1;
}

static int Lua_Font_Get_File(lua_State *L)
{
	char *f = Lua_Font::Object(L, 1)->File;
	if (f[0] == '#')
		lua_pushnil(L);
	else
		lua_pushstring(L, f);
	return 1;
}

static int Lua_Font_Get_ID(lua_State *L)
{
	char *f = Lua_Font::Object(L, 1)->File;
	if (f[0] == '#')
	{
		short fontid = -1;
		sscanf(&f[1], "%hd", &fontid);
		if (fontid != -1)
			lua_pushnumber(L, fontid);
		else
			lua_pushnil(L);
	}
	else
		lua_pushnil(L);
	return 1;
}

static int Lua_Font_Get_Line_Height(lua_State *L)
{
	lua_pushnumber(L, Lua_Font::Object(L, 1)->LineSpacing);
	return 1;
}

const luaL_reg Lua_Font_Get[] = {
{"size", Lua_Font_Get_Size},
{"style", Lua_Font_Get_Style},
{"file", Lua_Font_Get_File},
{"id", Lua_Font_Get_ID},
{"line_height", Lua_Font_Get_Line_Height},
{"measure_text", L_TableFunction<Lua_Font_Measure_Text>},
{"draw_text", L_TableFunction<Lua_Font_Draw_Text>},
{0, 0}
};

static int Lua_Font_GC(lua_State *L)
{
	delete Lua_Font::Object(L, 1);
	Lua_Font::Invalidate(L, Lua_Font::Index(L, 1));
	return 0;
}

const luaL_reg Lua_Font_Metatable[] = {
{"__gc", Lua_Font_GC},
{0, 0}
};


char Lua_Fonts_Name[] = "Fonts"; // "Fonts"
typedef L_Class<Lua_Fonts_Name> Lua_Fonts;

int Lua_Fonts_New(lua_State *L)
{
	FontSpecifier f = {"Monaco", 12, styleNormal, 0, "mono"};
	
	lua_pushstring(L, "id");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
		snprintf(f.File, FontSpecifier::NameSetLen, "#%d", lua_tointeger(L, -1));
	lua_pop(L, 1);
	
	lua_pushstring(L, "file");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
	{
		strncpy(f.File, lua_tostring(L, -1), FontSpecifier::NameSetLen);
		f.File[FontSpecifier::NameSetLen-1] = 0;
	}
	lua_pop(L, 1);
	
	lua_pushstring(L, "size");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
		f.Size = lua_tointeger(L, -1);
	lua_pop(L, 1);
	
	lua_pushstring(L, "style");
	lua_gettable(L, 1);
	if (!lua_isnil(L, -1))
		f.Style = lua_tointeger(L, -1);
	lua_pop(L, 1);
	
	FontSpecifier *ff = new FontSpecifier(f);
	ff->Init();
#ifdef HAVE_OPENGL
	if (alephone::Screen::instance()->openGL())
		ff->OGL_Reset(true);
#endif
	if (ff->LineSpacing <= 0)
	{
		lua_pushnil(L);
		delete ff;
		return 1;
	}
	
	Lua_Font::Push(L, ff);
	return 1;
}

const luaL_reg Lua_Fonts_Get[] = {
{"new", L_TableFunction<Lua_Fonts_New>},
{0, 0}
};


char Lua_HUDPlayer_Item_Name[] = "item";
typedef L_Class<Lua_HUDPlayer_Item_Name> Lua_HUDPlayer_Item;

static int Lua_HUDPlayer_Item_Get_Count(lua_State *L)
{
	int item_type = Lua_HUDPlayer_Item::Index(L, 1);

	int item_count = current_player->items[item_type];
	if (item_count == NONE) item_count = 0;
	lua_pushnumber(L, item_count);
	return 1;
}

static int Lua_HUDPlayer_Item_Get_Section(lua_State *L)
{
	int item_type = Lua_HUDPlayer_Item::Index(L, 1);
	
	Lua_InventorySection::Push(L, get_item_kind(item_type));
	return 1;
}

static int Lua_HUDPlayer_Item_Get_Singular(lua_State *L)
{
	int item_type = Lua_HUDPlayer_Item::Index(L, 1);
	char tmp[256];
	tmp[0] = 0;
	get_item_name(tmp, item_type, false);
	lua_pushstring(L, tmp);
	return 1;
}

static int Lua_HUDPlayer_Item_Get_Plural(lua_State *L)
{
	int item_type = Lua_HUDPlayer_Item::Index(L, 1);
	char tmp[256];
	tmp[0] = 0;
	get_item_name(tmp, item_type, true);
	lua_pushstring(L, tmp);
	return 1;
}

static int Lua_HUDPlayer_Item_Get_Type(lua_State *L)
{
	Lua_ItemType::Push(L, Lua_HUDPlayer_Item::Index(L, 1));
	return 1;
}

const luaL_reg Lua_HUDPlayer_Item_Get[] = { 
{"count", Lua_HUDPlayer_Item_Get_Count},
{"inventory_section", Lua_HUDPlayer_Item_Get_Section},
{"singular", Lua_HUDPlayer_Item_Get_Singular},
{"plural", Lua_HUDPlayer_Item_Get_Plural},
{"type", Lua_HUDPlayer_Item_Get_Type},
{0, 0} 
};



char Lua_HUDPlayer_Items_Name[] = "player_items";
typedef L_Class<Lua_HUDPlayer_Items_Name> Lua_HUDPlayer_Items;

static int Lua_HUDPlayer_Items_Get(lua_State *L)
{
	Lua_HUDPlayer_Item::Push(L, Lua_ItemType::ToIndex(L, 2));
	return 1;
}

static int Lua_HUDPlayer_Items_Length(lua_State *L)
{
	lua_pushnumber(L, NUMBER_OF_DEFINED_ITEMS);
	return 1;
}

const luaL_reg Lua_HUDPlayer_Items_Metatable[] = {
{"__index", Lua_HUDPlayer_Items_Get},
{"__len", Lua_HUDPlayer_Items_Length},
{0, 0}
};

char Lua_HUDPlayer_Weapon_Trigger_Name[] = "player_weapon_trigger";


class Lua_HUDPlayer_Weapon_Trigger : public L_Class<Lua_HUDPlayer_Weapon_Trigger_Name>
{
public:
	int16 m_weapon_index;
	
	static Lua_HUDPlayer_Weapon_Trigger *Push(lua_State *L, int16 weapon_index, int16 index);
	static int16 WeaponIndex(lua_State *L, int index);
};

Lua_HUDPlayer_Weapon_Trigger *Lua_HUDPlayer_Weapon_Trigger::Push(lua_State *L, int16 weapon_index, int16 index)
{
	Lua_HUDPlayer_Weapon_Trigger *t = static_cast<Lua_HUDPlayer_Weapon_Trigger *>(L_Class<Lua_HUDPlayer_Weapon_Trigger_Name>::Push(L, index));
	if (t)
	{
		t->m_weapon_index = weapon_index;
	}
	
	return t;
}

int16 Lua_HUDPlayer_Weapon_Trigger::WeaponIndex(lua_State *L, int index)
{
	Lua_HUDPlayer_Weapon_Trigger *t = static_cast<Lua_HUDPlayer_Weapon_Trigger*>(lua_touserdata(L, index));
	if (!t) luaL_typerror(L, index, Lua_HUDPlayer_Weapon_Trigger_Name);
	return t->m_weapon_index;
}

static int Lua_HUDPlayer_Weapon_Trigger_Get_Rounds(lua_State *L)
{
	short rounds = get_player_weapon_ammo_count(
                                                current_player_index, 
                                                Lua_HUDPlayer_Weapon_Trigger::WeaponIndex(L, 1),
                                                Lua_HUDPlayer_Weapon_Trigger::Index(L, 1));
	lua_pushnumber(L, rounds);
	return 1;
}

static int Lua_HUDPlayer_Weapon_Trigger_Get_Total_Rounds(lua_State *L)
{
	short rounds = get_player_weapon_ammo_maximum(
                                                    current_player_index, 
                                                    Lua_HUDPlayer_Weapon_Trigger::WeaponIndex(L, 1),
                                                    Lua_HUDPlayer_Weapon_Trigger::Index(L, 1));
	lua_pushnumber(L, rounds);
	return 1;
}

static int Lua_HUDPlayer_Weapon_Trigger_Get_Ammo_Type(lua_State *L)
{
	int16 t = get_player_weapon_ammo_type(
                                            current_player_index, 
                                            Lua_HUDPlayer_Weapon_Trigger::WeaponIndex(L, 1),
                                            Lua_HUDPlayer_Weapon_Trigger::Index(L, 1));
	Lua_ItemType::Push(L, t);
	return 1;
}

static int Lua_HUDPlayer_Weapon_Trigger_Get_Weapon_Drawn(lua_State *L)
{
	bool t = get_player_weapon_drawn(
                                      current_player_index, 
                                      Lua_HUDPlayer_Weapon_Trigger::WeaponIndex(L, 1),
                                      Lua_HUDPlayer_Weapon_Trigger::Index(L, 1));
	lua_pushboolean(L, t);
	return 1;
}

const luaL_reg Lua_HUDPlayer_Weapon_Trigger_Get[] = {
{"rounds", Lua_HUDPlayer_Weapon_Trigger_Get_Rounds},
{"total_rounds", Lua_HUDPlayer_Weapon_Trigger_Get_Total_Rounds},
{"ammo_type", Lua_HUDPlayer_Weapon_Trigger_Get_Ammo_Type},
{"weapon_drawn", Lua_HUDPlayer_Weapon_Trigger_Get_Weapon_Drawn},
{0, 0}
};

char Lua_HUDPlayer_Weapon_Name[] = "player_weapon";
typedef L_Class<Lua_HUDPlayer_Weapon_Name> Lua_HUDPlayer_Weapon;

template<int trigger>
static int get_hudweapon_trigger(lua_State *L)
{
	Lua_HUDPlayer_Weapon_Trigger::Push(L, Lua_HUDPlayer_Weapon::Index(L, 1), trigger);
	return 1;
}

static int Lua_HUDPlayer_Weapon_Get_Type(lua_State *L)
{
	Lua_WeaponType::Push(L, Lua_HUDPlayer_Weapon::Index(L, 1));
	return 1;
}

const luaL_reg Lua_HUDPlayer_Weapon_Get[] = { 
{"primary", get_hudweapon_trigger<_primary_weapon>},
{"secondary", get_hudweapon_trigger<_secondary_weapon>},
{"type", Lua_HUDPlayer_Weapon_Get_Type},
{0, 0} 
};

extern player_weapon_data *get_player_weapon_data(const short player_index);
extern bool player_has_valid_weapon(short player_index);

char Lua_HUDPlayer_Weapons_Name[] = "player_weapons";
typedef L_Class<Lua_HUDPlayer_Weapons_Name> Lua_HUDPlayer_Weapons;

extern bool can_wield_weapons[MAXIMUM_NUMBER_OF_NETWORK_PLAYERS];

static int Lua_HUDPlayer_Weapons_Get(lua_State *L)
{
	bool string_arg = lua_isstring(L, 2) && !lua_isnumber(L, 2);
	if (string_arg && (strcmp(lua_tostring(L, 2), "current") == 0))
	{
	    if (player_has_valid_weapon(current_player_index))
	    {
	        player_weapon_data *weapon_data = get_player_weapon_data(current_player_index);
	        Lua_HUDPlayer_Weapon::Push(L, weapon_data->current_weapon);
	    }
	    else
	    {
	        lua_pushnil(L);
	    }
	}
	else if (string_arg && (strcmp(lua_tostring(L, 2), "desired") == 0))
	{
	    player_weapon_data *weapon_data = get_player_weapon_data(current_player_index);
	    if (weapon_data->desired_weapon != NONE)
	    {
	        Lua_HUDPlayer_Weapon::Push(L, weapon_data->desired_weapon);
	    }
	    else
	    {
	        lua_pushnil(L);
	    }
	}
	else
	{
		int index = Lua_WeaponType::ToIndex(L, 2);
		Lua_HUDPlayer_Weapon::Push(L, index);
	}
	
	return 1;
}

static int Lua_HUDPlayer_Weapons_Length(lua_State *L)
{
	lua_pushnumber(L, MAXIMUM_NUMBER_OF_WEAPONS);
	return 1;
}

const luaL_reg Lua_HUDPlayer_Weapons_Metatable[] = {
{"__index", Lua_HUDPlayer_Weapons_Get},
{"__len", Lua_HUDPlayer_Weapons_Length},
{0, 0}
};

char Lua_HUDPlayer_Section_Name[] = "player_section";
typedef L_Class<Lua_HUDPlayer_Section_Name> Lua_HUDPlayer_Section;

static int Lua_HUDPlayer_Section_Get_Name(lua_State *L)
{
	char tmp[256];
	tmp[0] = 0;
	get_header_name(tmp, Lua_HUDPlayer_Section::Index(L, 1));
	lua_pushstring(L, tmp);
	return 1;
}

static int Lua_HUDPlayer_Section_Get_Type(lua_State *L)
{
	Lua_InventorySection::Push(L, Lua_HUDPlayer_Section::Index(L, 1));
	return 1;
}

const luaL_reg Lua_HUDPlayer_Section_Get[] = { 
{"name", Lua_HUDPlayer_Section_Get_Name},
{"type", Lua_HUDPlayer_Section_Get_Type},
{0, 0} 
};

char Lua_HUDPlayer_Sections_Name[] = "player_sections";
typedef L_Class<Lua_HUDPlayer_Sections_Name> Lua_HUDPlayer_Sections;

static int Lua_HUDPlayer_Sections_Get(lua_State *L)
{
	if (lua_isstring(L, 2) && strcmp(lua_tostring(L, 2), "current") == 0)
	{
		Lua_HUDPlayer_Section::Push(L, GET_CURRENT_INVENTORY_SCREEN(current_player));
	}
	else
	{
		int index = Lua_InventorySection::ToIndex(L, 2);
		Lua_HUDPlayer_Section::Push(L, index);
	}
	
	return 1;
}

static int Lua_HUDPlayer_Sections_Length(lua_State *L)
{
	lua_pushnumber(L, NUMBER_OF_ITEM_TYPES + 1);
	return 1;
}

const luaL_reg Lua_HUDPlayer_Sections_Metatable[] = {
{"__index", Lua_HUDPlayer_Sections_Get},
{"__len", Lua_HUDPlayer_Sections_Length},
{0, 0}
};

char Lua_HUDCompass_Name[] = "compass";
typedef L_Class<Lua_HUDCompass_Name> Lua_HUDCompass;

static int Lua_HUDCompass_Get_NE(lua_State *L)
{
	lua_pushboolean(L, get_network_compass_state(current_player_index) & _network_compass_ne);
	return 1;
}

static int Lua_HUDCompass_Get_NW(lua_State *L)
{
	lua_pushboolean(L, get_network_compass_state(current_player_index) & _network_compass_nw);
	return 1;
}

static int Lua_HUDCompass_Get_SE(lua_State *L)
{
	lua_pushboolean(L, get_network_compass_state(current_player_index) & _network_compass_se);
	return 1;
}

static int Lua_HUDCompass_Get_SW(lua_State *L)
{
	lua_pushboolean(L, get_network_compass_state(current_player_index) & _network_compass_sw);
	return 1;
}

const luaL_reg Lua_HUDCompass_Get[] = {
{"ne", Lua_HUDCompass_Get_NE},
{"northeast", Lua_HUDCompass_Get_NE},
{"nw", Lua_HUDCompass_Get_NW},
{"northwest", Lua_HUDCompass_Get_NW},
{"se", Lua_HUDCompass_Get_SE},
{"southeast", Lua_HUDCompass_Get_SE},
{"sw", Lua_HUDCompass_Get_SW},
{"southwest", Lua_HUDCompass_Get_SW},
{0, 0}
};

char Lua_MotionSensor_Blip_Name[] = "sensor_blip";
typedef L_Class<Lua_MotionSensor_Blip_Name> Lua_MotionSensor_Blip;

static int Lua_MotionSensor_Blip_Get_Type(lua_State *L)
{
	blip_info info = Lua_HUDInstance()->entity_blip(Lua_MotionSensor_Blip::Index(L, 1));
	Lua_SensorBlipType::Push(L, info.mtype);
	return 1;
}

static int Lua_MotionSensor_Blip_Get_Intensity(lua_State *L)
{
	blip_info info = Lua_HUDInstance()->entity_blip(Lua_MotionSensor_Blip::Index(L, 1));
	lua_pushnumber(L, info.intensity);
	return 1;
}

static int Lua_MotionSensor_Blip_Get_Distance(lua_State *L)
{
	blip_info info = Lua_HUDInstance()->entity_blip(Lua_MotionSensor_Blip::Index(L, 1));
	lua_pushnumber(L, info.distance / 1024.0);
	return 1;
}

static int Lua_MotionSensor_Blip_Get_Direction(lua_State *L)
{
	blip_info info = Lua_HUDInstance()->entity_blip(Lua_MotionSensor_Blip::Index(L, 1));
	lua_pushnumber(L, info.direction * AngleConvert);
	return 1;
}

const luaL_reg Lua_MotionSensor_Blip_Get[] = { 
{"type", Lua_MotionSensor_Blip_Get_Type},
{"intensity", Lua_MotionSensor_Blip_Get_Intensity},
{"distance", Lua_MotionSensor_Blip_Get_Distance},
{"direction", Lua_MotionSensor_Blip_Get_Direction},
{"yaw", Lua_MotionSensor_Blip_Get_Direction},
{0, 0} 
};

static bool Lua_MotionSensor_Blip_Valid(int16 index)
{
	return index >= 0 && index < Lua_HUDInstance()->entity_blip_count();
}

char Lua_MotionSensor_Blips_Name[] = "sensor_blips";
typedef L_Class<Lua_MotionSensor_Blips_Name> Lua_MotionSensor_Blips;

static int Lua_MotionSensor_Blips_Get(lua_State *L)
{	
	Lua_MotionSensor_Blip::Push(L, lua_tointeger(L, 2));
	return 1;
}

static int Lua_MotionSensor_Blips_Length(lua_State *L)
{	
	lua_pushnumber(L, Lua_HUDInstance()->entity_blip_count());
	return 1;
}

const luaL_reg Lua_MotionSensor_Blips_Metatable[] = {
{"__index", Lua_MotionSensor_Blips_Get},
{"__len", Lua_MotionSensor_Blips_Length},
{0, 0}
};


char Lua_MotionSensor_Name[] = "motion_sensor";
typedef L_Class<Lua_MotionSensor_Name> Lua_MotionSensor;

extern bool MotionSensorActive;

static int Lua_MotionSensor_Get_Active(lua_State *L)
{
	lua_pushboolean(L, !(GET_GAME_OPTIONS() & _motion_sensor_does_not_work) && MotionSensorActive);
	return 1;
}

static int Lua_MotionSensor_Get_Blips(lua_State *L)
{
	Lua_MotionSensor_Blips::Push(L, Lua_MotionSensor::Index(L, 1));
	return 1;
}

const luaL_reg Lua_MotionSensor_Get[] = {
{"active", Lua_MotionSensor_Get_Active},
{"blips", Lua_MotionSensor_Get_Blips},
{0, 0}
};

char Lua_HUDTexturePalette_Slot_Name[] = "hud_texture_palette_slot";
typedef L_Class<Lua_HUDTexturePalette_Slot_Name> Lua_HUDTexturePalette_Slot;

static int Lua_HUDTexturePalette_Slot_Get_Collection(lua_State *L)
{
    int index = Lua_HUDTexturePalette_Slot::Index(L, 1);
    shape_descriptor shape = LuaTexturePaletteTexture(index);
    if (shape == UNONE)
        return 0;
    
    lua_pushnumber(L, GET_COLLECTION(GET_DESCRIPTOR_COLLECTION(shape)));
    return 1;
}

static int Lua_HUDTexturePalette_Slot_Get_Texture(lua_State *L)
{
    int index = Lua_HUDTexturePalette_Slot::Index(L, 1);
    shape_descriptor shape = LuaTexturePaletteTexture(index);
    if (shape == UNONE)
        return 0;
    
    lua_pushnumber(L, GET_DESCRIPTOR_SHAPE(shape));
    return 1;
}

static int Lua_HUDTexturePalette_Slot_Get_Type(lua_State *L)
{
    int index = Lua_HUDTexturePalette_Slot::Index(L, 1);
    shape_descriptor shape = LuaTexturePaletteTexture(index);
    if (shape == UNONE)
        return 0;
    
    Lua_TextureType::Push(L, LuaTexturePaletteTextureType(index));
    return 1;
}

const luaL_reg Lua_HUDTexturePalette_Slot_Get[] = {
{"collection", Lua_HUDTexturePalette_Slot_Get_Collection},
{"texture_index", Lua_HUDTexturePalette_Slot_Get_Texture},
{"type", Lua_HUDTexturePalette_Slot_Get_Type},
{0, 0}
};

static bool Lua_HUDTexturePalette_Slot_Valid(int16 index)
{
	return index >= 0 && index < LuaTexturePaletteSize();
}

char Lua_HUDTexturePalette_Slots_Name[] = "hud_texture_palette_slots";
typedef L_Class<Lua_HUDTexturePalette_Slots_Name> Lua_HUDTexturePalette_Slots;

static int Lua_HUDTexturePalette_Slots_Get(lua_State *L)
{
	if (lua_isnumber(L, 2))
	{
		int index = static_cast<int>(lua_tonumber(L, 2));
		if (index >= 0 && index < LuaTexturePaletteSize())
			Lua_HUDTexturePalette_Slot::Push(L, index);
		else
			lua_pushnil(L);
	}
	else
		lua_pushnil(L);
    
	return 1;
}

static int Lua_HUDTexturePalette_Slots_Length(lua_State *L)
{
	lua_pushnumber(L, LuaTexturePaletteSize());
	return 1;
}

const luaL_reg Lua_HUDTexturePalette_Slots_Metatable[] = {
{"__index", Lua_HUDTexturePalette_Slots_Get},
{"__len", Lua_HUDTexturePalette_Slots_Length},
{0, 0}
};

char Lua_HUDTexturePalette_Name[] = "hud_texture_palette";
typedef L_Class<Lua_HUDTexturePalette_Name> Lua_HUDTexturePalette;

static int Lua_HUDTexturePalette_Get_Size(lua_State *L)
{
    lua_pushnumber(L, LuaTexturePaletteSize());
    return 1;
}

static int Lua_HUDTexturePalette_Get_Selected(lua_State *L)
{
    if (LuaTexturePaletteSize() < 1)
        return 0;
    
    int selected = LuaTexturePaletteSelected();
    if (selected < 0)
        return 0;
    
    lua_pushnumber(L, selected);
    return 1;
}

static int Lua_HUDTexturePalette_Get_Slots(lua_State *L)
{
    Lua_HUDTexturePalette_Slots::Push(L, Lua_HUDTexturePalette::Index(L, 1));
    return 1;
}

const luaL_reg Lua_HUDTexturePalette_Get[] = {
{"size", Lua_HUDTexturePalette_Get_Size},
{"highlight", Lua_HUDTexturePalette_Get_Selected},
{"slots", Lua_HUDTexturePalette_Get_Slots},
{0, 0}
};


char Lua_HUDPlayer_Name[] = "Player";
typedef L_Class<Lua_HUDPlayer_Name> Lua_HUDPlayer;

static int Lua_HUDPlayer_Get_Color(lua_State *L)
{
	Lua_PlayerColor::Push(L, current_player->color);
	return 1;
}

static int Lua_HUDPlayer_Get_Dead(lua_State *L)
{
	lua_pushboolean(L, (PLAYER_IS_DEAD(current_player) || PLAYER_IS_TOTALLY_DEAD(current_player)));
	return 1;
}

static int Lua_HUDPlayer_Get_Energy(lua_State *L)
{
	lua_pushnumber(L, current_player->suit_energy);
	return 1;
}

static int Lua_HUDPlayer_Get_Direction(lua_State *L)
{
	double angle = FIXED_INTEGERAL_PART(current_player->variables.direction) * AngleConvert;
	lua_pushnumber(L, angle);
	return 1;
}

static int Lua_HUDPlayer_Get_Elevation(lua_State *L)
{
	double angle = FIXED_INTEGERAL_PART(current_player->variables.elevation) * AngleConvert;
	lua_pushnumber(L, angle);
	return 1;
}

static int Lua_HUDPlayer_Get_Microphone(lua_State *L)
{    
    lua_pushboolean(L, current_netgame_allows_microphone() &&
                    (dynamic_world->speaking_player_index == local_player_index));
	return 1;
}

static int Lua_HUDPlayer_Get_Items(lua_State *L)
{
	Lua_HUDPlayer_Items::Push(L, Lua_HUDPlayer::Index(L, 1));
	return 1;
}

static int Lua_HUDPlayer_Get_Name(lua_State *L)
{
	lua_pushstring(L, current_player->name);
	return 1;
}

static int Lua_HUDPlayer_Get_Oxygen(lua_State *L)
{
	lua_pushnumber(L, current_player->suit_oxygen);
	return 1;
}

static int Lua_HUDPlayer_Get_Team(lua_State *L)
{
	Lua_PlayerColor::Push(L, current_player->team);
	return 1;
}

static int Lua_HUDPlayer_Get_Sections(lua_State *L)
{
	Lua_HUDPlayer_Sections::Push(L, Lua_HUDPlayer::Index(L, 1));
	return 1;
}

static int Lua_HUDPlayer_Get_Weapons(lua_State *L)
{
	Lua_HUDPlayer_Weapons::Push(L, Lua_HUDPlayer::Index(L, 1));
	return 1;
}

static int Lua_HUDPlayer_Get_Motion(lua_State *L)
{
	Lua_MotionSensor::Push(L, 1);
	return 1;
}

static int Lua_HUDPlayer_Get_Compass(lua_State *L)
{
	Lua_HUDCompass::Push(L, 1);
	return 1;
}

static int Lua_HUDPlayer_Get_Zoom(lua_State *L)
{
	lua_pushboolean(L, GetTunnelVision());
    return 1;
}

static int Lua_HUDPlayer_Get_Texture_Palette(lua_State *L)
{
    Lua_HUDTexturePalette::Push(L, 1);
    return 1;
}

const luaL_reg Lua_HUDPlayer_Get[] = {
{"color", Lua_HUDPlayer_Get_Color},
{"dead", Lua_HUDPlayer_Get_Dead},
{"direction", Lua_HUDPlayer_Get_Direction},
{"yaw", Lua_HUDPlayer_Get_Direction},
{"elevation", Lua_HUDPlayer_Get_Elevation},
{"pitch", Lua_HUDPlayer_Get_Elevation},
{"energy", Lua_HUDPlayer_Get_Energy},
{"life", Lua_HUDPlayer_Get_Energy},
{"microphone_active", Lua_HUDPlayer_Get_Microphone},
{"items", Lua_HUDPlayer_Get_Items},
{"name", Lua_HUDPlayer_Get_Name},
{"oxygen", Lua_HUDPlayer_Get_Oxygen},
{"team", Lua_HUDPlayer_Get_Team},
{"inventory_sections", Lua_HUDPlayer_Get_Sections},
{"weapons", Lua_HUDPlayer_Get_Weapons},
{"motion_sensor", Lua_HUDPlayer_Get_Motion},
{"compass", Lua_HUDPlayer_Get_Compass},
{"zoom_active", Lua_HUDPlayer_Get_Zoom},
{"texture_palette", Lua_HUDPlayer_Get_Texture_Palette},
{0, 0}
};


char Lua_Screen_Clip_Rect_Name[] = "clip_rect";
typedef L_Class<Lua_Screen_Clip_Rect_Name> Lua_Screen_Clip_Rect;

static int Lua_Screen_Clip_Rect_Get_X(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_clip_rect.x);
	return 1;
}

static int Lua_Screen_Clip_Rect_Get_Y(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_clip_rect.y);
	return 1;
}

static int Lua_Screen_Clip_Rect_Get_Width(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_clip_rect.w);
	return 1;
}

static int Lua_Screen_Clip_Rect_Get_Height(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_clip_rect.h);
	return 1;
}

static int Lua_Screen_Clip_Rect_Set_X(lua_State *L)
{
	alephone::Screen::instance()->lua_clip_rect.x = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Clip_Rect_Set_Y(lua_State *L)
{
	alephone::Screen::instance()->lua_clip_rect.y = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Clip_Rect_Set_Width(lua_State *L)
{
	alephone::Screen::instance()->lua_clip_rect.w = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Clip_Rect_Set_Height(lua_State *L)
{
	alephone::Screen::instance()->lua_clip_rect.h = lua_tointeger(L, 2);
  return 0;
}

const luaL_reg Lua_Screen_Clip_Rect_Get[] = {
{"x", Lua_Screen_Clip_Rect_Get_X},
{"y", Lua_Screen_Clip_Rect_Get_Y},
{"width", Lua_Screen_Clip_Rect_Get_Width},
{"height", Lua_Screen_Clip_Rect_Get_Height},
{0, 0}
};

const luaL_reg Lua_Screen_Clip_Rect_Set[] = {
{"x", Lua_Screen_Clip_Rect_Set_X},
{"y", Lua_Screen_Clip_Rect_Set_Y},
{"width", Lua_Screen_Clip_Rect_Set_Width},
{"height", Lua_Screen_Clip_Rect_Set_Height},
{0, 0}
};

char Lua_Screen_World_Rect_Name[] = "world_rect";
typedef L_Class<Lua_Screen_World_Rect_Name> Lua_Screen_World_Rect;

static int Lua_Screen_World_Rect_Get_X(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_view_rect.x);
	return 1;
}

static int Lua_Screen_World_Rect_Get_Y(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_view_rect.y);
	return 1;
}

static int Lua_Screen_World_Rect_Get_Width(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_view_rect.w);
	return 1;
}

static int Lua_Screen_World_Rect_Get_Height(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_view_rect.h);
	return 1;
}

static int Lua_Screen_World_Rect_Set_X(lua_State *L)
{
	alephone::Screen::instance()->lua_view_rect.x = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_World_Rect_Set_Y(lua_State *L)
{
	alephone::Screen::instance()->lua_view_rect.y = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_World_Rect_Set_Width(lua_State *L)
{
	alephone::Screen::instance()->lua_view_rect.w = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_World_Rect_Set_Height(lua_State *L)
{
	alephone::Screen::instance()->lua_view_rect.h = lua_tointeger(L, 2);
  return 0;
}

const luaL_reg Lua_Screen_World_Rect_Get[] = {
{"x", Lua_Screen_World_Rect_Get_X},
{"y", Lua_Screen_World_Rect_Get_Y},
{"width", Lua_Screen_World_Rect_Get_Width},
{"height", Lua_Screen_World_Rect_Get_Height},
{0, 0}
};

const luaL_reg Lua_Screen_World_Rect_Set[] = {
{"x", Lua_Screen_World_Rect_Set_X},
{"y", Lua_Screen_World_Rect_Set_Y},
{"width", Lua_Screen_World_Rect_Set_Width},
{"height", Lua_Screen_World_Rect_Set_Height},
{0, 0}
};

char Lua_Screen_Map_Rect_Name[] = "map_rect";
typedef L_Class<Lua_Screen_Map_Rect_Name> Lua_Screen_Map_Rect;

static int Lua_Screen_Map_Rect_Get_X(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_map_rect.x);
	return 1;
}

static int Lua_Screen_Map_Rect_Get_Y(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_map_rect.y);
	return 1;
}

static int Lua_Screen_Map_Rect_Get_Width(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_map_rect.w);
	return 1;
}

static int Lua_Screen_Map_Rect_Get_Height(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_map_rect.h);
	return 1;
}

static int Lua_Screen_Map_Rect_Set_X(lua_State *L)
{
	alephone::Screen::instance()->lua_map_rect.x = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Map_Rect_Set_Y(lua_State *L)
{
	alephone::Screen::instance()->lua_map_rect.y = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Map_Rect_Set_Width(lua_State *L)
{
	alephone::Screen::instance()->lua_map_rect.w = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Map_Rect_Set_Height(lua_State *L)
{
	alephone::Screen::instance()->lua_map_rect.h = lua_tointeger(L, 2);
  return 0;
}

const luaL_reg Lua_Screen_Map_Rect_Get[] = {
{"x", Lua_Screen_Map_Rect_Get_X},
{"y", Lua_Screen_Map_Rect_Get_Y},
{"width", Lua_Screen_Map_Rect_Get_Width},
{"height", Lua_Screen_Map_Rect_Get_Height},
{0, 0}
};

const luaL_reg Lua_Screen_Map_Rect_Set[] = {
{"x", Lua_Screen_Map_Rect_Set_X},
{"y", Lua_Screen_Map_Rect_Set_Y},
{"width", Lua_Screen_Map_Rect_Set_Width},
{"height", Lua_Screen_Map_Rect_Set_Height},
{0, 0}
};

char Lua_Screen_Term_Rect_Name[] = "term_rect";
typedef L_Class<Lua_Screen_Term_Rect_Name> Lua_Screen_Term_Rect;

static int Lua_Screen_Term_Rect_Get_X(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_term_rect.x);
	return 1;
}

static int Lua_Screen_Term_Rect_Get_Y(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_term_rect.y);
	return 1;
}

static int Lua_Screen_Term_Rect_Get_Width(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_term_rect.w);
	return 1;
}

static int Lua_Screen_Term_Rect_Get_Height(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->lua_term_rect.h);
	return 1;
}

static int Lua_Screen_Term_Rect_Set_X(lua_State *L)
{
	alephone::Screen::instance()->lua_term_rect.x = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Term_Rect_Set_Y(lua_State *L)
{
	alephone::Screen::instance()->lua_term_rect.y = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Term_Rect_Set_Width(lua_State *L)
{
	alephone::Screen::instance()->lua_term_rect.w = lua_tointeger(L, 2);
  return 0;
}

static int Lua_Screen_Term_Rect_Set_Height(lua_State *L)
{
	alephone::Screen::instance()->lua_term_rect.h = lua_tointeger(L, 2);
  return 0;
}

const luaL_reg Lua_Screen_Term_Rect_Get[] = {
{"x", Lua_Screen_Term_Rect_Get_X},
{"y", Lua_Screen_Term_Rect_Get_Y},
{"width", Lua_Screen_Term_Rect_Get_Width},
{"height", Lua_Screen_Term_Rect_Get_Height},
{0, 0}
};

const luaL_reg Lua_Screen_Term_Rect_Set[] = {
{"x", Lua_Screen_Term_Rect_Set_X},
{"y", Lua_Screen_Term_Rect_Set_Y},
{"width", Lua_Screen_Term_Rect_Set_Width},
{"height", Lua_Screen_Term_Rect_Set_Height},
{0, 0}
};

char Lua_Screen_FOV_Name[] = "field_of_view";
typedef L_Class<Lua_Screen_FOV_Name> Lua_Screen_FOV;

static int Lua_Screen_FOV_Get_Horizontal(lua_State *L)
{
    lua_pushnumber(L, world_view->half_cone * 2.0f);
    return 1;
}

static int Lua_Screen_FOV_Get_Vertical(lua_State *L)
{
    lua_pushnumber(L, world_view->half_vertical_cone * 2.0f);
    return 1;
}

static int Lua_Screen_FOV_Get_Fix(lua_State *L)
{
    lua_pushboolean(L, View_FOV_FixHorizontalNotVertical());
    return 1;
}

const luaL_reg Lua_Screen_FOV_Get[] = {
{"horizontal", Lua_Screen_FOV_Get_Horizontal},
{"vertical", Lua_Screen_FOV_Get_Vertical},
{"fix_h_not_v", Lua_Screen_FOV_Get_Fix},
{0, 0}
};

const luaL_reg Lua_Screen_FOV_Set[] = {
{0, 0}
};

char Lua_Screen_Name[] = "Screen";
typedef L_Class<Lua_Screen_Name> Lua_Screen;

static int Lua_Screen_Get_Width(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->window_width());
	return 1;
}

static int Lua_Screen_Get_Height(lua_State *L)
{
	lua_pushnumber(L, alephone::Screen::instance()->window_height());
	return 1;
}

static int Lua_Screen_Get_Renderer(lua_State *L)
{
	Lua_RendererType::Push(L, get_screen_mode()->acceleration);
	return 1;
}

static int Lua_Screen_Get_Term_Size(lua_State *L)
{
	Lua_SizePreference::Push(L, get_screen_mode()->term_scale_level);
	return 1;
}

static int Lua_Screen_Get_HUD_Size(lua_State *L)
{
	Lua_SizePreference::Push(L, get_screen_mode()->hud_scale_level);
	return 1;
}

static int Lua_Screen_Get_Clip_Rect(lua_State *L)
{
	Lua_Screen_Clip_Rect::Push(L, Lua_Screen::Index(L, 1));
	return 1;
}

static int Lua_Screen_Get_World_Rect(lua_State *L)
{
	Lua_Screen_World_Rect::Push(L, Lua_Screen::Index(L, 1));
	return 1;
}

static int Lua_Screen_Get_Map_Rect(lua_State *L)
{
	Lua_Screen_Map_Rect::Push(L, Lua_Screen::Index(L, 1));
	return 1;
}

static int Lua_Screen_Get_Term_Rect(lua_State *L)
{
	Lua_Screen_Term_Rect::Push(L, Lua_Screen::Index(L, 1));
	return 1;
}

static int Lua_Screen_Get_Map_Active(lua_State *L)
{
	lua_pushboolean(L, world_view->overhead_map_active);
	return 1;
}

static int Lua_Screen_Get_Term_Active(lua_State *L)
{
	lua_pushboolean(L, world_view->terminal_mode_active);
	return 1;
}

static int Lua_Screen_Get_FOV(lua_State *L)
{
    Lua_Screen_FOV::Push(L, Lua_Screen::Index(L, 1));
    return 1;
}

int Lua_Screen_Fill_Rect(lua_State *L)
{
	float x = static_cast<float>(lua_tonumber(L, 1));
	float y = static_cast<float>(lua_tonumber(L, 2));
	float w = static_cast<float>(lua_tonumber(L, 3));
	float h = static_cast<float>(lua_tonumber(L, 4));
	float r = Lua_HUDColor_Get_R(L, 5);
	float g = Lua_HUDColor_Get_G(L, 5);
	float b = Lua_HUDColor_Get_B(L, 5);
	float a = Lua_HUDColor_Get_A(L, 5);
	
	Lua_HUDInstance()->fill_rect(x, y, w, h, r, g, b, a);
	return 0;
}

int Lua_Screen_Frame_Rect(lua_State *L)
{
	float x = static_cast<float>(lua_tonumber(L, 1));
	float y = static_cast<float>(lua_tonumber(L, 2));
	float w = static_cast<float>(lua_tonumber(L, 3));
	float h = static_cast<float>(lua_tonumber(L, 4));
	float r = Lua_HUDColor_Get_R(L, 5);
	float g = Lua_HUDColor_Get_G(L, 5);
	float b = Lua_HUDColor_Get_B(L, 5);
	float a = Lua_HUDColor_Get_A(L, 5);
	float thickness = static_cast<float>(lua_tonumber(L, 6));
	
	Lua_HUDInstance()->frame_rect(x, y, w, h, r, g, b, a, thickness);
	return 0;
}

const luaL_reg Lua_Screen_Get[] = {
{"width", Lua_Screen_Get_Width},
{"height", Lua_Screen_Get_Height},
{"renderer", Lua_Screen_Get_Renderer},
{"clip_rect", Lua_Screen_Get_Clip_Rect},
{"world_rect", Lua_Screen_Get_World_Rect},
{"map_rect", Lua_Screen_Get_Map_Rect},
{"term_rect", Lua_Screen_Get_Term_Rect},
{"map_active", Lua_Screen_Get_Map_Active},
{"term_active", Lua_Screen_Get_Term_Active},
{"hud_size_preference", Lua_Screen_Get_HUD_Size},
{"term_size_preference", Lua_Screen_Get_Term_Size},
{"field_of_view", Lua_Screen_Get_FOV},
{"fill_rect", L_TableFunction<Lua_Screen_Fill_Rect>},
{"frame_rect", L_TableFunction<Lua_Screen_Frame_Rect>},
{0, 0}
};


char Lua_HUDGame_Player_Name[] = "game_player";
typedef L_Class<Lua_HUDGame_Player_Name> Lua_HUDGame_Player;

static int Lua_HUDGame_Player_Get_Color(lua_State *L)
{
	Lua_PlayerColor::Push(L, get_player_data(Lua_HUDGame_Player::Index(L, 1))->color);
	return 1;
}

static int Lua_HUDGame_Player_Get_Team(lua_State *L)
{
	Lua_PlayerColor::Push(L, get_player_data(Lua_HUDGame_Player::Index(L, 1))->team);
	return 1;
}

static int Lua_HUDGame_Player_Get_Name(lua_State *L)
{
	lua_pushstring(L, get_player_data(Lua_HUDGame_Player::Index(L, 1))->name);
	return 1;
}

static int Lua_HUDGame_Player_Get_Local(lua_State *L)
{
	lua_pushboolean(L, Lua_HUDGame_Player::Index(L, 1) == local_player_index);
	return 1;
}

static int Lua_HUDGame_Player_Get_Kills(lua_State *L)
{
	short player_index = Lua_HUDGame_Player::Index(L, 1);
	struct player_data* player = get_player_data(player_index);
	lua_pushnumber(L, player->total_damage_given.kills - player->damage_taken[player_index].kills);
	return 1;
}

static int Lua_HUDGame_Player_Get_Ranking(lua_State *L)
{
	short kills, deaths;
	lua_pushnumber(L, get_player_net_ranking(Lua_HUDGame_Player::Index(L, 1), &kills, &deaths, false));
	return 1;
}

const luaL_reg Lua_HUDGame_Player_Get[] = { 
{"color", Lua_HUDGame_Player_Get_Color},
{"team", Lua_HUDGame_Player_Get_Team},
{"name", Lua_HUDGame_Player_Get_Name},
{"local_", Lua_HUDGame_Player_Get_Local},
{"kills", Lua_HUDGame_Player_Get_Kills},
{"ranking", Lua_HUDGame_Player_Get_Ranking},
{0, 0} 
};

static bool Lua_HUDGame_Player_Valid(int16 index)
{
	return index >= 0 && index < dynamic_world->player_count;
}

char Lua_HUDGame_Players_Name[] = "game_players";
typedef L_Class<Lua_HUDGame_Players_Name> Lua_HUDGame_Players;

static int Lua_HUDGame_Players_Get(lua_State *L)
{	
	Lua_HUDGame_Player::Push(L, lua_tointeger(L, 2));
	return 1;
}

static int Lua_HUDGame_Players_Length(lua_State *L)
{	
	lua_pushnumber(L, dynamic_world->player_count);
	return 1;
}

const luaL_reg Lua_HUDGame_Players_Metatable[] = {
{"__index", Lua_HUDGame_Players_Get},
{"__len", Lua_HUDGame_Players_Length},
{0, 0}
};


char Lua_HUDGame_Name[] = "Game";
typedef L_Class<Lua_HUDGame_Name> Lua_HUDGame;

static int Lua_HUDGame_Get_Players(lua_State *L)
{
	Lua_HUDGame_Players::Push(L, Lua_HUDGame::Index(L, 1));
	return 1;
}

static int Lua_HUDGame_Get_Difficulty(lua_State *L)
{
	Lua_DifficultyType::Push(L, dynamic_world->game_information.difficulty_level);
	return 1;
}

static int Lua_HUDGame_Get_Kill_Limit(lua_State *L)
{
	if (GET_GAME_OPTIONS() & _game_has_kill_limit) 
	{
		switch (GET_GAME_TYPE())
		{
			case _game_of_kill_monsters:
			case _game_of_cooperative_play:
			case _game_of_king_of_the_hill:
			case _game_of_kill_man_with_ball:
			case _game_of_tag:
				lua_pushnumber(L, dynamic_world->game_information.kill_limit);
				return 1;
		}
	}
	lua_pushnil(L);
	return 1;
}

static int Lua_HUDGame_Get_Time_Remaining(lua_State* L)
{
  if(dynamic_world->game_information.game_time_remaining > 999 * 30)
    lua_pushnil(L);
  else
    lua_pushnumber(L, dynamic_world->game_information.game_time_remaining);
  return 1;
}

static int Lua_HUDGame_Get_Ticks(lua_State *L)
{
	lua_pushnumber(L, dynamic_world->tick_count);
	return 1;
}

static int Lua_HUDGame_Get_Type(lua_State *L)
{
	Lua_GameType::Push(L, GET_GAME_TYPE());
	return 1;
}

extern int game_scoring_mode;
static int Lua_HUDGame_Get_Scoring_Mode(lua_State *L)
{
	Lua_ScoringMode::Push(L, game_scoring_mode);
	return 1;
}

static int Lua_HUDGame_Get_Version(lua_State *L)
{
	lua_pushstring(L, A1_DATE_VERSION);
	return 1;
}

const luaL_reg Lua_HUDGame_Get[] = {
{"difficulty", Lua_HUDGame_Get_Difficulty},
{"kill_limit", Lua_HUDGame_Get_Kill_Limit},
{"time_remaining", Lua_HUDGame_Get_Time_Remaining},
{"scoring_mode", Lua_HUDGame_Get_Scoring_Mode},
{"ticks", Lua_HUDGame_Get_Ticks},
{"type", Lua_HUDGame_Get_Type},
{"version", Lua_HUDGame_Get_Version},
{"players", Lua_HUDGame_Get_Players},
{0, 0}
};

char Lua_HUDLighting_Fader_Name[] = "lighting_fader";
typedef L_Class<Lua_HUDLighting_Fader_Name> Lua_HUDLighting_Fader;

static int Lua_HUDLighting_Fader_Get_Active(lua_State *L)
{
    bool active = false;
#ifdef HAVE_OPENGL
    if (OGL_FaderActive())
    {
        OGL_Fader *fader = GetOGL_FaderQueueEntry(Lua_HUDLighting_Fader::Index(L, 1));
        if (fader && fader->Type != NONE && fader->Color[3] > 0.01)
            active = true;
    }
#endif
    lua_pushboolean(L, active);
    return 1;
}


static int Lua_HUDLighting_Fader_Get_Type(lua_State *L)
{
#ifdef HAVE_OPENGL
    if (OGL_FaderActive())
    {
        OGL_Fader *fader = GetOGL_FaderQueueEntry(Lua_HUDLighting_Fader::Index(L, 1));
        if (fader && fader->Type != NONE && fader->Color[3] > 0.01)
        {
            Lua_FadeEffectType::Push(L, fader->Type);
            return 1;
        }
    }
#endif
    lua_pushnil(L);
    return 1;
}

static int Lua_HUDLighting_Fader_Get_Color(lua_State *L)
{
#ifdef HAVE_OPENGL
    if (OGL_FaderActive())
    {
        OGL_Fader *fader = GetOGL_FaderQueueEntry(Lua_HUDLighting_Fader::Index(L, 1));
        if (fader && fader->Type != NONE && fader->Color[3] > 0.01)
        {
            lua_newtable(L);
            lua_pushstring(L, "r");
            lua_pushnumber(L, std::min(1.f, std::max(0.f, fader->Color[0])));
            lua_settable(L, -3);
            lua_pushstring(L, "g");
            lua_pushnumber(L, std::min(1.f, std::max(0.f, fader->Color[1])));
            lua_settable(L, -3);
            lua_pushstring(L, "b");
            lua_pushnumber(L, std::min(1.f, std::max(0.f, fader->Color[2])));
            lua_settable(L, -3);
            lua_pushstring(L, "a");
            lua_pushnumber(L, std::min(1.f, std::max(0.f, fader->Color[3])));
            lua_settable(L, -3);
            return 1;
        }
    }
#endif
    lua_pushnil(L);
    return 1;
}

const luaL_reg Lua_HUDLighting_Fader_Get[] = {
{"active", Lua_HUDLighting_Fader_Get_Active},
{"type", Lua_HUDLighting_Fader_Get_Type},
{"color", Lua_HUDLighting_Fader_Get_Color},
{0, 0}
};

char Lua_HUDLighting_Name[] = "Lighting";
typedef L_Class<Lua_HUDLighting_Name> Lua_HUDLighting;

static int Lua_HUDLighting_Get_Ambient(lua_State *L)
{
    lua_pushnumber(L, get_light_intensity(get_polygon_data(world_view->origin_polygon_index)->floor_lightsource_index)/65535.f);
	return 1;
}

static int Lua_HUDLighting_Get_Weapon(lua_State *L)
{
    lua_pushnumber(L, PIN(current_player->weapon_intensity - 32768, 0, 32767)/32767.f);
    return 1;
}

static int Lua_HUDLighting_Get_Liquid_Fader(lua_State *L)
{
    Lua_HUDLighting_Fader::Push(L, FaderQueue_Liquid);
    return 1;
}

static int Lua_HUDLighting_Get_Damage_Fader(lua_State *L)
{
    Lua_HUDLighting_Fader::Push(L, FaderQueue_Other);
    return 1;
}

const luaL_reg Lua_HUDLighting_Get[] = {
{"ambient_light", Lua_HUDLighting_Get_Ambient},
{"weapon_flash", Lua_HUDLighting_Get_Weapon},
{"liquid_fader", Lua_HUDLighting_Get_Liquid_Fader},
{"damage_fader", Lua_HUDLighting_Get_Damage_Fader},
{0, 0}
};

extern bool collection_loaded(short);

int Lua_HUDObjects_register(lua_State *L)
{
	Lua_Collection::Register(L, Lua_Collection_Get, 0, 0, Lua_Collection_Mnemonics);
	Lua_Collection::Valid = collection_loaded;

	Lua_Collections::Register(L);
	Lua_Collections::Length = Lua_Collections::ConstantLength(MAXIMUM_COLLECTIONS);

	Lua_DifficultyType::Register(L, 0, 0, 0, Lua_DifficultyType_Mnemonics);
	Lua_DifficultyType::Valid = Lua_DifficultyType::ValidRange(NUMBER_OF_GAME_DIFFICULTY_LEVELS);

	Lua_DifficultyTypes::Register(L);
	Lua_DifficultyTypes::Length = Lua_DifficultyTypes::ConstantLength(NUMBER_OF_GAME_DIFFICULTY_LEVELS);
    
    Lua_FadeEffectType::Register(L, 0, 0, 0, Lua_FadeEffectType_Mnemonics);
    Lua_FadeEffectType::Valid = Lua_FadeEffectType::ValidRange(NUMBER_OF_FADER_FUNCTIONS);
    
    Lua_FadeEffectTypes::Register(L);
    Lua_FadeEffectTypes::Length = Lua_FadeEffectTypes::ConstantLength(NUMBER_OF_FADER_FUNCTIONS);
	
	Lua_GameType::Register(L, 0, 0, 0, Lua_GameType_Mnemonics);
	Lua_GameType::Valid = Lua_GameType::ValidRange(NUMBER_OF_GAME_TYPES);
	
	Lua_GameTypes::Register(L);
	Lua_GameTypes::Length = Lua_GameTypes::ConstantLength(NUMBER_OF_GAME_TYPES);
	
	Lua_InventorySection::Register(L, 0, 0, 0, Lua_InventorySection_Mnemonics);
	Lua_InventorySection::Valid = Lua_InventorySection::ValidRange(NUMBER_OF_ITEM_TYPES + 1);
	
	Lua_InventorySections::Register(L);
	Lua_InventorySections::Length = Lua_InventorySections::ConstantLength(NUMBER_OF_ITEM_TYPES + 1);
	
	Lua_ItemType::Register(L, Lua_ItemType_Get, 0, 0, Lua_ItemType_Mnemonics);
	Lua_ItemType::Valid = Lua_ItemType::ValidRange(NUMBER_OF_DEFINED_ITEMS);
	
	Lua_ItemTypes::Register(L);
	Lua_ItemTypes::Length = Lua_ItemTypes::ConstantLength(NUMBER_OF_DEFINED_ITEMS);
	
	Lua_PlayerColor::Register(L, 0, 0, 0, Lua_PlayerColor_Mnemonics);
	Lua_PlayerColor::Valid = Lua_PlayerColor::ValidRange(NUMBER_OF_TEAM_COLORS);
	
	Lua_PlayerColors::Register(L);
	Lua_PlayerColors::Length = Lua_PlayerColors::ConstantLength((int16) NUMBER_OF_TEAM_COLORS);

	Lua_RendererType::Register(L, 0, 0, 0, Lua_RendererType_Mnemonics);
	Lua_RendererType::Valid = Lua_RendererType::ValidRange(NUMBER_OF_RENDERER_TYPES);
	
	Lua_RendererTypes::Register(L);
	Lua_RendererTypes::Length = Lua_RendererTypes::ConstantLength(NUMBER_OF_RENDERER_TYPES);

	Lua_ScoringMode::Register(L, 0, 0, 0, Lua_ScoringMode_Mnemonics);
	Lua_ScoringMode::Valid = Lua_ScoringMode::ValidRange(NUMBER_OF_GAME_SCORING_MODES);
	
	Lua_ScoringModes::Register(L);
	Lua_ScoringModes::Length = Lua_ScoringModes::ConstantLength(NUMBER_OF_GAME_SCORING_MODES);
	
	Lua_SizePreference::Register(L, 0, 0, 0, Lua_SizePref_Mnemonics);
	Lua_SizePreference::Valid = Lua_SizePreference::ValidRange(NUMBER_OF_SIZE_PREFERENCES);
	
	Lua_SizePreferences::Register(L);
	Lua_SizePreferences::Length = Lua_SizePreferences::ConstantLength(NUMBER_OF_SIZE_PREFERENCES);
    
	Lua_SensorBlipType::Register(L, 0, 0, 0, Lua_SensorBlipType_Mnemonics);
	Lua_SensorBlipType::Valid = Lua_SensorBlipType::ValidRange(NUMBER_OF_MDISPTYPES);
	
	Lua_SensorBlipTypes::Register(L);
	Lua_SensorBlipTypes::Length = Lua_SensorBlipTypes::ConstantLength(NUMBER_OF_MDISPTYPES);
	
	Lua_TextureType::Register(L, 0, 0, 0, Lua_TextureType_Mnemonics);
	Lua_TextureType::Valid = Lua_TextureType::ValidRange(NUMBER_OF_LUA_TEXTURE_TYPES);
	
	Lua_TextureTypes::Register(L);
	Lua_TextureTypes::Length = Lua_TextureTypes::ConstantLength(NUMBER_OF_LUA_TEXTURE_TYPES);
	
	Lua_WeaponType::Register(L, 0, 0, 0, Lua_WeaponType_Mnemonics);
	Lua_WeaponType::Valid = Lua_WeaponType::ValidRange(MAXIMUM_NUMBER_OF_WEAPONS);

	Lua_WeaponTypes::Register(L);
	Lua_WeaponTypes::Length = Lua_WeaponTypes::ConstantLength((int16) MAXIMUM_NUMBER_OF_WEAPONS);
	
	Lua_HUDPlayer_Weapon::Register(L, Lua_HUDPlayer_Weapon_Get);
	Lua_HUDPlayer_Weapon::Valid = Lua_HUDPlayer_Weapon::ValidRange(MAXIMUM_NUMBER_OF_WEAPONS);
	
	Lua_HUDPlayer_Weapons::Register(L, 0, 0, Lua_HUDPlayer_Weapons_Metatable);
	
	Lua_HUDPlayer_Section::Register(L, Lua_HUDPlayer_Section_Get);
	Lua_HUDPlayer_Section::Valid = Lua_HUDPlayer_Section::ValidRange(NUMBER_OF_ITEM_TYPES + 1);
	
	Lua_HUDPlayer_Sections::Register(L, 0, 0, Lua_HUDPlayer_Sections_Metatable);
	
	Lua_HUDPlayer_Weapon_Trigger::Register(L, Lua_HUDPlayer_Weapon_Trigger_Get);
	Lua_HUDPlayer_Weapon_Trigger::Valid = Lua_HUDPlayer_Weapon_Trigger::ValidRange((int) _secondary_weapon + 1);
	
	Lua_HUDPlayer_Item::Register(L, Lua_HUDPlayer_Item_Get);
	Lua_HUDPlayer_Item::Valid = Lua_HUDPlayer_Item::ValidRange(NUMBER_OF_DEFINED_ITEMS);

	Lua_HUDPlayer_Items::Register(L, 0, 0, Lua_HUDPlayer_Items_Metatable);
	
	Lua_MotionSensor_Blip::Register(L, Lua_MotionSensor_Blip_Get);
	Lua_MotionSensor_Blip::Valid = Lua_MotionSensor_Blip_Valid;
	
	Lua_MotionSensor_Blips::Register(L, 0, 0, Lua_MotionSensor_Blips_Metatable);
	
	Lua_MotionSensor::Register(L, Lua_MotionSensor_Get);

	Lua_HUDCompass::Register(L, Lua_HUDCompass_Get);

    Lua_HUDTexturePalette_Slot::Register(L, Lua_HUDTexturePalette_Slot_Get);
    Lua_HUDTexturePalette_Slot::Valid = Lua_HUDTexturePalette_Slot_Valid;
    
	Lua_HUDTexturePalette_Slots::Register(L, 0, 0, Lua_HUDTexturePalette_Slots_Metatable);
    
	Lua_HUDTexturePalette::Register(L, Lua_HUDTexturePalette_Get);
    
	Lua_HUDPlayer::Register(L, Lua_HUDPlayer_Get);
	Lua_HUDPlayer::Push(L, 0);
	lua_setglobal(L, Lua_HUDPlayer_Name);

	Lua_Screen_Clip_Rect::Register(L, Lua_Screen_Clip_Rect_Get, Lua_Screen_Clip_Rect_Set);
	Lua_Screen_World_Rect::Register(L, Lua_Screen_World_Rect_Get, Lua_Screen_World_Rect_Set);
	Lua_Screen_Map_Rect::Register(L, Lua_Screen_Map_Rect_Get, Lua_Screen_Map_Rect_Set);
	Lua_Screen_Term_Rect::Register(L, Lua_Screen_Term_Rect_Get, Lua_Screen_Term_Rect_Set);
	Lua_Screen_FOV::Register(L, Lua_Screen_FOV_Get, Lua_Screen_FOV_Set);
	
	Lua_Screen::Register(L, Lua_Screen_Get);
	Lua_Screen::Push(L, 0);
	lua_setglobal(L, Lua_Screen_Name);
	
	Lua_HUDGame_Player::Register(L, Lua_HUDGame_Player_Get);
	Lua_HUDGame_Player::Valid = Lua_HUDGame_Player_Valid;
	
	Lua_HUDGame_Players::Register(L, 0, 0, Lua_HUDGame_Players_Metatable);
	
	Lua_HUDGame::Register(L, Lua_HUDGame_Get);
	Lua_HUDGame::Push(L, 0);
	lua_setglobal(L, Lua_HUDGame_Name);

	Lua_Font::Register(L, Lua_Font_Get, 0, Lua_Font_Metatable);
	
	Lua_Fonts::Register(L, Lua_Fonts_Get);
	Lua_Fonts::Push(L, 0);
	lua_setglobal(L, Lua_Fonts_Name);

	Lua_Image_Crop_Rect::Register(L, Lua_Image_Crop_Rect_Get, Lua_Image_Crop_Rect_Set);
	Lua_Image_Crop_Rect::Valid = Lua_Image::Valid;
	
	Lua_Image::Register(L, Lua_Image_Get, Lua_Image_Set, Lua_Image_Metatable);
	
	Lua_Images::Register(L, Lua_Images_Get);
	Lua_Images::Push(L, 0);
	lua_setglobal(L, Lua_Images_Name);
    
	Lua_Shape_Crop_Rect::Register(L, Lua_Shape_Crop_Rect_Get, Lua_Shape_Crop_Rect_Set);
	Lua_Shape_Crop_Rect::Valid = Lua_Shape::Valid;
	
	Lua_Shape::Register(L, Lua_Shape_Get, Lua_Shape_Set, Lua_Shape_Metatable);
	
	Lua_Shapes::Register(L, Lua_Shapes_Get);
	Lua_Shapes::Push(L, 0);
	lua_setglobal(L, Lua_Shapes_Name);
    
    Lua_HUDLighting_Fader::Register(L, Lua_HUDLighting_Fader_Get);
    Lua_HUDLighting_Fader::Valid = Lua_HUDLighting_Fader::ValidRange(NUMBER_OF_FADER_QUEUE_ENTRIES);
    
    Lua_HUDLighting::Register(L, Lua_HUDLighting_Get);
    Lua_HUDLighting::Push(L, 0);
    lua_setglobal(L, Lua_HUDLighting_Name);
	
	return 0;
}

#endif
#endif
