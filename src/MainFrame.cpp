#include "MainFrame.hpp"
#include "TextEditor.hpp"
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/event.h>
#include <wx/wxprec.h>

enum {
    ID_Hello = 1
};


MainFrame::MainFrame() : wxFrame(nullptr, wxID_ANY,  "Notebook", wxDefaultPosition, wxSize(800, 600)) {
        wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Hello, "&New\tCtrl+N",
                     "Open a new text file");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
 
    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
 
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
 
    SetMenuBar(menuBar);
 
    CreateStatusBar();
    SetStatusText("Welcome to Notebook!");

    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    TextEditor *editor = new TextEditor(this);

    
    mainSizer->Add(editor, 1, wxEXPAND, 0);

    this->SetSizer(mainSizer);

    

    Bind(wxEVT_MENU, &MainFrame::OnAbout, this, wxID_ABOUT);
}

 void MainFrame::OnAbout(wxCommandEvent& event) {
    wxMessageBox("Notebook is a simple text editor created for educational purposes. \nCreated by ethannself");
 }