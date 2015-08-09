TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES -= UNICODE

SOURCES += game.cpp gamestate.cpp gui.cpp image.cpp main.cpp panel.cpp player.cpp resources.cpp screen.cpp sector.cpp sound.cpp tutorial.cpp utils.cpp TinyXML/tinyxml.cpp TinyXML/tinyxmlerror.cpp TinyXML/tinyxmlparser.cpp

INCLUDEPATH += ../APIs/SDL2/include/
INCLUDEPATH += ../APIs/SDL2_image/include/
INCLUDEPATH += ../APIs/SDL2_mixer/include/

LIBS += -L$$PWD # add the source folder for libs
LIBS += -lUser32 -lShell32 -lSDL2 -lSDL2main -lSDL2_image -lSDL2_mixer

include(deployment.pri)
qtcAddDeployment()
