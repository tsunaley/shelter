#!/bin/bash

# Check if an IP address is provided as a command-line argument
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <ip_address> <memory_size>"
    exit 1
fi

ip_address=$1
memory_size=$2

if [ -f measurement.bin ]; then
    rm -f measurement.bin
fi

if ls secret_* 1> /dev/null 2>&1; then
    rm -f secret_*
fi

if ls sev_* 1> /dev/null 2>&1; then
    rm -f sev_*
fi

if pidof qemu-system-x86_64 > /dev/null; then
    kill -9 $(pidof qemu-system-x86_64)
fi

if pidof b_relay > /dev/null; then
    kill -9 $(pidof b_relay)
fi

sevctl export /tmp/sev.pdh
check_cert_chain "$ip_address"

if [ ! -f "sev_godh.b64" ]; then
    echo "Error: File sev_godh.b64 not exist!"
    exit 1
fi

if [ ! -f "sev_session.b64" ]; then
    echo "Error: File sev_session.b64 not exist!"
    exit 1
fi

b_relay &

qemu-system-x86_64 \
-device vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid=3 \
-enable-kvm \
-cpu EPYC \
-m "$memory_size" \
-bios /usr/local/bin/OVMF.fd \
-kernel /usr/local/bin/bzImage \
-net none -nographic \
-initrd /usr/local/bin/initrd.img \
-append "root=/dev/ram rdinit=/init console=ttyS0,115200" \
-nographic \
-object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=1,policy=0x1,dh-cert-file=sev_godh.b64,session-file=sev_session.b64 \
-machine confidential-guest-support=sev0 \
-qmp unix:/tmp/qmp-sock,server,nowait -S \
-netdev bridge,id=net0,br=br0 \
-device virtio-net-pci,netdev=net0 \
> /var/log/qemu.log 2>&1 &
sleep 2
{
    echo '{"execute": "qmp_capabilities"}'
    echo '{"execute": "query-sev-launch-measure"}'
} | socat - UNIX-CONNECT:/tmp/qmp-sock > measurement.bin

if ! grep -q '"data"' measurement.bin; then
    sleep 5
    {
        echo '{"execute": "qmp_capabilities"}'
        echo '{"execute": "query-sev-launch-measure"}'
    } | socat - UNIX-CONNECT:/tmp/qmp-sock > measurement.bin
fi

if ! grep -q '"data"' measurement.bin; then
    sleep 15
    {
        echo '{"execute": "qmp_capabilities"}'
        echo '{"execute": "query-sev-launch-measure"}'
    } | socat - UNIX-CONNECT:/tmp/qmp-sock > measurement.bin
fi

# sleep 2
remote_attestation "$ip_address"

if [ ! -f "secret_header.b64" ]; then
    echo "Error: File secret_header.b64 not exist!"
    exit 1
fi

if [ ! -f "secret_payload.b64" ]; then
    echo "Error: File secret_payload.b64 not exist!"
    exit 1
fi

packet_header=$(<secret_header.b64)
secret=$(<secret_payload.b64)

command="{ \"execute\": \"sev-inject-launch-secret\", \"arguments\": { \"packet-header\": \"$packet_header\", \"secret\": \"$secret\" } }"


{
    echo '{ "execute": "qmp_capabilities" }'
    sleep 1

    echo "$command"
    sleep 1

    echo '{ "execute": "cont" }'
} | socat - UNIX-CONNECT:/tmp/qmp-sock