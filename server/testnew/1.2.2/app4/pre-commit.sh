#!/bin/bash
set -e

echo "$PCO_STAGING_ARTIFACTS_PATH"

CG="/sys/fs/cgroup"
CG_PCO="$CG/pco"
CG_CONT="$CG_PCO/cont"

# Настройка cgroup
mkdir -p "$CG"
echo "+memory" > "$CG/cgroup.subtree_control"

mkdir -p "$CG_PCO"
echo "+memory" > "$CG_PCO/cgroup.subtree_control"

mkdir -p "$CG_CONT"
echo "10000000" > "$CG_CONT/memory.max"

# Запуск в новом namespace, если не "inner"
if [ "$1" != "inner" ]; then
    exec unshare -mu --mount-proc --propagation private -- "$0" inner
fi

# Подготовка контейнера
CONTAINER="/tmp/pco/staging/container"
mkdir -p "$CONTAINER"
mount -t tmpfs none "$CONTAINER"

NEWROOT="$CONTAINER/root"
mount --make-rprivate /
mkdir -p "$NEWROOT"
mount --bind "$NEWROOT" "$NEWROOT"

mkdir -p "$NEWROOT"/{usr,bin,tmp,opt,lib64}

# BusyBox
busybox_path=$(which busybox)
if [ -z "$busybox_path" ]; then
    echo "busybox is empty"
    exit 1
fi
cp "$busybox_path" "$NEWROOT/bin/"
cd "$NEWROOT/bin"
./busybox --install -s .

# Копирование бинарников и зависимостей
IFS=':' read -r -a targets <<< "$PCO_REQUIRED_ARTIFACTS_PATHS"
IFS=':' read -r -a sources <<< "$PCO_STAGING_ARTIFACTS_PATHS"
t_idx=0

for src in "${sources[@]}"; do
    # Пропускаем скрипты
    if [[ "$src" == *.sh ]]; then
        echo "Skipping script $src"
        continue
    fi

    # Пропуск, если пути назначения закончились
    if [ "$t_idx" -ge "${#targets[@]}" ]; then
        echo "No target left for $src, skipping"
        continue
    fi

    dest="$NEWROOT${targets[$t_idx]}"
    t_idx=$((t_idx + 1))

    mkdir -p "$(dirname "$dest")"
    cp -u "$src" "$dest"
    echo "Copied $src -> $dest"

    deps=$(ldd "$src" 2>/dev/null | awk '{print $3}' | grep '^/')
    for f in $deps; do
        dep_path="$NEWROOT$f"
        mkdir -p "$(dirname "$dep_path")"
        cp "$f" "$dep_path"
        echo "Copied dependency $f -> $dep_path"
    done
done

cp /lib64/ld-linux-x86-64.so.2 "$NEWROOT/lib64/"

# pivot_root
mkdir -p "$NEWROOT/.oldroot"
pivot_root "$NEWROOT" "$NEWROOT/.oldroot"
cd /

# Настройка /dev
busybox mkdir -p /dev
busybox mknod -m 666 /dev/null c 1 3
busybox mknod -m 666 /dev/zero c 1 5
busybox mknod -m 666 /dev/random c 1 8
busybox mknod -m 666 /dev/urandom c 1 9

busybox umount -l /.oldroot
busybox rm -rf /.oldroot

# Повторное подключение cgroup
busybox mkdir -p "$CG"
busybox mount -t cgroup2 none "$CG"

echo 1 > /sys/fs/cgroup/pco/cont/cgroup.kill

# Запуск процессов из targets и добавление в cgroup
for bin in "${targets[@]}"; do
    "$bin" &
    PID=$!
    echo "$PID" >> "$CG_CONT/cgroup.procs"
    echo "Started $bin with PID $PID"
done

# Мониторинг cgroup
pc=$(busybox wc -l < "$CG_CONT/cgroup.procs")

for i in $(busybox seq 1 200); do
    mem=$(busybox cat "$CG_CONT/memory.current")
    if [ "$mem" -gt 10000000000 ]; then
        echo "Memory limit exceeded"
        echo 1 > "$CG_CONT/cgroup.kill"
        kill -9 "$PID"
        exit 2
    fi

    echo "$mem"
    curr=$(busybox wc -l < "$CG_CONT/cgroup.procs")

    if [ "$pc" -ne "$curr" ]; then
        echo "Some of processes exited"
        exit 12
    fi

    busybox sleep 1
done

echo 1 > "$CG_CONT/cgroup.kill"
exit 0
