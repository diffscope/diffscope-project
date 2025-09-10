mkdir -p /tmp/app.iconset
cp ./src/app/icons/app/16x16.png /tmp/app.iconset/icon_16x16.png
cp ./src/app/icons/app/32x32.png /tmp/app.iconset/icon_16x16@2x.png
cp ./src/app/icons/app/32x32.png /tmp/app.iconset/icon_32x32.png
cp ./src/app/icons/app/64x64.png /tmp/app.iconset/icon_32x32@2x.png
cp ./src/app/icons/app/128x128.png /tmp/app.iconset/icon_128x128.png
cp ./src/app/icons/app/256x256.png /tmp/app.iconset/icon_128x128@2x.png
cp ./src/app/icons/app/256x256.png /tmp/app.iconset/icon_256x256.png
cp ./src/app/icons/app/512x512.png /tmp/app.iconset/icon_256x256@2x.png
cp ./src/app/icons/app/512x512.png /tmp/app.iconset/icon_512x512.png
cp ./src/app/icons/app/1024x1024.png /tmp/app.iconset/icon_512x512@2x.png
iconutil -c icns -o ./src/app/app.icns /tmp/app.iconset
rm -rf /tmp/app.iconset