#include <iostream>
#include <QApplication>
#include "TimeWindow.h"

int main() {
	//std::cout << "AutomaticChessClock: starting (stub main)\n";
	QApplication app(argc, argv);

    TimeWindow window;
    window.show();

    return app.exec();
	return 0;
}