﻿/*
 * qaul.net is free software
 * licensed under GPL (version 3)
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <windows.h> // must not be included when using afxdlgs.h
#include <string>

#include "qaulHelpers.h"

// add libqaul
#include "..\..\libqaul\qaullib.h"
#pragma comment(lib, "libqaul.lib")


namespace qaul {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Threading;
	using namespace System::Diagnostics;

	/// <summary>
	/// Summary for formStart
	/// </summary>
	public ref class formStart : public System::Windows::Forms::Form
	{
	// my private functions
	private:
		// variables
		System::String^ qaulResourcePath;
		int qaulStartCounter;
		bool isXP;
		// wifi config
		DWORD dwResult;
		DWORD dwMaxClient;
		DWORD dwCurVersion;
		//pin_ptr<DWORD> pdwCurVersion;
		// wifi interface & connection variables
		//bool netInterfaceFound;
		//DWORD netInterfaceIndex;
		//GUID netInterfaceGuid;
		qaulInterface* netInterface;

		// functions
		void InitializeQaul(void);
		void ExitQaul(void);
		void QaulStarting(void);

		bool WifiFindInterface(void);
		bool WifiSetProfile(void);
		bool WifiConnectProfile(void);
		bool WifiSetIp(void);
		bool StartOlsr(void);
		bool StopOlsr(void);
		bool StartPortforward(void);
		bool StopPortforward(void);

		// Timers
		static System::Windows::Forms::Timer^ ipcTimer = gcnew System::Windows::Forms::Timer;
		static System::Windows::Forms::Timer^ ipcTimer2 = gcnew System::Windows::Forms::Timer;
		static System::Windows::Forms::Timer^ ipcTimer3 = gcnew System::Windows::Forms::Timer;
		static void formStart::CheckIpcMessage( Object^ myObject, EventArgs^ myEventArgs);
		static void formStart::CheckIpcTopology( Object^ myObject, EventArgs^ myEventArgs);
		static void formStart::CheckAppEvents( Object^ myObject, EventArgs^ myEventArgs);

		// string helpers
		static bool formStart::cvtLPW2stdstring(std::string& s, const LPWSTR pw, UINT codepage);

		// Layout
		bool startFormActive;
		bool chatFormActive;
		bool chatFormInitialized;
		bool errorFormActive;
		bool errorFormInitialized;
		bool qaulTestError;


	public:
		formStart(void)
		{
			// check OS version
			OSVERSIONINFO osvi;
			ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
			osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			if(osvi.dwMajorVersion < 6) isXP = true; // is XP
			
			// set resource Path
			qaulResourcePath = Application::StartupPath::get();
			Debug::WriteLine(System::String::Format("ExecutablePath: {0} ",qaulResourcePath));

			// wifi interface
			//netInterfaceFound = false;
			netInterface = new qaulInterface();

			// initialize layout
			InitializeComponent();
			startFormActive = true;
			chatFormActive = false;
			chatFormInitialized = false;
			errorFormActive = false;
			errorFormInitialized = false;

			// initialize Qaul
			InitializeQaul();
		}

	protected:
		/// <summary>
		/// clean up used resources
		/// </summary>
		~formStart()
		{
			Debug::WriteLine(L"Destroy Window");
			ExitQaul();
			
			if (components)
			{
				delete components;
			}
		}
	
	/**
	 * Views
	 */
	// chat view
	private: System::Windows::Forms::WebBrowser^  webBrowser1;

	// error view
	private: System::Windows::Forms::Button^  btnRetry;
	private: System::Windows::Forms::Label^  lblError;

	
	/**
	 * background worker
	 */
	System::ComponentModel::BackgroundWorker^ backgroundWorkerStart;
	void backgroundWorkerStart_DoWork( Object^ sender, DoWorkEventArgs^ e );
	void backgroundWorkerStart_RunWorkerCompleted( Object^ /*sender*/, RunWorkerCompletedEventArgs^ e );

	// ----------------------------------------------
	protected: 

	private:
		/// <summary>
		/// needed designer variable
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Erforderliche Methode für die Designerunterstützung.
		/// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
		/// </summary>
		void InitializeComponent(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(formStart::typeid));
			this->SuspendLayout();
			// 
			// formStart
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(8, 16);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::Color::White;
			this->ClientSize = System::Drawing::Size(400, 600);
			this->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 13, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Pixel, 
				static_cast<System::Byte>(0)));
			this->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(187)), static_cast<System::Int32>(static_cast<System::Byte>(187)), 
				static_cast<System::Int32>(static_cast<System::Byte>(187)));
			this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
			this->Icon = (cli::safe_cast<System::Drawing::Icon^  >(resources->GetObject(L"$this.Icon")));
			this->Margin = System::Windows::Forms::Padding(3, 4, 3, 4);
			this->Name = L"formStart";
			this->Text = L"qaul.net – قول";
			this->FormClosing += gcnew System::Windows::Forms::FormClosingEventHandler(this, &formStart::formStart_FormClosing);
			this->Load += gcnew System::EventHandler(this, &formStart::formStart_Load);
			this->Shown += gcnew System::EventHandler(this, &formStart::formStart_Shown);
			this->ResumeLayout(false);

		}
