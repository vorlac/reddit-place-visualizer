#pragma once

#include "diff_parser.h"

#include <algorithm>
#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <vector>
#include <inttypes.h>
#include <Windows.h>
#include <assert.h>
#include <msclr\marshal_cppstd.h>

namespace place_gui 
{
	using namespace System;
	using namespace System::IO;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;
	using namespace System::ComponentModel;
	using namespace msclr::interop;

	public ref class PlaceVisualizerForm : public Form
	{
	public:
		PlaceVisualizerForm(void)
			: m_LastBitmapIndex(0)
			, m_pForwardDiffData(nullptr)
			, m_pReverseDiffData(nullptr)
			, m_pBaseBitmap(nullptr)
			, m_pLastBitmap(nullptr)
		{
			InitializeComponent();
			this->m_pTrackBar->Enabled = false;
		}

		void UpdateStatusLabel(String^ msg)
		{
			m_pProgressLabel->Text = msg;
		}

		void LoadThreadComplete(Image^ image)
		{
			m_pPictureBox->Image = image;
			m_pTrackBar->Enabled = true;
			m_pTrackBar->Minimum = 0;
			m_pTrackBar->Maximum = static_cast<int>(m_pForwardDiffData->size());
		}

		// Thread function
		void LoadPlaceDiffsFile(System::Object^ threadParam)
		{
			DiffLoadThreadParam^ param = (DiffLoadThreadParam^) threadParam;
			std::ifstream diffs_file(*param->m_file_path, std::ios::in | std::ios::binary);
			if (param->m_pDiffData != nullptr && diffs_file.is_open())
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
					{
						int progress = static_cast<int>(((diffs_file.tellg() - file_start) / file_length) * 100.0);
						Control::Invoke(gcnew Action<String^>(this, &PlaceVisualizerForm::UpdateStatusLabel), "Loading diff file (" + progress + "%)");
					}

					diffs_file.read(reinterpret_cast<char*>(&diff), sizeof(diff));
					if (diff.timestamp != timestamp)
					{
						param->m_pDiffData->resize(param->m_pDiffData->size() + 1);
						timestamp = diff.timestamp;
					}
					param->m_pDiffData->back().push_back(diff);
				}

				diffs_file.close();
				if (param->m_pDiffData->size() > 0)
				{
					Control::Invoke(gcnew Action<String^>(this, &PlaceVisualizerForm::UpdateStatusLabel), "Diff file loaded successfully");
					param->m_pBaseBitmap = new BitMapCore(1000, 1000);
					if (m_pForwardDiffData->size() > 0)
					{
						if (m_pForwardDiffData->at(0).size() > 0)
						{
							const auto& diff_step = m_pForwardDiffData->at(0);
							param->m_pBaseBitmap->Update(diff_step);
							auto image_data = param->m_pBaseBitmap->GenerateBMPData();
							array<Byte>^ pBaseImage = gcnew array<Byte>(static_cast<int>(image_data.size()));
							Marshal::Copy((IntPtr)image_data.data(), pBaseImage, 0, static_cast<int>(image_data.size()));
							MemoryStream^ ms = gcnew MemoryStream(pBaseImage);
							Image^ pImage = Bitmap::FromStream(ms);
							Control::Invoke(gcnew Action<Image^>(this, &PlaceVisualizerForm::LoadThreadComplete), gcnew Bitmap(pImage));
						}
					}
					*param->m_pLastBitmap = BitMapCore(*param->m_pBaseBitmap);
				}
			}
		}

		void LoadPlaceDiffs(const std::string& file_path)
		{
			if (m_pForwardDiffData != nullptr)
				delete m_pForwardDiffData;
			if (m_pBaseBitmap != nullptr)
				delete m_pBaseBitmap;
			if (m_pLastBitmap != nullptr)
				delete m_pLastBitmap;

			System::Threading::Thread^ pLoadThread = 
				gcnew System::Threading::Thread(
					gcnew System::Threading::ParameterizedThreadStart(
						this, &PlaceVisualizerForm::LoadPlaceDiffsFile));

			m_pBaseBitmap = new BitMapCore(1000, 1000);
			m_pLastBitmap = new BitMapCore(1000, 1000);
			m_pForwardDiffData = new std::vector<std::vector<PlaceDiff>>();			
			pLoadThread->Start(gcnew DiffLoadThreadParam(file_path, m_pForwardDiffData, m_pBaseBitmap, m_pLastBitmap));
		}

		void UpdatePlaceImage(int step)
		{
			if (m_pForwardDiffData == nullptr)
				return;
			if (m_pBaseBitmap == nullptr)
				return;
			if (m_pLastBitmap == nullptr)
				return;

			size_t diff_index = step;
			if (m_pForwardDiffData->size() > diff_index)
			{
				// start with last bitmap that was drawn
				size_t start_idx = (diff_index > m_LastBitmapIndex) ? m_LastBitmapIndex + 1 : 0;
				BitMapCore* pNewBitmap = new BitMapCore(start_idx == 0 ? *m_pBaseBitmap : *m_pLastBitmap);
				m_pProgressLabel->Text = step + " / " + m_pForwardDiffData->size();

				for (size_t i = start_idx; i < diff_index; ++i)
				{
					// play back any diff data since the last step drawn
					const std::vector<PlaceDiff>& diff_step = m_pForwardDiffData->at(i);
					pNewBitmap->Update(diff_step);
				}

				auto image_data = pNewBitmap->GenerateBMPData();
				array<Byte>^ pImageData = gcnew array<Byte>(static_cast<int>(image_data.size()));
				Marshal::Copy((IntPtr)image_data.data(), pImageData, 0, static_cast<int>(image_data.size()));
				MemoryStream^ ms = gcnew MemoryStream(pImageData);
				Image^ pNewImage = Bitmap::FromStream(ms);
				m_pPictureBox->Image = pNewImage;
				m_LastBitmapIndex = diff_index;

				if (m_pLastBitmap != nullptr)
					delete m_pLastBitmap;
				m_pLastBitmap = pNewBitmap;
			}
		}

	protected:
		~PlaceVisualizerForm()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		System::Void trackBar1_Scroll(System::Object^ sender, System::EventArgs^ e)
		{
			TrackBar^ myTB = dynamic_cast<TrackBar^>(sender);
			this->UpdatePlaceImage(myTB->Value);
		}

		System::Void loadDiffToolStripMenuItem_Click(System::Object^ sender, System::EventArgs^ e) 
		{
			OpenFileDialog fd;
			if (fd.ShowDialog() == System::Windows::Forms::DialogResult::OK)
			{
				std::string file_path = marshal_as<std::string>(fd.FileName);
				LoadPlaceDiffs(file_path);
			}
		}

	private: 
		// UI Widgets
		System::Windows::Forms::PictureBox^			m_pPictureBox;
		System::Windows::Forms::TableLayoutPanel^	m_pTableLayout;
		System::Windows::Forms::TrackBar^			m_pTrackBar;
		System::Windows::Forms::MenuStrip^			menuStrip1;
		System::Windows::Forms::ToolStripMenuItem^  fileToolStripMenuItem;
		System::Windows::Forms::ToolStripMenuItem^  loadDiffToolStripMenuItem;
		System::Windows::Forms::Label^				m_pProgressLabel;

		// Required designer variable.
		System::ComponentModel::Container^			components;

		std::vector<std::vector<PlaceDiff>>*		m_pForwardDiffData;
		std::vector<std::vector<PlaceDiff>>*		m_pReverseDiffData;
		BitMapCore*									m_pBaseBitmap;
		BitMapCore*									m_pLastBitmap;
		size_t										m_LastBitmapIndex;

