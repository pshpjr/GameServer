FROM mysql:8.4
COPY init/ /docker-entrypoint-initdb.d/
COPY entrypoint.sh /usr/local/bin/portfolio26-entrypoint
RUN sed -i 's/\r$//' /usr/local/bin/portfolio26-entrypoint && chmod 755 /usr/local/bin/portfolio26-entrypoint
ENTRYPOINT ["/usr/local/bin/portfolio26-entrypoint"]
CMD ["mysqld"]
