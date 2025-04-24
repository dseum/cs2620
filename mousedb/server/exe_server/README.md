# exe_server

This is the server that provides a frontend to the database. The focus of this project is only on features related to the server such as networking, authentication, and connections.

## Development

In `lib_database`, you need to build the release target and install it with `cmake --install build`. While the build will be cached to your filesystem, you will have to install it any time you down the container since the install is intentionally not persistent.
