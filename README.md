# HAR image extractor

This is the C fork of the HAR image extractor, originally [forked by me](https://gist.github.com/Lewiscowles1986/645e79295efa84698f4e45cd06d610ea/0d1e0735b70e469143c2b65ee79223bd0326aef6) from [this Gist](https://gist.github.com/Lewiscowles1986/645e79295efa84698f4e45cd06d610ea/0d1e0735b70e469143c2b65ee79223bd0326aef6)
by [Kafran](https://github.com/kafran) using the [Python programming language](https://www.python.org/).

## Compiling

### Disclaimer

I've not tested this on Linux or Windows yet. I've in-fact only built and ran it on an Apple M1 Ultra using homebrew.
I've also not compiled this against a matrix of compilers; 

```
$ gcc --version
Apple clang version 15.0.0 (clang-1500.0.40.1)
Target: arm64-apple-darwin23.1.0
Thread model: posix
InstalledDir: /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin
```

### Pre-requisites (OSX)

- OSX [xcode command-line dev tools](https://www.freecodecamp.org/news/install-xcode-command-line-tools/) (should ship with gcc, git, etc) `xcode-select --install`
- Install [Homebrew](https://brew.sh/) `$SHELL -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`
- `brew install libb64`

### Running the build (OSX)

```
gcc main.c \
    -o har-img-extract \
    -I/opt/homebrew/Cellar/libb64/1.2.1_1/include \
    -L/opt/homebrew/Cellar/libb64/1.2.1_1/lib \
    -lb64
```

## Usage

As you'll see from the below, I'm not sure if this supports anything besides absolute paths.

`./har-img-extract $(pwd)/sample_data/imgur.har $(pwd)/out/a/b/c/d`

## Background

2023-11-04, someone linked me a very interesting [C fork](https://gist.github.com/OhMyCatile/c1077fada299797254686f592b00d009). I could not find it's dependencies, which stopped me compiling it; but a quick scan through, confirmed it didn't seem to be malware, and was in-fact C code.

Among my frustration with wanting to see if there were edge-cases between C and the, more forgiving python, I set about trying to make the thing compile:

- Seek out dependencies
- Failing at that, try to replace dependencies
  - [superwills](https://github.com/superwills)[NibbleAndAHalf](https://github.com/superwills/NibbleAndAHalf) base64 library
    - `curl -O https://raw.githubusercontent.com/superwills/NibbleAndAHalf/f83d3842baad289392f693ba81cb77b5eb3848b7/NibbleAndAHalf/base64.c > base64.c`
    - `curl -O https://raw.githubusercontent.com/superwills/NibbleAndAHalf/f83d3842baad289392f693ba81cb77b5eb3848b7/NibbleAndAHalf/base64.h > base64.h`
  - [sheredom](https://github.com/sheredom)[json.h](https://github.com/sheredom/json.h)
    - `curl -O https://raw.githubusercontent.com/sheredom/json.h/06aa5782d650e7b46c6444c2d0a048c0a1b3a072/json.h > json.h`
- Polyfilling what I hope will be cross-os compatibility
  - See: `int create_directory(const char *path);`
- Remove `free` calls in-case they led to segfaults (they likely had nothing to do with them 😊)
- Removing behavior I found unintuitive, such as taking the input file-name if no out directory was specified.
- That then changed into working out diffent things that I perceived to be bugs:
  - JSON array code deleted
  - Binary Read mode `rb`
  - Binary Write mode `wb`
  - Inseting C-String `\0` characters
  - Removing C-String `\0` characters
  - Mkdir following `mkdir -p` conventions
  - Not all HAR content being `base64` encoded

What you have here now is by no means a glorious example of C; but rather an example of me iterating via the compiler and runtime errors; until you get what is here now, that has been tested working, with two sample HAR archives.

It's actually going to lead to an improvement in the original Python Gist fork I maintain, which is... Unexpected.

## Todo

- Add tests
- build or use an output / logging library for info, error, debug messages
- test on other OS / processor architecture combinations
  - x86
  - ARM
  - RISCV
  - windows
  - Linux
- Automated builds 
- Review contributions