#pragma region Windows Form Designer generated code
		// Required method for Designer support - do not modify
		// the contents of this method with the code editor.
		void InitializeComponent(void)
		{
			this->m_pPictureBox = (gcnew System::Windows::Forms::PictureBox());
			this->m_pTableLayout = (gcnew System::Windows::Forms::TableLayoutPanel());
			this->m_pTrackBar = (gcnew System::Windows::Forms::TrackBar());
			this->menuStrip1 = (gcnew System::Windows::Forms::MenuStrip());
			this->fileToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->loadDiffToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->m_pProgressLabel = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->m_pPictureBox))->BeginInit();
			this->m_pTableLayout->SuspendLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->m_pTrackBar))->BeginInit();
			this->menuStrip1->SuspendLayout();
			this->SuspendLayout();
			// 
			// m_pPictureBox
			// 
			this->m_pPictureBox->Dock = System::Windows::Forms::DockStyle::Fill;
			this->m_pPictureBox->Location = System::Drawing::Point(0, 0);
			this->m_pPictureBox->Margin = System::Windows::Forms::Padding(0);
			this->m_pPictureBox->Name = L"m_pPictureBox";
			this->m_pPictureBox->Size = System::Drawing::Size(634, 615);
			this->m_pPictureBox->SizeMode = System::Windows::Forms::PictureBoxSizeMode::StretchImage;
			this->m_pPictureBox->TabIndex = 3;
			this->m_pPictureBox->TabStop = false;
			// 
			// m_pTableLayout
			// 
			this->m_pTableLayout->AutoSizeMode = System::Windows::Forms::AutoSizeMode::GrowAndShrink;
			this->m_pTableLayout->ColumnCount = 1;
			this->m_pTableLayout->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
				100)));
			this->m_pTableLayout->Controls->Add(this->m_pPictureBox, 0, 0);
			this->m_pTableLayout->Controls->Add(this->m_pTrackBar, 0, 1);
			this->m_pTableLayout->Dock = System::Windows::Forms::DockStyle::Fill;
			this->m_pTableLayout->Location = System::Drawing::Point(0, 24);
			this->m_pTableLayout->Margin = System::Windows::Forms::Padding(10);
			this->m_pTableLayout->Name = L"m_pTableLayout";
			this->m_pTableLayout->RowCount = 2;
			this->m_pTableLayout->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 93.31849F)));
			this->m_pTableLayout->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 6.681514F)));
			this->m_pTableLayout->Size = System::Drawing::Size(634, 660);
			this->m_pTableLayout->TabIndex = 1;
			// 
			// m_pTrackBar
			// 
			this->m_pTrackBar->Dock = System::Windows::Forms::DockStyle::Fill;
			this->m_pTrackBar->LargeChange = 100;
			this->m_pTrackBar->Location = System::Drawing::Point(15, 630);
			this->m_pTrackBar->Margin = System::Windows::Forms::Padding(15);
			this->m_pTrackBar->Maximum = 1;
			this->m_pTrackBar->Name = L"m_pTrackBar";
			this->m_pTrackBar->Size = System::Drawing::Size(604, 15);
			this->m_pTrackBar->TabIndex = 1;
			this->m_pTrackBar->Scroll += gcnew System::EventHandler(this, &PlaceVisualizerForm::trackBar1_Scroll);
			// 
			// menuStrip1
			// 
			this->menuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->fileToolStripMenuItem });
			this->menuStrip1->Location = System::Drawing::Point(0, 0);
			this->menuStrip1->Name = L"menuStrip1";
			this->menuStrip1->Size = System::Drawing::Size(634, 24);
			this->menuStrip1->TabIndex = 2;
			this->menuStrip1->Text = L"menuStrip1";
			// 
			// fileToolStripMenuItem
			// 
			this->fileToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->loadDiffToolStripMenuItem });
			this->fileToolStripMenuItem->Name = L"fileToolStripMenuItem";
			this->fileToolStripMenuItem->Size = System::Drawing::Size(37, 20);
			this->fileToolStripMenuItem->Text = L"File";
			// 
			// loadDiffToolStripMenuItem
			// 
			this->loadDiffToolStripMenuItem->Name = L"loadDiffToolStripMenuItem";
			this->loadDiffToolStripMenuItem->Size = System::Drawing::Size(131, 22);
			this->loadDiffToolStripMenuItem->Text = L"Load Diff...";
			this->loadDiffToolStripMenuItem->Click += gcnew System::EventHandler(this, &PlaceVisualizerForm::loadDiffToolStripMenuItem_Click);
			// 
			// m_pProgressLabel
			// 
			this->m_pProgressLabel->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Right));
			this->m_pProgressLabel->Location = System::Drawing::Point(316, 1);
			this->m_pProgressLabel->Name = L"m_pProgressLabel";
			this->m_pProgressLabel->Size = System::Drawing::Size(318, 23);
			this->m_pProgressLabel->TabIndex = 3;
			this->m_pProgressLabel->Text = L"Please load diff file";
			this->m_pProgressLabel->TextAlign = System::Drawing::ContentAlignment::MiddleRight;
			// 
			// PlaceVisualizerForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(634, 684);
			this->Controls->Add(this->m_pProgressLabel);
			this->Controls->Add(this->m_pTableLayout);
			this->Controls->Add(this->menuStrip1);
			this->MainMenuStrip = this->menuStrip1;
			this->Name = L"PlaceVisualizerForm";
			this->Text = L"r/place visualizer";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->m_pPictureBox))->EndInit();
			this->m_pTableLayout->ResumeLayout(false);
			this->m_pTableLayout->PerformLayout();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->m_pTrackBar))->EndInit();
			this->menuStrip1->ResumeLayout(false);
			this->menuStrip1->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion		
	};
}
