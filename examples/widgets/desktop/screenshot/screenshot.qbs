import qbs

QtExample {
    name: "screenshot"
    condition: Qt.widgets.present

    Depends { name: "Qt.gui"; required: false }
    Depends { name: "Qt.widgets"; required: false }

    files: [
        "main.cpp",
        "screenshot.cpp",
        "screenshot.h",
    ]
}
