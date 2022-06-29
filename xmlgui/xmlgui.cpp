/*
 * xmlgui.cpp
 *
 *  Created on: Jan 24, 2020
 *      Author: thomas
 */

#include <QApplication>
#include <iostream>

#include "ConfigWindowPresenter.h"
#include "ConfigWindowView.h"

int main(int argc, char** argv)
{
  QApplication app(argc, argv);

  if (argc > 1)
  {
    std::string configFile(argv[1]);
    ConfigWindowView configView;
    ConfigWindowPresenter configPresenter(&configView, configFile);

    configView.show();

    return app.exec();
  }
  else
  {
    std::cout << "No config file was specified! Usage: ./gui <configfile>"
              << std::endl;
  }
  return -1;
}
