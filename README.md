# RTSP-Streaming
Beleg Videostreaming für das Modul Internettechnologien 2

##Documents given by professor

* [Projektbeschreibung](Projektbeschreibung.md)
* [Aufgabenstellung](Aufgabenstellung.md)
* [Abgabeformat](Abgabeformat.md)
* [Git/GitHub-Nutzung](git.md)

##[Implementation notes](doc/notes.md)

#Building
To build this project following libraries are needed
*   Boost >= 1.66
*   Qt >= 4.8

Building is done via cmake.
So after cloning one could for example
```
mkdir bin
cd bin
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
ctest
```
or when boost is installed in a non standard directory, for example the home:
```
cmake -D"BOOST_ROOT"="~" -D"BOOST_LIBRARYDIR"="~/lib" -DCMAKE_BUILD_TYPE=Release ..
```

#Known Issues:
RTSP server could try to send on boost asio tcp socket, when messages are coming in to quick
Can be solved by a out queue in the rtsp server tcp connection


