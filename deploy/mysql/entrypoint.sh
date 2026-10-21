#!/bin/bash
set -eu
# Windows bind mounts may appear world-writable; MySQL ignores such cnf files.
if [ -f /config/mysql.cnf ]; then
    install -m 644 /config/mysql.cnf /etc/mysql/conf.d/portfolio26.cnf
fi
exec /usr/local/bin/docker-entrypoint.sh "$@"
