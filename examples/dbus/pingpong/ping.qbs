import qbs

QtExample {
    name: "ping"
    condition: Qt.dbus.present
    installDir: project.examplesInstallDir + "/dbus/pingpong"
    Depends { name: "Qt.dbus"; required: false }
    files: [
        "ping-common.h",
        "ping.cpp",
    ]
}
