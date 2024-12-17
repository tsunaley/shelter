FROM ubuntu:local
RUN apt-get update && apt-get install -y                    qemu-system-x86                    socat                    && apt-get clean 
COPY tests/defualt_component/sevctl /usr/local/bin/sevctl
COPY tests/defualt_component/bzImage /usr/local/bin/bzImage
COPY tests/defualt_component/OVMF.fd /usr/local/bin/OVMF.fd
COPY tests/defualt_component/start.sh /usr/local/bin/start.sh
COPY tests/defualt_component/b_relay /usr/local/bin/b_relay
COPY bin/server_boot /usr/local/bin/
COPY bin/server_continue /usr/local/bin/
RUN chmod +x /usr/local/bin/start.sh
