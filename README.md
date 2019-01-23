# RTSP-Streaming
Beleg Videostreaming für das Modul Internettechnologien 2

##Documents given by professor

* [Projektbeschreibung](Projektbeschreibung.md)
* [Aufgabenstellung](Aufgabenstellung.md)
* [Abgabeformat](Abgabeformat.md)
* [Git/GitHub-Nutzung](git.md)

##[Implementation notes](doc/notes.md)

#Execution
##Client
Execute the `rtsp_gui` executable in the build/bin directory
##Server
Execute the `rtsp_server_standalone` executable in the build/bin directory for default settings or
for example `./rtsp_server_standalone 5054 ../res` in the build directory for port 5054 usage and using the project
resource directory for providing the example movie.mjpeg

#Building
##Dependencies
To build this project following libraries are needed
*   Boost >= 1.66
*   Qt >= 4.8
Qt 4.8 is installed on ilux150 so build boost and install it to home one could use 
```
cd /tmp/
git clone https://github.com/boostorg/boost.git
cd boost/
git submodule update  --init --recursive
./bootstrap.sh --prefix=$HOME
./b2 
./b2 install
```

##Project
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
- RTSP server could try to send on boost asio tcp socket, when messages are coming in to quick
Can be solved by a out queue in the rtsp server tcp connection
- RTSP require header handling for all methods/cleanup
- RTSP Connection header handling


