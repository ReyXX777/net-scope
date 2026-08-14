QT       += core gui widgets network
CONFIG   += c++17
TARGET    = PacketInspector
TEMPLATE  = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    captureworker.cpp \
    packetmodel.cpp \
    hexview.cpp \
    pcap_file.cpp

HEADERS += \
    packet_structs.h \
    mainwindow.h \
    captureworker.h \
    packetmodel.h \
    hexview.h \
    pcap_file.h

# --- Npcap SDK Configuration (Self-contained in project folder) ---
NPCAP_SDK_PATH = $$PWD/3rdparty/npcap

exists($$NPCAP_SDK_PATH) {
    INCLUDEPATH += $$NPCAP_SDK_PATH/Include
    
    # Hardcode x64 since we are using mingw81_64
    LIBS += -L$$NPCAP_SDK_PATH/Lib/x64 -lwpcap -lws2_32
    
    DEFINES += HAVE_NPCAP
} else {
    error("Npcap SDK not found at $$NPCAP_SDK_PATH. Please extract the Npcap SDK there.")
}
# Windows-specific: Ensure console subsystem is disabled for GUI app
win32:CONFIG -= console