### XMR-Multisweeper
This tool scans the Monero blockchain looking for decryptable owned transaction outputs and will sweep all found outputs to a single wallet of your choice, effectively consolidating several wallets into one. With multithreading support!

### Build instructions

First make sure you have all the necessary libraries install to build [monero](https://github.com/monero-project/monero).
Then simply run `./build.sh`, your binaries will be outputted to the `build` subdirectory.

### Usage

This program uses command line arguments to feed it some necessary information. These are
* (optional) the number of concurrent threads to use. This is specified with `-j <thread_count>` where thread_count is an integer.
* a Monero node to connect to (the daemon interface not the wallet RPC). If you are running your own node the default is 127.0.0.1:18080. Lists of public nodes can be found on the internet.
* a Monero address that should receive any funds found.
* a comma seperated list of secret keys in the form of a hexadecimal or a 25 word mnemonic (the reference implementation format).

Example usage: `./multisweeper 127.0.0.1:18080 -j 2 46YCMVGbdnfCoA7PammbanBtyaTzhLzgF3bEJAeybFQZ4aTZ4cu76KHKXe4Mft9yaodUBzs3HQpW23qmiJB2NZ714snYrAV boss wedge inbound avoid enforce smelting entrance much salads icon yeti volcano nightly seismic gymnast avoid dewdrop deepest shipped nitrogen pizza ointment acoustic adventure adventure, 3dc0c516976cf04e8d71c24ef6a6e012e8489379c1e8b6b7077e9fdd805e6202`