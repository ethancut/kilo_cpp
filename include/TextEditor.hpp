#include <wx/stc/stc.h>


class TextEditor : public wxStyledTextCtrl {
    public:
        TextEditor(wxWindow* parent);
        const int MARGIN_TEXT_STYLE = wxSTC_STYLE_MAX;
        int m_lastMaxDigits;
        void RefreshLineNumbers(int fromLine);
        void UpdateMarginWidth();
    private:
        void OnModified(wxStyledTextEvent& event);
};