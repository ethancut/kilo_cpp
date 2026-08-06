#include "TextEditor.hpp"
#include <wx/colour.h>

TextEditor::TextEditor(wxWindow* parent) : wxStyledTextCtrl(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE) {

    this->StyleSetBackground(wxSTC_STYLE_DEFAULT, wxColour("#2E2E2E"));
    this->StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColour("#FFFFFF"));

    this->StyleClearAll();
    //padding, scrolling
    this->SetMarginLeft(0);
    this->SetMarginRight(20);
    this->SetScrollWidth(1);
    this->SetScrollWidthTracking(true);
    // line number margin
    this->SetMarginType(0, wxSTC_MARGIN_NUMBER);
    this->SetMarginWidth(0, 50);
    this->StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour("#363636"));
    this->StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour("#8F8F8F"));
    //caret (selected line)
    this->SetCaretLineVisible(true);
    this->SetCaretLineBackground(wxColour("#363636"));
    this->SetCaretForeground(wxColour("#FFFFFF"));
    //textareacolor
}