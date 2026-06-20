#ifndef NATIVE_H
#define NATIVE_H

/*
 * Native Messaging Host Loop.
 * Listens on stdin for messages sent by the Firefox WebExtension,
 * and replies on stdout matching Mozilla's native messaging protocol.
 * Returns 0 on clean exit (e.g. EOF on stdin), -1 on error.
 */
int native_messaging_loop(const char *filepath);

#endif
