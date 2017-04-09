#pragma once

#include <algorithm>
#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <vector>
#include <inttypes.h>
#include <Windows.h>
#include <assert.h>

enum DiffColor
{
	White = 0,
	Gray10 = 1,
	Gray50 = 2,
	Gray90 = 3,
	Pink = 4,
	Red = 5,
	Orange = 6,
	DarkOrange = 7,
	Yellow = 8,
	LimeGreen = 9,
	Green = 10,
	LightBlue = 11,
	MediumBlue = 12,
	Blue = 13,
	LightPurle = 14,
	Purple = 15
};

#pragma pack(push, 2)

class PlaceDiff
{
public:
	uint32_t	timestamp;
	uint32_t	x;
	uint32_t	y;
	DiffColor	color;
};

class BitMapColor
{
public:
	BitMapColor()
		: m_red(0xFF)
		, m_green(0xFF)
		, m_blue(0xFF)
	{
	}

	BitMapColor(DiffColor diff_color)
	{
		uint32_t color = BitMapColor::Convert(diff_color);
		m_blue	= (color & 0x00FF0000) >> 16;
		m_green = (color & 0x0000FF00) >> 8;
		m_red	= (color & 0x000000FF);
	}

	BitMapColor(uint8_t r, uint8_t g, uint8_t b)
		: m_red(r)
		, m_green(g)
		, m_blue(b)
	{
	}

	static uint32_t Convert(DiffColor color)
	{
		switch (color)
		{
			default:
			case White:			return 0xFFFFFF;
			case Gray10:		return 0xE4E4E4;
			case Gray50:		return 0x888888;
			case Gray90:		return 0x222222;
			case Pink:			return 0xFFA7D1;
			case Red:			return 0xE50000;
			case Orange:		return 0xE59500;
			case DarkOrange:	return 0xA06A42;
			case Yellow:		return 0xE5D900;
			case LimeGreen:		return 0x94E044;
			case Green:			return 0x02BE01;
			case LightBlue:		return 0x00D3DD;
			case MediumBlue:	return 0x0083C7;
			case Blue:			return 0x0000EA;
			case LightPurle:	return 0xCF6EE4;
			case Purple:		return 0x820080;
		}
	}

private:
	uint8_t m_red;
	uint8_t m_green;
	uint8_t m_blue;
};

class BitMapInfoHeader
{
public:
	BitMapInfoHeader() = delete;
	BitMapInfoHeader(int32_t width, int32_t height, int8_t color_res = 24)
		: m_Size(sizeof(BitMapInfoHeader))
		, m_Width(width)				// width height (pixels)
		, m_Height(height)				// image height (pixels)
		, m_Planes(1)					// 1 plane
		, m_ColorBitCount(color_res)	// 4 bit colot, 16 bit color, 24 bit color, etc...
		, m_Compression(0)				// RGB
		, m_SizeImage(height * ((width * sizeof(BitMapColor)) + ((4 - (width * sizeof(BitMapColor)) % 4) % 4))) // row count * (4 byte aligned column byte count)
		, m_XPixelsPerM(0)
		, m_YPixelsPerM(0)
		, m_ColorUsed(0)
		, m_ColorImportant(0)
	{
	}

	int32_t Width()	 const { return m_Width; }
	int32_t Height() const { return m_Height; }

private:
	uint32_t	m_Size;				// specifies the size of the BITMAPINFOHEADER structure, in bytes.
	int32_t		m_Width;			// specifies the width of the image, in pixels.
	int32_t		m_Height;			// specifies the height of the image, in pixels.
	uint16_t	m_Planes;			// specifies the number of planes of the target device, must be set to zero.
	uint16_t	m_ColorBitCount;	// specifies the number of bits per pixel - the color resolution. (1 = black/white, 4 = 16 colors, 8 = 256 colors, 24 = 16.7 million colors)
	uint32_t	m_Compression;		// Specifies the type of compression, usually set to zero (no compression).
	uint32_t	m_SizeImage;		// specifies the size of the image data, in bytes. If there is no compression, it is valid to set this member to zero.
	int32_t		m_XPixelsPerM;		// specifies the the horizontal pixels per meter on the designated targer device, usually set to zero.
	int32_t		m_YPixelsPerM;		// specifies the the vertical pixels per meter on the designated targer device, usually set to zero.
	uint32_t	m_ColorUsed;		// specifies the number of colors used in the bitmap, if set to zero the number of colors is calculated using the biBitCount member.
	uint32_t	m_ColorImportant;	// specifies the number of color that are 'important' for the bitmap, if set to zero, all colors are important.
};

class BitMapFileHeader
{
public:
	BitMapFileHeader(int32_t width, int32_t height)
		: m_Type(19778) // = 'B' + 'M' = bitmap
		, m_FileSize(sizeof(BitMapFileHeader) + sizeof(BitMapInfoHeader) + (height * (width * sizeof(BitMapColor))))
		, m_Reserved1(0)
		, m_Reserved2(0)
		, m_Offset(0)
	{
	}

	uint32_t FileSize() const
	{
		return m_FileSize;
	}

private:
	uint16_t m_Type;		// must always be set to 'BM' to declare that this is a .bmp-file.
	uint32_t m_FileSize;	// specifies the total size of the bmp file in bytes.
	uint16_t m_Reserved1;	// must always be set to zero.
	uint16_t m_Reserved2;	// must always be set to zero.
	uint32_t m_Offset;		// specifies the offset from the beginning of the file to the bitmap data.
};

