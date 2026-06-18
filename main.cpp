#include "mainwindow.h"
#include <QApplication>
#include <QString>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set dark theme stylesheet
    a.setStyleSheet(R"(
        QWidget {
            background-color: #2e2e2e;
            color: #cccccc;
            font-size: 10pt;
        }

        QMainWindow {
            background-color: #1e1e1e;
        }
        
        QMenuBar, QMenuBar::item {
            background-color: #3c3c3c;
            color: #cccccc;
        }

        QMenuBar::item:selected {
            background-color: #5a5a5a;
        }

        QMenu {
            background-color: #3c3c3c;
            border: 1px solid #5a5a5a;
        }

        QMenu::item:selected {
            background-color: #5a5a5a;
        }

        QToolBar {
            background-color: #3c3c3c;
            border: none;
        }

        QSplitter::handle {
            background-color: #3c3c3c;
            width: 1px;
        }

        QSplitter::handle:hover {
            background-color: #5a5a5a;
        }

        QListWidget {
            background-color: #252525;
            border: 1px solid #3c3c3c;
        }

        QLineEdit, QTextEdit {
            background-color: #252525;
            border: 1px solid #3c3c3c;
            padding: 2px;
        }
        
        QLabel {
             background-color: transparent;
        }

        /* Style for the central viewport widget */
        QWidget#viewport {
            background-color: #333;
        }
    )");

    MainWindow w;
    w.show();

    return a.exec();
}
