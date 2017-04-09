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
	White = 0,	// #FFFFFF	
	Gray10 = 1,	// #E4E4E4	
	Gray50 = 2,	// #888888	
	Gray90 = 3,	// #222222	
	Pink = 4,	// #FFA7D1	
	Red = 5,	// #E50000	
	Orange = 6,	// #E59500	
	DarkOrange = 7,	// #A06A42	
	Yellow = 8,	// #E5D900	
	LimeGreen = 9,	// #94E044	
	Green = 10,	// #02BE01	
	LightBlue = 11,	// #00D3DD	
	MediumBlue = 12,	// #0083C7	
	Blue = 13,	// #0000EA	
	LightPurle = 14,	// #CF6EE4	
	Purple = 15	// #820080	
};

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
		m_blue = (color & 0x00FF0000) >> 16;
		m_green = (color & 0x0000FF00) >> 8;
		m_red = (color & 0x000000FF);
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
		return 0;
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
		: m_Type(19778)
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

class BitMap
{
public:
	BitMap(const std::string& name, int32_t width, int32_t height)
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

	~BitMap()
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

	std::vector<char> GenerateBMP() const
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
		std::ofstream bmp2(m_Name, std::ios::out | std::ios::binary);
		if (bmp2.is_open())
		{
			std::vector<char> data = GenerateBMP();
			bmp2.write(data.data(), data.size());
			bmp2.close();
		}

		return true;
	}

	bool WriteBMP() const
	{

		std::ofstream bmp(m_Name, std::ios::out | std::ios::binary);
		if (bmp.is_open())
		{
			// write bmp file header
			bmp.write(reinterpret_cast<const char*>(&m_FileHeader), sizeof(m_FileHeader));

			// write bmp info header
			bmp.write(reinterpret_cast<const char*>(&m_InfoHeader), sizeof(m_InfoHeader));

			// write pixel data
			const auto height = m_InfoHeader.Height();
			const auto width = m_InfoHeader.Width();
			for (int32_t row = height; row--; /*empty*/)
			{
				for (int32_t col = 0; col < width; ++col)
				{
					const BitMapColor& rgb = m_PixelData[row][col];
					bmp.write(reinterpret_cast<const char*>(&rgb), sizeof(rgb));
				}

				static char padding[4] = { 0,0,0,0 };
				auto row_bytes = sizeof(BitMapColor) * width;
				auto pad_bytes = (4 - (row_bytes % 4)) % 4;
				bmp.write(padding, pad_bytes);
			}
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

void print_progress(double curr, double total, bool step)
{
	double progress = (curr / total) * 100.0;
	switch (step)
	{
		case 0:
			std::cout << "Reading diffs file [" << progress << "%]...\t\r";
			break;
		case 1:
			std::cout << "Generating bitmap " << curr << " of " << total << " [" << progress << "%]...\t\r";
			break;
	}
}

int main()
{
	std::ifstream diffs_file("diffs.bin", std::ios::in | std::ios::binary);

	std::vector<std::vector<PlaceDiff>> diffs;
	if (diffs_file.is_open())
	{
		PlaceDiff diff;
		uint32_t timestamp = 0;

		auto file_start = diffs_file.tellg();
		diffs_file.seekg(0, std::ios::end);
		auto file_end = diffs_file.tellg();
		diffs_file.seekg(0, std::ios::beg);
		double file_length = static_cast<double>(file_end - file_start);

		uint32_t count = 0;
		while (!diffs_file.eof())
		{
			if (++count % 100000 == 0)
				print_progress(static_cast<double>(diffs_file.tellg() - file_start), file_length, 0);

			diffs_file.read(reinterpret_cast<char*>(&diff), sizeof(diff));
			if (diff.timestamp != timestamp)
			{
				diffs.resize(diffs.size() + 1);
				timestamp = diff.timestamp;
			}
			diffs.back().push_back(diff);
		}

		print_progress(100.0, 100.0, 0);
		std::cout << std::endl;
		diffs_file.close();

		std::string name = "place";
		BitMap bmp(name, 1000, 1000);
		if (diffs.size() > 0)
		{
			auto start_time = 0;
			if (diffs[0].size() > 0)
				start_time = diffs[0][0].timestamp;

			const double total_steps = static_cast<double>(diffs.size());
			for (auto step = 0; step < total_steps; ++step)
			{
				if (step % 100 == 0)
					print_progress(step, total_steps, 1);

				const auto& diff_step = diffs[step];
				if (diff_step.size() > 0)
				{
					auto relative_time = diff_step[0].timestamp - start_time;
					bmp.SetName(name + std::to_string(relative_time));
					bmp.Update(diff_step);
					bmp.Write();
				}
			}
		}
	}
	else
	{
		std::cout << "Failed to open diffs file!" << std::endl;
	}
	return 0;
}