class BitMapCore
{
public:
	BitMapCore(int32_t width, int32_t height, const std::string& name = "name")
		: m_FileHeader(width, height)
		, m_InfoHeader(width, height, 24)
		, m_Name(name)
	{
		m_PixelData.resize(height);
		for (auto& vec : m_PixelData)
			vec.resize(width);

		m_Colors = {
			BitMapColor::Convert(White),
			BitMapColor::Convert(Gray10),
			BitMapColor::Convert(Gray50),
			BitMapColor::Convert(Gray90),
			BitMapColor::Convert(Pink),
			BitMapColor::Convert(Red),
			BitMapColor::Convert(Orange),
			BitMapColor::Convert(DarkOrange),
			BitMapColor::Convert(Yellow),
			BitMapColor::Convert(LimeGreen),
			BitMapColor::Convert(Green),
			BitMapColor::Convert(LightBlue),
			BitMapColor::Convert(MediumBlue),
			BitMapColor::Convert(Blue),
			BitMapColor::Convert(LightPurle),
			BitMapColor::Convert(Purple)
		};
	}

	BitMapCore(const BitMapCore& rhs)
		: m_FileHeader(rhs.m_FileHeader)
		, m_InfoHeader(rhs.m_InfoHeader)
		, m_Colors(rhs.m_Colors)
		, m_PixelData(rhs.m_PixelData)
		, m_BitmapBits(rhs.m_BitmapBits)
		, m_Name(rhs.m_Name)
	{
		m_PixelData.resize(rhs.m_InfoHeader.Height());
		for (size_t i = 0; i < m_PixelData.size(); ++i)
			m_PixelData[i] = rhs.m_PixelData[i];
	}

	~BitMapCore()
	{
	}

	void SetName(const std::string& name)
	{
		m_Name = name + ".bmp";
	}

	bool Update(const std::vector<PlaceDiff>& timestep)
	{
		for (const auto& pixel : timestep)
		{
			BitMapColor color(pixel.color);
			uint32_t row = pixel.y;
			uint32_t col = pixel.x;

			if (row >= m_PixelData.size())
				return false;

			std::vector<BitMapColor>& bmp_row = m_PixelData[row];
			if (col >= bmp_row.size())
				return false;

			bmp_row[col] = color;
		}

		return true;
	}

	std::vector<char> GenerateBMPData() const
	{
		std::vector<char> bmp_data;
		bmp_data.resize(m_FileHeader.FileSize());

		size_t bytes_written = 0;

		// write bmp file header
		const auto& file_header_begin = reinterpret_cast<const char*>(&m_FileHeader);
		const auto& file_header_end = file_header_begin + sizeof(m_FileHeader);
		std::copy(file_header_begin, file_header_end, bmp_data.begin() + bytes_written);
		bytes_written += sizeof(m_FileHeader);

		// write bmp info header
		const auto& info_header_begin = reinterpret_cast<const char*>(&m_InfoHeader);
		const auto& info_header_end = info_header_begin + sizeof(m_InfoHeader);
		std::copy(info_header_begin, info_header_end, bmp_data.begin() + bytes_written);
		bytes_written += sizeof(m_InfoHeader);

		// write image data
		const auto height = m_InfoHeader.Height();
		const auto width = m_InfoHeader.Width();
		for (int32_t row = height; row--; /*empty*/)
		{
			for (int32_t col = 0; col < width; ++col)
			{
				const BitMapColor& rgb = m_PixelData[row][col];
				const auto& rgb_begin = reinterpret_cast<const char*>(&rgb);
				const auto& rgb_end = rgb_begin + sizeof(rgb);
				std::copy(rgb_begin, rgb_end, bmp_data.begin() + bytes_written);
				bytes_written += sizeof(rgb);
			}

			// pad each row to ensure 4 byte alignment
			static const char padding[4] = { 0, 0, 0, 0 };
			const auto row_bytes = sizeof(BitMapColor) * width;
			const auto pad_bytes = (4 - (row_bytes % 4)) % 4;
			const auto& padding_begin = reinterpret_cast<const char*>(&padding);
			const auto& padding_end = padding_begin + pad_bytes;
			std::copy(padding_begin, padding_end, bmp_data.begin() + bytes_written);
			bytes_written += pad_bytes;
		}

		assert(bytes_written == m_FileHeader.FileSize());

		return bmp_data;
	}

	bool Write() const
	{
		std::ofstream bmp(m_Name, std::ios::out | std::ios::binary);
		if (bmp.is_open())
		{
			std::vector<char> data = GenerateBMPData();
			bmp.write(data.data(), data.size());
			bmp.close();
		}

		return true;
	}

private:
	BitMapFileHeader						m_FileHeader;
	BitMapInfoHeader						m_InfoHeader;
	std::vector<uint32_t>					m_Colors;
	std::vector<std::vector<BitMapColor>>	m_PixelData;
	std::vector<uint8_t>					m_BitmapBits;
	std::string								m_Name;
};

ref class DiffLoadThreadParam
{
public:
	DiffLoadThreadParam(const std::string& file_path, std::vector<std::vector<PlaceDiff>>* pDiffData, BitMapCore* pBaseBMP, BitMapCore* pLastBMP)
		: m_file_path(new std::string(file_path))
		, m_pDiffData(pDiffData)
		, m_pBaseBitmap(pBaseBMP)
		, m_pLastBitmap(pLastBMP)
	{ }

	~DiffLoadThreadParam()
	{
		if (m_file_path == nullptr)
			delete m_file_path;
		m_file_path = nullptr;
	}

	const std::string*						m_file_path;
	std::vector<std::vector<PlaceDiff>>*	m_pDiffData;
	BitMapCore*								m_pBaseBitmap;
	BitMapCore*								m_pLastBitmap;
};

#pragma pack(pop, 2)