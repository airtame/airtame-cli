# airtame-cli

Airtame-cli is the command line interface to the AIRTAME core streaming software. It communicates with the **streamer** program that runs on the same machine and allows the user to connect the PC to an AIRTAME device.

## Building airtame-cli
The airtame-cli depends on jsonrpc-c library. Before building the airtame-cli you need to make sure jsonrpc-c is fetched and built inside deps folder.
##### Getting the source code and the dependencies
```sh
git clone https://github.com/airtame/airtame-cli.git
cd airtame-cli
git submodule update --init
```
#### Building airtame-cli
```sh
cd airtame-cli && mkdir build
cd build && cmake -DCMAKE_INSTALL_PREFIX:PATH=<install-path> ../
make
```
#### Installing airtame-cli
```sh
make install
```

## Licence
airtame-cli is released under GNU General Public License. Please see the [LICENCE.txt](LICENCE.txt) file.

## Contributions
We would love to get contributions, feedback or bug reports from you. For now we don't have a formalized source code format, but if you want to contribute just try to use the same style of the file you are changing. We plan to clean the code and make it look better very soon :).

### //TODO:
- use linenoise to read the commands in shell mode
- documentation
- cleanup the code
- improve the client side in the jsonrpc-c project
- extract the JSON RPC function and parameters names into a separate file
