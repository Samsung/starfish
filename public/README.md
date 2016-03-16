# StarFish

## Directory Structure
The directory structure is as follows.

```sh
preprocessor\
  convert.sh
  runner.js
  tool\

tizen\
  wearable2.3\
    inc\
      *.h
    shared\
      res\
    so\
      wearable_arm\
        *.so
      wearable_emulator\
        *.so
    src\
      starfish.c
```

- preprocessor: Contains a script that converts a web widget written in HTML to an XML format required by StarFish.
- tizen: Contains a skeleton tizen IDE package. This package can be loaded by a Tizen SDK to run a web widget.
- inc: Includes a StarFish header files.
- shared: A web widget in XML format is to be placed here along with any resouce files, i.e., \*.jpg
- so: Contains library files required by a target and an emulator.
- src: starfish.c contains main() that executes a web widget.

## Usage

1. Convert a web widget written in HTML to an XML format.
  ```sh
  cd preprocessor
  ./convert.sh widget.html
  ```

2. Copy the web widget in XML to a shared directory in tizen IDE package.

  ```sh
  cp -r out/* ../tizen/wearable2.3/shared
  ```

3. Load wearable2.3 package from Tizen IDE

4. Set StarFish library path from Tizen IDE either for a device or emulator.

5. Compile and run



