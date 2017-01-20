# Rallets-linux

This project is based on [shadowsocks-qt5](https://github.com/shadowsocks/shadowsocks-qt5)

## Development

1. Install dependencies

    ```
    sudo apt-get install build-essential qt5-qmake qtbase5-dev qtcreator libbotan1.10-dev libappindicator-dev
    ```

2. Open Qt Creator from launch pad or command line

    ```
    qtcreator
    ```

3. Select and open the project inside Qt Creator
4. Compile and run

## I18N
1. Generate translate files

    ```
    lupdate shadowsocks-qt5.pro
    ```

2. Open `linguist`, edit translations
3. Create run-time translation files

    ```
    lrelease shadowsocks-qt5.pro
    ```

### Note
If you have both qt4 and qt5 installed, you may need to select the right version by:

```
export QT_SELECT=5
```

### Change your system language and test
```
LANGUAGE=zh_CN
rallets
```

## Build debian package

### Build for x64

```
./scripts/build_64bit.sh
```

### Build for x86

Docker should be installed for building 32bit package

```
./scripts/build_32bit.sh
```

### Clean
```
dpkg-buildpackage -Tclean
```

## Usage

### Fresh start on ubuntu

1. Install google chrome

    ```
    wget https://dl.google.com/linux/direct/google-chrome-stable_current_amd64.deb
    dpkg -i google-chrome-stable_current_amd64.deb
    ```
2. Open rallets for linux, login and connect to a server
3. Open chrome with a proxy

    ```
    google-chrome --proxy-server="socks5://127.0.0.1:1080"
    ```
4. Go into chrome app store, install `SwitchyOmega`
5. Setup a new proxy profile: `SOCKS5`, `127.0.0.1`, `1080`
6. Setup a new switch profile:

    In `Rule List Config`, select `AutuProxy`

    In `Rule List URL`, put `https://raw.githubusercontent.com/gfwlist/gfwlist/master/gfwlist.txt`

    Click `Download Profile Now`

    Click `Apply changes`
7. Click SwitchyOmega button, select the switch profile
8. There you go!

## LICENSE

![](http://www.gnu.org/graphics/lgplv3-147x51.png)

Copyright © 2014-2016 Symeon Huang

This project is licensed under version 3 of the GNU Lesser General Public License.
