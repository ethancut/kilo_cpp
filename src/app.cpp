#include "app.hpp"
#include "MainFrame.hpp"

bool App::OnInit() {
    MainFrame *mainF = new MainFrame();
    mainF->Show();
    return true;
}