# Updating icons
# inspired by https://stackoverflow.com/questions/12306223/how-to-manually-create-icns-files-using-iconutil

on macos update the SVG file

ensure imagemagick
```
brew install imagemagick --with-librsvg
```

then perform the conversions from the image dir
```
./svg2icons.bash IconFile.svg
```
