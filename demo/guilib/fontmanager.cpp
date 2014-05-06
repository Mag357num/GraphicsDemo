/*
 * Copyright (c) 2014 Roman Kuznetsov 
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "fontmanager.h"
#include "logger.h"
#include "uimanager.h"

#include <map>
#include <sstream>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>

namespace gui
{

const int DPI = 96;

namespace 
{

int findNearestPower2(int v)
{
	int i = 2;
	while (i < v) i <<= 1;
	return i;
}

}

class FreeTypeWrapper
{
public:
	FreeTypeWrapper(){}
	~FreeTypeWrapper(){}

	bool init()
	{
		if (FT_Init_FreeType(&m_library))
		{
			utils::Logger::toLog("Error: could not not initialize FreeType library.\n");
			return false;
		}
		return true;
	}

	void destroy()
	{
		FT_Done_FreeType(m_library);
	}

	bool loadFont(Font& font, const std::string& name, size_t height)
	{
		// create font resource
		if (!UIManager::instance().factory())
		{
			utils::Logger::toLog("Error: could not find an instance of UIResourcesFactory.\n");
			return false;
		}

		FT_Face face;
		if (FT_New_Face(m_library, name.c_str(), 0, &face))
		{
			utils::Logger::toLogWithFormat("Error: could not not load font '%s'.\n", name.c_str());
			return false;
		}

		FT_Set_Char_Size(face, height << 6, height << 6, DPI, DPI);

		// build name
		std::stringstream str;
		str << face->family_name << " " << height;
		font.m_name = str.str();

		// init maximum buffer
		const int MAX_BUFFER_SIZE = 2048;
		std::unique_ptr<unsigned char[]> fontBuffer(new unsigned char[MAX_BUFFER_SIZE * MAX_BUFFER_SIZE]);
		memset(fontBuffer.get(), 0, MAX_BUFFER_SIZE * MAX_BUFFER_SIZE);

		int offsetX = 0;
		int offsetY = 0;
		int maxHeight = 0;
		int maxWidth = 0;
		FT_GlyphSlot slot = face->glyph;
		
		font.m_charToGlyph.resize(128);
		std::map<unsigned int, unsigned int> glyphs;
		unsigned int charToGlyphIndex = 0;
		for (short index = 0; index < 128; index++)
		{
			// create association between characters and glyphs
			FT_UInt glyphIndex = FT_Get_Char_Index(face, index);
			if (glyphs.find(glyphIndex) == glyphs.end()) 
			{
				glyphs[glyphIndex] = charToGlyphIndex;
			}
			else 
			{
				font.m_charToGlyph[index] = glyphs[glyphIndex];
				continue;
			}
			
			// load a glyph
			if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT))
			{
				utils::Logger::toLogWithFormat("Error: could not create a font '%s'. A glyph loading failed.\n", font.getName().c_str());
				FT_Done_Face(face);
				return false;
			}

			// render a glyph
			if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL))
			{
				utils::Logger::toLogWithFormat("Error: could not create a font '%s'. A glyph rendering failed.\n", font.getName().c_str());
				FT_Done_Face(face);
				return false;
			}

			// read glyph's parameters
			int w = (int)slot->metrics.width >> 6;
			int h = (int)slot->metrics.height >> 6;
			int ax = (int)slot->advance.x >> 6;
			int bx = (int)slot->metrics.horiBearingX >> 6;
			int by = (int)slot->metrics.horiBearingY >> 6;

			if (offsetX + w >= MAX_BUFFER_SIZE)
			{
				offsetX = 0;
				offsetY += maxHeight;
				maxHeight = h;
			}
			if (offsetY + h >= MAX_BUFFER_SIZE)
			{
				utils::Logger::toLogWithFormat("Error: could not create a font '%s'. Maximum buffer size was exceeded.\n", font.getName().c_str());
				FT_Done_Face(face);
				return false;
			}

			if (h > maxHeight) maxHeight = h;

			// copy a glyph to buffer
			for (int j = 0; j < h; j++) 
			{
				for (int i = 0; i < w; i++)
				{
					fontBuffer[(i + offsetX) + slot->bitmap.width * (j + offsetY)] = slot->bitmap.buffer[i + slot->bitmap.width * j];
				}
			}

			offsetX += w;
			if (offsetX > maxWidth + 1) maxWidth = offsetX - 1;

			font.m_charToGlyph[index] = charToGlyphIndex;
			charToGlyphIndex++;
		}

		FT_Done_Face(face);

		// determine actual buffer size
		size_t textureWidth = findNearestPower2(maxWidth);
		size_t textureHeight = findNearestPower2(maxHeight);

		std::vector<unsigned char> outputBuffer;
		outputBuffer.resize(textureWidth * textureHeight);
		memset(outputBuffer.data(), 0, textureWidth * textureHeight);
		for (size_t j = 0; j < textureHeight; j++)
		{
			memcpy((void*)(outputBuffer.data() + j * textureWidth), fontBuffer.get() + j * MAX_BUFFER_SIZE, textureWidth);
		}
		fontBuffer.reset();

		// create a resource
		font.m_resource = UIManager::instance().factory()->createFontResource();
		if (font.m_resource.expired() ||
			!font.m_resource.lock()->createResource(font, outputBuffer, textureWidth, textureHeight))
		{
			utils::Logger::toLogWithFormat("Error: could not create a resource for the font '%s'.\n", font.getName().c_str());
			return false;
		}

		return true;
	}

private:
	FT_Library m_library;
};

bool FontManager::init()
{
	m_freetype.reset(new FreeTypeWrapper());
	if (!m_freetype->init()) 
	{ 
		m_freetype.reset(); 
		return false; 
	}

	return true;
}

void FontManager::destroy()
{
	if (m_freetype) { m_freetype->destroy(); }
}

Font FontManager::createFont(const std::string& fontPath, size_t height)
{
	Font font;
	if (!m_freetype->loadFont(font, fontPath, height)) 
		return std::move(Font());

	return std::move(font);
}

}