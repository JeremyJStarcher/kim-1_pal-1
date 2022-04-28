# emcc dice-roll.c ../../src/kim-1-due/src/webmain.cpp -s LINKABLE=1 -s EXPORT_ALL=1 -s WASM=1 -O3 -D TARGETWEB -o index.js  

# Make a local copy so we can access it via our source maps
DIST="dist/";
rm -rf ${DIST}
mkdir -p ${DIST}

CSRC="${DIST}csrc"
cp ../src/kim-1-due/src ${CSRC} -Rv

SOURCE="src/c/main.c"
SOURCE="src/c/serial_display.cpp"
SOURCE="${SOURCE} ${CSRC}/webmain.cpp"
SOURCE="${SOURCE} ${CSRC}/cpu.cpp"
SOURCE="${SOURCE} ${CSRC}/led_driver.cpp"
SOURCE="${SOURCE} ${CSRC}/host-hardware.cpp"
SOURCE="${SOURCE} ${CSRC}/MemIo/MemIoBase.cpp"
SOURCE="${SOURCE} ${CSRC}/MemIo/MemIo.cpp"
SOURCE="${SOURCE} ${CSRC}/MemIo/MemIoRom.cpp"
SOURCE="${SOURCE} ${CSRC}/MemIo/MemIoRam.cpp"
SOURCE="${SOURCE} ${CSRC}/MemIo/Riot002.cpp"
SOURCE="${SOURCE} ${CSRC}/MemIo/RiotTimer.cpp"

echo "SOURCE: ${SOURCE}"
echo "CSRC: ${CSRC}"

LEVEL="-O0 -g4"
#LEVEL="-O3"

emcc ${SOURCE} --emrun -s EXPORTED_FUNCTIONS='["_debuggetaddress", "_websetup", "_webloop", "_injectkey"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' -s WASM=1 ${LEVEL} -D TARGETWEB -o ${DIST}/index.js

cp index.html ${DIST}
cp *.css ${DIST}
cp *.js ${DIST}

cd ${DIST}
emrun --no_browser --port 8080 ${DIST} 
cd ..

#python -m SimpleHTTPServer 9000