#pragma endregion

		// ----------------------------------------------
		// Set Starting view
		void StartHide(void)
		{
			if(startFormActive)
			{
				//delete this->lblStart;
				//this->Controls->Remove(this->lblStart);
				//this->Controls->Remove(this->imgDrehen);
				//this->Controls->Remove(this->label2);
			}
			startFormActive = false;
		}
		
		void StartShow(void)
		{
			if(!startFormActive)
			{
				//delete this->lblStart;
				//this->Controls->Add(this->lblStart);
				//this->Controls->Add(this->imgDrehen);
				//this->Controls->Add(this->label2);
			}
			startFormActive = true;
			QaulStarting();
		}

private: System::Void formStart_Load(System::Object^  sender, System::EventArgs^  e) {
				 // initialize Qaul
			 }
private: System::Void formStart_Shown(System::Object^  sender, System::EventArgs^  e) {
			 QaulStarting();
		 }
private: System::Void formStart_FormClosing(System::Object^  sender, System::Windows::Forms::FormClosingEventArgs^  e) {
		 }


		// ----------------------------------------------
		// Set Chat view
		void ChatInitialize(void)
		{
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(formStart::typeid));
			this->webBrowser1 = (gcnew System::Windows::Forms::WebBrowser());
			this->SuspendLayout();
			// 
			// webBrowser1
			// 
			this->webBrowser1->IsWebBrowserContextMenuEnabled = false;
			this->webBrowser1->Location = System::Drawing::Point(0, 0);
			this->webBrowser1->MinimumSize = System::Drawing::Size(20, 20);
			this->webBrowser1->Name = L"webBrowser1";
			this->webBrowser1->ScrollBarsEnabled = false;
			this->webBrowser1->Size = System::Drawing::Size(400, 600);
			this->webBrowser1->TabIndex = 5;
			this->webBrowser1->Url = (gcnew System::Uri(L"http://127.0.0.1:8081/jqm_qaul.html", System::UriKind::Absolute));
			// 
			// Form1
			// 
			this->Controls->Add(this->webBrowser1);
			this->ResumeLayout(false);
			this->PerformLayout();
			chatFormInitialized = true;
		}

		void ChatShow(void)
		{
			if(!chatFormInitialized) ChatInitialize();
			else if(!chatFormActive)
			{
				this->Controls->Add(this->webBrowser1);
			}
			chatFormActive = true;
		}

		void ChatHide(void)
		{
			if(chatFormActive)
			{
				this->Controls->Remove(this->webBrowser1);
			}
			chatFormActive = false;
		}
		// ----------------------------------------------

		// ----------------------------------------------
		// Set Error view
		void ErrorInitialize(void)
		{
			this->lblError = (gcnew System::Windows::Forms::Label());
			this->btnRetry = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// lblError
			// 
			this->lblError->ForeColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(187)), static_cast<System::Int32>(static_cast<System::Byte>(187)), 
				static_cast<System::Int32>(static_cast<System::Byte>(187)));
			this->lblError->Location = System::Drawing::Point(50, 43);
			this->lblError->Name = L"lblError";
			this->lblError->Size = System::Drawing::Size(300, 75);
			this->lblError->TabIndex = 5;
			this->lblError->Text = L"Error";
			this->lblError->TextAlign = System::Drawing::ContentAlignment::TopCenter;
			// 
			// btnRetry
			// 
			this->btnRetry->ForeColor = System::Drawing::Color::Black;
			this->btnRetry->Location = System::Drawing::Point(156, 138);
			this->btnRetry->Name = L"btnRetry";
			this->btnRetry->Size = System::Drawing::Size(86, 23);
			this->btnRetry->TabIndex = 8;
			this->btnRetry->Text = L"&retry";
			this->btnRetry->UseVisualStyleBackColor = true;
			this->btnRetry->Click += gcnew System::EventHandler(this, &formStart::btnRetry_Click);
			// 
			// formError
			// 
			this->Controls->Add(this->btnRetry);
			this->Controls->Add(this->lblError);
			this->ResumeLayout(false);
			this->PerformLayout();
			errorFormInitialized = true;
		}

		void ErrorShow(System::String^ strError)
		{
			if(startFormActive) StartHide();
			if(chatFormActive) ChatHide();

			if(!errorFormInitialized) ErrorInitialize();
			else if(!errorFormActive)
			{
				this->Controls->Add(this->btnRetry);
				this->Controls->Add(this->lblError);
			}

			// fill in Error
			lblError->Text = strError;

			errorFormActive = true;
		}

		void ErrorHide(void)
		{
			if(errorFormActive)
			{
				this->Controls->Remove(this->btnRetry);
				this->Controls->Remove(this->lblError);
			}
			errorFormActive = false;
		}


private: System::Void btnRetry_Click(System::Object^ sender, System::EventArgs^ e) {
			ErrorHide();
			StartShow();
		}

		// ----------------------------------------------

private: System::Void lblStart_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
private: System::Void imgDrehen_Click(System::Object^  sender, System::EventArgs^  e) {
		 }
};
}
